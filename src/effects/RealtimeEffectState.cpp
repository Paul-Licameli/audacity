/**********************************************************************
 
  Audacity: A Digital Audio Editor
 
  RealtimeEffectState.cpp
 
 *********************************************************************/

#include "RealtimeEffectState.h"

#include "RealtimeEffectList.h"
#include "RealtimeEffectManager.h"

#include "EffectInterface.h"
#include "MemoryX.h"

#include "../PluginManager.h"
#include "../Track.h"
#include "../xml/XMLWriter.h"
//#include "EffectUI.h"

#include <wx/time.h>

RealtimeEffectState::RealtimeEffectState()
{
   mEffect.reset();

   mInitialized = false;
   mPre = false;
   mBypass = false;
   mSuspendCount = 0;   // default to unsuspended
}

RealtimeEffectState::RealtimeEffectState(const PluginID & id)
:  RealtimeEffectState()
{
   SetID(id);
}

void RealtimeEffectState::SetID(const PluginID & id)
{
   mID = id;
}

EffectClientInterface *RealtimeEffectState::GetEffect()
{
   if (!mEffect)
   {
//      mEffect = EffectManager::Get().NewEffect(mID);
   }

   return mEffect.get();
}

bool RealtimeEffectState::IsPreFade()
{
   return mPre;
}

void RealtimeEffectState::PreFade(bool pre)
{
   mPre = pre;
}

bool RealtimeEffectState::IsActive() const
{
   return !mBypass && mSuspendCount == 0;
}

bool RealtimeEffectState::IsBypassed()
{
   return mBypass;
}

void RealtimeEffectState::Bypass(bool Bypass)
{
   mBypass = Bypass;
}

bool RealtimeEffectState::IsSuspended()
{
   return mSuspendCount > 0;
}

bool RealtimeEffectState::Suspend()
{
   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   auto result = effect->RealtimeSuspend();
   if (result)
   {
      mSuspendCount++;
   }

   return result;
}

bool RealtimeEffectState::Resume()
{
   wxASSERT(mSuspendCount >= 0);

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   auto result = effect->RealtimeResume();
   if (result)
   {
      mSuspendCount--;
   }

   return result;
}

bool RealtimeEffectState::Initialize(double rate)
{
   wxLogDebug(wxT("State Initialize"));

   mGroups.clear();

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   effect->SetSampleRate(rate);

   return effect->RealtimeInitialize();
}

// RealtimeAddProcessor and RealtimeProcess use the same method of
// determining the current processor index, so updates to one should
// be reflected in the other.
bool RealtimeEffectState::AddProcessor(Track *track, unsigned chans, float rate)
{
   wxLogDebug(wxT("State AddProcessor"));

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   auto ichans = chans;
   auto ochans = chans;
   auto gchans = chans;

   const auto numAudioIn = effect->GetAudioInCount();
   const auto numAudioOut = effect->GetAudioOutCount();

   auto group = mGroups.size();
   mGroups.insert({track, group});

   // Call the client until we run out of input or output channels
   while (ichans > 0 && ochans > 0)
   {
      // If we don't have enough input channels to accommodate the client's
      // requirements, then we replicate the input channels until the
      // client's needs are met.
      if (ichans < numAudioIn)
      {
         // All input channels have been consumed
         ichans = 0;
      }
      // Otherwise fulfill the client's needs with as many input channels as possible.
      // After calling the client with this set, we will loop back up to process more
      // of the input/output channels.
      else if (ichans >= numAudioIn)
      {
         gchans = numAudioIn;
         ichans -= gchans;
      }

      // If we don't have enough output channels to accommodate the client's
      // requirements, then we provide all of the output channels and fulfill
      // the client's needs with dummy buffers.  These will just get tossed.
      if (ochans < numAudioOut)
      {
         // All output channels have been consumed
         ochans = 0;
      }
      // Otherwise fulfill the client's needs with as many output channels as possible.
      // After calling the client with this set, we will loop back up to process more
      // of the input/output channels.
      else if (ochans >= numAudioOut)
      {
         ochans -= numAudioOut;
      }

      // Add a NEW processor
      effect->RealtimeAddProcessor(gchans, rate);
   }

   return true;
}

bool RealtimeEffectState::ProcessStart()
{
   wxLogDebug(wxT("State Start"));

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   return effect->RealtimeProcessStart();
}

// RealtimeAddProcessor and RealtimeProcess use the same method of
// determining the current processor group, so updates to one should
// be reflected in the other.
size_t RealtimeEffectState::Process(Track *track,
                                    unsigned chans,
                                    float **inbuf,
                                    float **outbuf,
                                    size_t numSamples)
{
   wxLogDebug(wxT("State Process"));

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   auto group = mGroups[track];

   // The caller passes the number of channels to process and specifies
   // the number of input and output buffers.  There will always be the
   // same number of output buffers as there are input buffers.
   //
   // Effects always require a certain number of input and output buffers,
   // so if the number of channels we're currently processing are different
   // than what the effect expects, then we use a few methods of satisfying
   // the effects requirements.
   const auto numAudioIn = effect->GetAudioInCount();
   const auto numAudioOut = effect->GetAudioOutCount();

   float **clientIn = (float **) alloca(numAudioIn * sizeof(float *));
   float **clientOut = (float **) alloca(numAudioOut * sizeof(float *));
   float *dummybuf = (float *) alloca(numSamples * sizeof(float));
   decltype(numSamples) len = 0;
   auto ichans = chans;
   auto ochans = chans;
   auto gchans = chans;
   unsigned indx = 0;
   unsigned ondx = 0;

   // Call the client until we run out of input or output channels
   while (ichans > 0 && ochans > 0)
   {
      // If we don't have enough input channels to accommodate the client's
      // requirements, then we replicate the input channels until the
      // client's needs are met.
      if (ichans < numAudioIn)
      {
         for (size_t i = 0; i < numAudioIn; i++)
         {
            if (indx == ichans)
            {
               indx = 0;
            }
            clientIn[i] = inbuf[indx++];
         }

         // All input channels have been consumed
         ichans = 0;
      }
      // Otherwise fulfill the client's needs with as many input channels as possible.
      // After calling the client with this set, we will loop back up to process more
      // of the input/output channels.
      else if (ichans >= numAudioIn)
      {
         gchans = 0;
         for (size_t i = 0; i < numAudioIn; i++, ichans--, gchans++)
         {
            clientIn[i] = inbuf[indx++];
         }
      }

      // If we don't have enough output channels to accommodate the client's
      // requirements, then we provide all of the output channels and fulfill
      // the client's needs with dummy buffers.  These will just get tossed.
      if (ochans < numAudioOut)
      {
         for (size_t i = 0; i < numAudioOut; i++)
         {
            if (i < ochans)
            {
               clientOut[i] = outbuf[i];
            }
            else
            {
               clientOut[i] = dummybuf;
            }
         }

         // All output channels have been consumed
         ochans = 0;
      }
      // Otherwise fulfill the client's needs with as many output channels as possible.
      // After calling the client with this set, we will loop back up to process more
      // of the input/output channels.
      else if (ochans >= numAudioOut)
      {
         for (size_t i = 0; i < numAudioOut; i++, ochans--)
         {
            clientOut[i] = outbuf[ondx++];
         }
      }

      // Finally call the plugin to process the block
      len = 0;
      const auto blockSize = effect->GetBlockSize();
      for (decltype(numSamples) block = 0; block < numSamples; block += blockSize)
      {
         auto cnt = std::min(numSamples - block, blockSize);
         len += effect->RealtimeProcess(group, clientIn, clientOut, cnt);

         for (size_t i = 0 ; i < numAudioIn; i++)
         {
            clientIn[i] += cnt;
         }

         for (size_t i = 0 ; i < numAudioOut; i++)
         {
            clientOut[i] += cnt;
         }
      }
   }

   return len;
}

bool RealtimeEffectState::ProcessEnd()
{
   wxLogDebug(wxT("State End"));

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   return effect->RealtimeProcessEnd();
}

bool RealtimeEffectState::Finalize()
{
   wxLogDebug(wxT("State Finalize"));

   mGroups.clear();

   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   return effect->RealtimeFinalize();
}

bool RealtimeEffectState::ShowEditor(wxWindow *parent)
{
   auto effect = GetEffect();
   if (!effect)
   {
      return false;
   }

   return effect->ShowInterface(*parent, EffectUI::DialogFactory, false);
}

void RealtimeEffectState::CloseEditor()
{
   auto effect = GetEffect();
   if (!effect)
   {
      return;
   }

   effect->CloseInterface();
}

bool RealtimeEffectState::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (!wxStrcmp(tag, wxT("effect")))
   {
      mParameters.clear();
      mEffect.reset();
      mID.clear();

      while(*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         const wxString strValue = value;
         if (!wxStrcmp(attr, wxT("id")))
         {
            mID = value;

            auto effect = GetEffect();
            if (!effect)
            {
               // TODO - complain!!!!
            }
         }
         else if (!wxStrcmp(attr, wxT("version")))
         {
         }
         else if (!wxStrcmp(attr, wxT("Bypass")))
         {
            long nValue;
            if (XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            {
               Bypass(nValue != 0);
            }
         }
         else if (!wxStrcmp(attr, wxT("prefader")))
         {
            long nValue;
            if (XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            {
              PreFade(nValue != 0);
            }
         }
      }

      return true;
   }

   if (!wxStrcmp(tag, wxT("parameters")))
   {
      return true;
   }

   if (!wxStrcmp(tag, wxT("parameter")))
   {
      wxString n;
      wxString v;

      while(*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         if (!wxStrcmp(attr, wxT("name")))
         {
            n = value;
         }
         else if (!wxStrcmp(attr, wxT("value")))
         {
            v = value;
         }
      }

      mParameters += wxString::Format(wxT("\"%s=%s\" "), n, v);

      return true;
   }

   return false;
}

void RealtimeEffectState::HandleXMLEndTag(const wxChar *tag)
{
   if (!wxStrcmp(tag, wxT("effect")))
   {
      auto effect = GetEffect();
      if (effect && !mParameters.empty())
      {
         CommandParameters parms(mParameters);
         effect->SetAutomationParameters(parms);
      }
   }
}

XMLTagHandler *RealtimeEffectState::HandleXMLChild(const wxChar *tag)
{
   return this;
}

void RealtimeEffectState::WriteXML(XMLWriter &xmlFile)
{
   auto effect = GetEffect();
   if (!effect)
   {
      return;
   }

   xmlFile.StartTag(wxT("effect"));
   xmlFile.WriteAttr(wxT("id"), XMLWriter::XMLEsc(PluginManager::GetID(effect)));
   xmlFile.WriteAttr(wxT("version"), XMLWriter::XMLEsc(mEffect->GetVersion()));
   xmlFile.WriteAttr(wxT("Bypass"), mBypass);
   xmlFile.WriteAttr(wxT("prefader"), mPre);
      
   CommandParameters cmdParms;
   if (effect->GetAutomationParameters(cmdParms))
   {
      xmlFile.StartTag(wxT("parameters"));

      wxString entryName;
      long entryIndex;
      bool entryKeepGoing;

      entryKeepGoing = cmdParms.GetFirstEntry(entryName, entryIndex);
      while (entryKeepGoing)
      {
         wxString entryValue = cmdParms.Read(entryName, "");

         xmlFile.StartTag(wxT("parameter"));
         xmlFile.WriteAttr(wxT("name"), XMLWriter::XMLEsc(entryName));
         xmlFile.WriteAttr(wxT("value"), XMLWriter::XMLEsc(entryValue));
         xmlFile.EndTag(wxT("parameter"));

         entryKeepGoing = cmdParms.GetNextEntry(entryName, entryIndex);
      }

      xmlFile.EndTag(wxT("parameters"));
   }

   xmlFile.EndTag(wxT("effect"));
}
