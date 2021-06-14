/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 RealtimeEffectManager.h
 
 **********************************************************************/

#ifndef __AUDACITY_REALTIME_EFFECT_MANAGER__
#define __AUDACITY_REALTIME_EFFECT_MANAGER__

#include <chrono>
#include <map>
#include <memory>
#include <vector>
#include <wx/thread.h>

#include "RealtimeEffectList.h"
#include "RealtimeEffectState.h"

class EffectClientInterface;

class AUDACITY_DLL_API RealtimeEffectManager final : public ClientData::Base,
                                                     public std::enable_shared_from_this<RealtimeEffectManager>
{
public:
   RealtimeEffectManager(AudacityProject &project);
   ~RealtimeEffectManager();

   bool IsBypassed(const Track &track);
   void Bypass(Track &track, bool bypass);

   bool HasPrefaders(int group);
   bool HasPostfaders(int group);

   XMLTagHandler *ReadXML(AudacityProject &project);
   XMLTagHandler *ReadXML(Track &track);
   void WriteXML(XMLWriter &xmlFile, const AudacityProject &project) const;
   void WriteXML(XMLWriter &xmlFile, const Track &track) const;

   bool IsActive() const;
   bool IsSuspended() const;
   void Suspend();
   void Resume();

   void Initialize(double rate);
   void AddProcessor(Track *track, unsigned chans, float rate);
   void ProcessStart();
   size_t Process(Track *track, float gain, float **buffers, size_t numSamps);
   void ProcessEnd();
   void Finalize();

   int GetLatency() const;

   void Show(AudacityProject &project);
   void Show(Track &track, wxPoint pos);

   static RealtimeEffectManager &Get(AudacityProject &project);
   static const RealtimeEffectManager &Get(const AudacityProject &project);

private:
   void ProcessGroup(Track *leader, std::function<void(RealtimeEffectState &state, bool bypassed)> func);

   RealtimeEffectState & AddState(RealtimeEffectList &states, const PluginID & id);
   void RemoveState(RealtimeEffectList &states, RealtimeEffectState &state);

   void WriteXML(XMLWriter &xmlFile, const RealtimeEffectList &states) const;

private:
   AudacityProject &mProject;

   std::mutex mLock;
   std::chrono::microseconds mLatency;

   double mRate;

   std::atomic<bool> mSuspended;
   std::atomic<bool> mActive;
   std::atomic<bool> mProcessing;

   std::vector<Track *> mGroupLeaders;
   std::unordered_map<Track *, int> mChans;
   std::unordered_map<Track *, double> mRates;
   int mCurrentGroup;

   std::vector<EffectClientInterface *> mInitialized;

   friend class RealtimeEffectList;
   friend class RealtimeEffectState;
   friend class RealtimeEffectUI;
};

#endif
