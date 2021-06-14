/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.cpp
 
 **********************************************************************/


#include "RealtimeEffectManager.h"
#include "RealtimeEffectState.h"

#include "EffectInterface.h"
#include <memory>
#include "../Project.h"
#include "../ProjectHistory.h"
#include "../ProjectSettings.h"
#include "PluginManager.h"
#include "ProjectFileIORegistry.h"
#include "Track.h"
#include "UndoManager.h"

#include <atomic>
#include <chrono>

static ProjectFileIORegistry::Entry registerFactory
{
   wxT( "effects" ),
   [](AudacityProject &project)
   {
      auto & manager = RealtimeEffectManager::Get(project);
      return manager.ReadXML(project);
   }
};

RealtimeEffectManager::RealtimeEffectManager(AudacityProject &project)
:  mProject(project)
{
   std::lock_guard<std::mutex> guard(mLock);

   mActive = false;
   mSuspended = false;
   mProcessing = false;
   mLatency = std::chrono::microseconds(0);
}

RealtimeEffectManager::~RealtimeEffectManager()
{
}

bool RealtimeEffectManager::IsActive() const
{
   return mActive;
}

bool RealtimeEffectManager::IsSuspended() const
{
   return mSuspended;
}

void RealtimeEffectManager::Suspend()
{
   wxLogDebug(wxT("Suspend"));
   // Already suspended...bail
   if (mSuspended)
   {
      return;
   }

   // Show that we aren't going to be doing anything
   mSuspended = true;

   auto & masterList = RealtimeEffectList::Get(mProject);
   masterList.Suspend();

   for (auto leader : mGroupLeaders)
   {
      auto & trackList = RealtimeEffectList::Get(*leader);
      trackList.Suspend();
   }
}

void RealtimeEffectManager::Resume()
{
   wxLogDebug(wxT("Resume"));
   // Already running...bail
   if (!mSuspended)
   {
      return;
   }
 
   auto & masterList = RealtimeEffectList::Get(mProject);
   masterList.Resume();

   for (auto leader : mGroupLeaders)
   {
      auto & trackList = RealtimeEffectList::Get(*leader);
      trackList.Resume();
   }

   // Show that we aren't going to be doing anything
   mSuspended = false;
}

void RealtimeEffectManager::Initialize(double rate)
{
   wxLogDebug(wxT("Initialize"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(!mActive);
   wxASSERT(!mSuspended);

   // The audio thread should not be running yet, but protect anyway
   Suspend();

   // Remember the rate
   mRate = rate;

   // (Re)Set processor parameters
   mChans.clear();
   mRates.clear();
   mGroupLeaders.clear();

   // RealtimeAdd/RemoveEffect() needs to know when we're active so it can
   // initialize newly added effects
   mActive = true;

   // Get things moving
   Resume();
}

void RealtimeEffectManager::AddProcessor(Track *track, unsigned chans, float rate)
{
   wxLogDebug(wxT("AddProcess"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(mActive);
   wxASSERT(!mSuspended);

   Suspend();

   auto leader = *track->GetOwner()->FindLeader(track);
   mGroupLeaders.push_back(leader);
   mChans.insert({leader, chans});
   mRates.insert({leader, rate});

   ProcessGroup(leader,
      [&](RealtimeEffectState & state, bool bypassed)
      {
         state.Initialize(rate);

         state.AddProcessor(leader, chans, rate);
      }
   );

   Resume();
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::ProcessStart()
{
   wxLogDebug(wxT("ProcessStart"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(mActive);
   wxASSERT(!mProcessing);

   Suspend();

   mCurrentGroup = 0;

   for (auto leader : mGroupLeaders)
   {
      ProcessGroup(leader,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            state.ProcessStart();
         }
      );
   }

   mProcessing = true;

   Resume();
}

//
// This will be called in a different thread than the main GUI thread.
//
size_t RealtimeEffectManager::Process(Track *track,
                                      float gain,
                                      float **buffers,
                                      size_t numSamps)
{
   wxLogDebug(wxT("Process"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   if (mSuspended || !mProcessing)
   {
      return 0;
   }

   auto numChans = mChans[track];

   // Remember when we started so we can calculate the amount of latency we
   // are introducing
   auto start = std::chrono::steady_clock::now();

   // Allocate the in, out, and prefade buffer arrays
   float **ibuf = (float **) alloca(numChans * sizeof(float *));
   float **obuf = (float **) alloca(numChans * sizeof(float *));
   float **pibuf = (float **) alloca(numChans * sizeof(float *));
   float **pobuf = (float **) alloca(numChans * sizeof(float *));

   auto group = mCurrentGroup++;

   auto prefade = HasPrefaders(group);

   // And populate the input with the buffers we've been given while allocating
   // NEW output buffers
   for (unsigned int c = 0; c < numChans; ++c)
   {
      ibuf[c] = buffers[c];
      obuf[c] = (float *) alloca(numSamps * sizeof(float));

      if (prefade)
      {
         pibuf[c] = (float *) alloca(numSamps * sizeof(float));
         pobuf[c] = (float *) alloca(numSamps * sizeof(float));

         memcpy(pibuf[c], buffers[c], numSamps * sizeof(float));
      }
   }

   // Apply gain
   if (gain != 1.0)
   {
      for (auto c = 0; c < numChans; ++c)
      {
         for (auto s = 0; s < numSamps; ++s)
         {
            ibuf[c][s] *= gain;
         }
      }
   }

   // Tracks how many processors were called
   size_t called = 0;

   // Now call each effect in the chain while swapping buffer pointers to feed the
   // output of one effect as the input to the next effect
   if (HasPostfaders(group))
   {
      ProcessGroup(track,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            if (bypassed || state.IsPreFade())
            {
               return;
            }

            state.Process(track, numChans, ibuf, obuf, numSamps);

            for (auto c = 0; c < numChans; ++c)
            {
               std::swap(ibuf[c], obuf[c]);
            }
            called++;
         }
      );

      // Once we're done, we might wind up with the last effect storing its results
      // in the temporary buffers.  If that's the case, we need to copy it over to
      // the caller's buffers.  This happens when the number of effects processed
      // is odd.
      if (called & 1)
      {
         for (auto c = 0; c < numChans; ++c)
         {
            memcpy(buffers[c], ibuf[c], numSamps * sizeof(float));
         }
      }
   }

   if (prefade)
   {
      ProcessGroup(track,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            if (bypassed || !state.IsPreFade())
            {
               return;
            }

            state.Process(track, numChans, pibuf, pobuf, numSamps);

            for (auto c = 0; c < numChans; ++c)
            {
               for (auto s = 0; s < numSamps; ++s)
               {
                  buffers[c][s] += pobuf[c][s];
               }

               std::swap(pibuf[c], pobuf[c]);
            }

            called++;
         }
      );
   }

   // Remember the latency
   auto end = std::chrono::steady_clock::now();
   mLatency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

   //
   // This is wrong...needs to handle tails
   //
   return called ? numSamps : 0;
}

//
// This will be called in a different thread than the main GUI thread.
//
void RealtimeEffectManager::ProcessEnd()
{
   wxLogDebug(wxT("ProcessEnd"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   if (!mProcessing)
   {
      return;
   }

   Suspend();

   for (auto leader : mGroupLeaders)
   {
      ProcessGroup(leader,
         [&](RealtimeEffectState &state, bool bypassed)
         {
            state.ProcessEnd();
         }
      );
   }

   mProcessing = false;

   Resume();
}

void RealtimeEffectManager::Finalize()
{
   wxLogDebug(wxT("Finalize"));

   wxASSERT(mActive);
   wxASSERT(!mProcessing);
   wxASSERT(!mSuspended);

   while (mProcessing.load())
   {
      wxMilliSleep(1);
   }

   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   wxASSERT(mActive);

   // Make sure nothing is going on
   Suspend();

   // It is now safe to clean up
   mLatency = std::chrono::microseconds(0);

   // Process master list
   auto & states = RealtimeEffectList::Get(mProject).GetStates();
   for (auto & state : states)
   {
      state->Finalize();
   }

   // And all track lists
   for (auto leader : mGroupLeaders)
   {
      auto & states = RealtimeEffectList::Get(*leader).GetStates();
      for (auto &state : states)
      {
         state->Finalize();
      }
   }

   // Reset processor parameters
   mGroupLeaders.clear();
   mChans.clear();
   mRates.clear();

   // No longer active
   mActive = false;

   Resume();
}

int RealtimeEffectManager::GetLatency() const
{
   return mLatency.count();
}


void RealtimeEffectManager::Show(AudacityProject &project)
{
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   auto & list = RealtimeEffectList::Get(project);
   list.Show(this, XO("Master Effects"));
}

void RealtimeEffectManager::Show(Track &track, wxPoint pos)
{
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   auto & list = RealtimeEffectList::Get(track);
   list.Show(this, XO("%s Effects").Format(track.GetName()), pos);
}

bool RealtimeEffectManager::IsBypassed(const Track &track)
{
   return RealtimeEffectList::Get(track).IsBypassed();
}

void RealtimeEffectManager::Bypass(Track &track, bool bypass)
{
   RealtimeEffectList::Get(track).Bypass(bypass);
}

bool RealtimeEffectManager::HasPrefaders(int group)
{
   return RealtimeEffectList::Get(mProject).HasPrefaders() ||
          RealtimeEffectList::Get(*mGroupLeaders[group]).HasPrefaders();
}

bool RealtimeEffectManager::HasPostfaders(int group)
{
   return RealtimeEffectList::Get(mProject).HasPostfaders() ||
          RealtimeEffectList::Get(*mGroupLeaders[group]).HasPostfaders();
}

void RealtimeEffectManager::ProcessGroup(Track *leader, std::function<void(RealtimeEffectState &state, bool bypassed)> func)
{
   // Call the function for each effect on the master list
   RealtimeEffectList::Get(mProject).ProcessGroup(func);

   // Call the function for each effect on the track list
   RealtimeEffectList::Get(*leader).ProcessGroup(func);
}

RealtimeEffectState &RealtimeEffectManager::AddState(RealtimeEffectList &states, const PluginID & id)
{
   wxLogDebug(wxT("AddState"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   // Block processing
   Suspend();

   auto & state = states.AddState(id);
   
   if (mActive)
   {
      state.Initialize(mRate);

      for (auto &leader : mGroupLeaders)
      {
         auto chans = mChans[leader];
         auto rate = mRates[leader];

         state.AddProcessor(leader, chans, rate);
      }
   }

   if (mProcessing)
   {
      state.ProcessStart();
   }

   ProjectHistory::Get(mProject).PushState(
      XO("Added %s effect").Format(state.GetEffect()->GetName()),
      XO("Added Effect"),
      UndoPush::NONE
   );

   // Allow RealtimeProcess() to, well, process 
   Resume();

   return state;
}

void RealtimeEffectManager::RemoveState(RealtimeEffectList &states, RealtimeEffectState &state)
{
   wxLogDebug(wxT("RemoveState"));
   // Protect...
   std::lock_guard<std::mutex> guard(mLock);

   // Block RealtimeProcess()
   Suspend();

   ProjectHistory::Get(mProject).PushState(
      XO("Removed %s effect").Format(state.GetEffect()->GetName()),
      XO("Removed Effect"),
      UndoPush::NONE
   );

   if (mProcessing)
   {
      state.ProcessEnd();
   }

   if (mActive)
   {
      state.Finalize();
   }

   states.RemoveState(state);

   // Allow RealtimeProcess() to, well, process 
   Resume();
}

XMLTagHandler *RealtimeEffectManager::ReadXML(AudacityProject &project)
{
   return &RealtimeEffectList::Get(project);
}

XMLTagHandler *RealtimeEffectManager::ReadXML(Track &track)
{
   return &RealtimeEffectList::Get(track);
}

void RealtimeEffectManager::WriteXML(
   XMLWriter &xmlFile, const AudacityProject &project) const
{
   RealtimeEffectList::Get(project).WriteXML(xmlFile);
}

void RealtimeEffectManager::WriteXML(
   XMLWriter &xmlFile, const Track &track) const
{
   RealtimeEffectList::Get(track).WriteXML(xmlFile);
}

void RealtimeEffectManager::WriteXML(
   XMLWriter &xmlFile, const RealtimeEffectList &states) const
{
}

static const AttachedProjectObjects::RegisteredFactory manager
{
   [](AudacityProject &project)
   {
      return std::make_shared<RealtimeEffectManager>(project);
   }
};

RealtimeEffectManager &RealtimeEffectManager::Get(AudacityProject &project)
{
   return project.AttachedObjects::Get<RealtimeEffectManager&>(manager);
}

const RealtimeEffectManager &RealtimeEffectManager::Get(const AudacityProject &project)
{
   return Get(const_cast<AudacityProject &>(project));
}

#if 0
static ProjectFileIORegistry::WriterEntry entry {
[](const AudacityProject &project, XMLWriter &xmlFile){
   auto &realtimeEffectManager = RealtimeEffectManager::Get(project);
   realtimeEffectManager.WriteXML(xmlFile, project);
}
};
#endif
