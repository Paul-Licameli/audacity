/**********************************************************************

 Audacity: A Digital Audio Editor

 RealtimeEffectList.h

 *********************************************************************/

#ifndef __AUDACITY_REALTIMEEFFECTLIST_H__
#define __AUDACITY_REALTIMEEFFECTLIST_H__

#include <atomic>
#include <mutex>
#include <vector>

#include <wx/defs.h>

#include "ClientData.h"
#include "RealtimeEffectState.h"
#include "xml/XMLTagHandler.h"

class AudacityProject;
class EffectClientInterface;
class Effect;

class RealtimeEffectManager;
class RealtimeEffectUI;

class RealtimeEffectList final : public ClientData::Base,
                                 public std::enable_shared_from_this<RealtimeEffectList>,
                                 public XMLTagHandler
{
public:
   RealtimeEffectList(bool deleteUI);
   virtual ~RealtimeEffectList();

   static RealtimeEffectList &Get(AudacityProject &project);
   static const RealtimeEffectList &Get(const AudacityProject &project);

   static RealtimeEffectList &Get(Track &track);
   static const RealtimeEffectList &Get(const Track &track);

   bool IsBypassed() const;
   void Bypass(bool bypass);

   void Suspend();
   void Resume();

   void ProcessGroup(std::function<void(RealtimeEffectState &state, bool bypassed)> func);

   void SetPrefade(RealtimeEffectState &state, bool prefade);
   bool HasPrefaders();
   bool HasPostfaders();

   RealtimeEffectState & AddState(const PluginID &id);
   void RemoveState(RealtimeEffectState &state);
   void Swap(int index1, int index2);

   using States = std::vector<std::unique_ptr<RealtimeEffectState>>;
   States & GetStates();
   RealtimeEffectState &GetState(int index);

   void Show(RealtimeEffectManager *manager, const TranslatableString &title, wxPoint pos = wxDefaultPosition);

   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   void HandleXMLEndTag(const wxChar *tag) override;
   XMLTagHandler *HandleXMLChild(const wxChar *tag) override;
   void WriteXML(XMLWriter &xmlFile) const;

private:
   RealtimeEffectState *DoAdd(const PluginID &id = {});
private:
   States mStates;

   bool mBypass;
   int mSuspend;

   int mPrefaders;
   int mPostfaders;

   friend class RealtimeEffectUI;
   friend class RealtimeEffectManager;
};

#endif // __AUDACITY_REALTIMEEFFECTLIST_H__
