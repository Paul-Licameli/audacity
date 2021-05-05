/**********************************************************************

 Audacity: A Digital Audio Editor

 RealtimeEffectState.h

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTSTATE_H__
#define __AUDACITY_REALTIMEEFFECTSTATE_H__

#include <atomic>
#include <unordered_map>
#include <mutex>
#include <vector>

#include <wx/defs.h>

#include "audacity/Types.h"
#include "xml/XMLTagHandler.h"

class EffectClientInterface;
class Effect;
class RealtimeEffectUI;
class Track;

class RealtimeEffectState : public XMLTagHandler
{  
public:
   explicit RealtimeEffectState();
   explicit RealtimeEffectState(const PluginID & id);

   void SetID(const PluginID & id);
   EffectClientInterface *GetEffect();

   bool IsPreFade();
   void PreFade(bool pre);

   bool IsActive() const;

   bool IsBypassed();
   void Bypass(bool Bypass);

   bool IsSuspended();
   bool Suspend();
   bool Resume();

   bool Initialize(double rate);
   bool AddProcessor(Track *track, unsigned chans, float rate);
   bool ProcessStart();
   size_t Process(Track *track, unsigned chans, float **inbuf,  float **outbuf, size_t numSamples);
   bool ProcessEnd();
   bool Finalize();

   bool ShowEditor(wxWindow *parent);
   void CloseEditor();

   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   void HandleXMLEndTag(const wxChar *tag) override;
   XMLTagHandler *HandleXMLChild(const wxChar *tag) override;
   void WriteXML(XMLWriter &xmlFile);

private:
   PluginID mID;
   std::unique_ptr<EffectClientInterface> mEffect;
   bool mInitialized;

   std::unordered_map<Track *, int> mGroups;

   bool mBypass;

   bool mPre;

   std::mutex mMutex;

   wxString mParameters;

   std::atomic<int> mSuspendCount{ 1 };    // Effects are initially suspended

   friend class RealtimeEffectList;
   friend class RealtimeEffectUI;
   friend class RealtimeEffectManager;
};

#endif // __AUDACITY_REALTIMEEFFECTSTATE_H__
