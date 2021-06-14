/**********************************************************************
 
  Audacity: A Digital Audio Editor
 
  RealtimeEffectList.cpp
 
 *********************************************************************/

#include "RealtimeEffectList.h"

#include "RealtimeEffectManager.h"
#include "RealtimeEffectState.h"

#include "EffectInterface.h"

#include "../PluginManager.h"
#include "../Project.h"
#include "../Track.h"
#include "../xml/XMLWriter.h"
#include "EffectUI.h"

#include <wx/time.h>

RealtimeEffectList::RealtimeEffectList(bool deleteUI)
{
   mBypass = false;
   mSuspend = 0;
   mPrefaders = 0;
   mPostfaders = 0;
}

RealtimeEffectList::~RealtimeEffectList()
{
}

static const AttachedProjectObjects::RegisteredFactory masterEffects
{
   [](AudacityProject &project)
   {
      return std::make_shared<RealtimeEffectList>(false);
   }
};

RealtimeEffectList &RealtimeEffectList::Get(AudacityProject &project)
{
   return project.AttachedObjects::Get<RealtimeEffectList>(masterEffects);
}

const RealtimeEffectList &RealtimeEffectList::Get(const AudacityProject &project)
{
   return Get(const_cast<AudacityProject &>(project));
}

static const AttachedTrackObjects::RegisteredFactory trackEffects
{
   [](Track &track)
   {
      return std::make_shared<RealtimeEffectList>(true);
   }
};

RealtimeEffectList &RealtimeEffectList::Get(Track &track)
{
   return track.AttachedObjects::Get<RealtimeEffectList>(trackEffects);
}

const RealtimeEffectList &RealtimeEffectList::Get(const Track &track)
{
   return Get(const_cast<Track &>(track));
}

bool RealtimeEffectList::IsBypassed() const
{
   return mBypass;
}

void RealtimeEffectList::Bypass(bool bypass)
{
   mBypass = bypass;
}

void RealtimeEffectList::Suspend()
{
   mSuspend++;
}

void RealtimeEffectList::Resume()
{
   mSuspend--;
}

void RealtimeEffectList::ProcessGroup(std::function<void(RealtimeEffectState &state, bool bypassed)> func)
{
   for (auto &state : mStates)
   {
      func(*state, IsBypassed() || !state->IsActive());
   }
}

void RealtimeEffectList::SetPrefade(RealtimeEffectState &state, bool prefade)
{
   state.PreFade(prefade);

   if (state.IsPreFade())
   {
      mPrefaders++;
      mPostfaders--;
   }
   else
   {
      mPostfaders++;
      mPrefaders--;
   }
}

bool RealtimeEffectList::HasPrefaders()
{
   return mPrefaders;
}

bool RealtimeEffectList::HasPostfaders()
{
   return mPostfaders;
}

RealtimeEffectState & RealtimeEffectList::AddState(const PluginID & id)
{
   return *DoAdd(id);
}

void RealtimeEffectList::RemoveState(RealtimeEffectState &state)
{
   auto end = mStates.end();
   auto found = std::find_if(mStates.begin(), end,
      [&](const std::unique_ptr<RealtimeEffectState> &item)
      {
         return item.get() == &state;
      }
   );

   if (found != end)
   {
      mStates.erase(found);
   }
}

void RealtimeEffectList::Swap(int index1, int index2)
{
   std::swap(mStates[index1], mStates[index2]);
}

RealtimeEffectList::States & RealtimeEffectList::GetStates()
{
   return mStates;
}

RealtimeEffectState &RealtimeEffectList::GetState(int index)
{
   return *mStates[index];
}

void RealtimeEffectList::Show(RealtimeEffectManager *manager,
                              const TranslatableString &title,
                              wxPoint pos /* = wxDefaultPosition */)
{
   bool existed = (mUI != nullptr);
   if (!existed)
   {
      mUI = safenew RealtimeEffectUI(manager, title, *this);
   }

   if (mUI)
   {
      if (!existed)
      {
         mUI->CenterOnParent();
      }

      mUI->Show();
      if (!existed && pos != wxDefaultPosition)
      {
         mUI->Move(pos);
      }
   }
}

bool RealtimeEffectList::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (!wxStrcmp(tag, wxT("effects")))
   {
      mBypass = false;

      while(*attrs)
      {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;

         if (!value)
         {
            break;
         }

         const wxString strValue = value;
         if (!wxStrcmp(attr, wxT("bypass")))
         {
            long nValue;
            if (XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            {
               Bypass(nValue != 0);
            }
         }
      }

      return true;
   }

   return false;
}

void RealtimeEffectList::HandleXMLEndTag(const wxChar *tag)
{
   if (!wxStrcmp(tag, wxT("effects")))
   {
      // This shouldn't really be needed
      if (mUI)
      {
         mUI->Rebuild();
      }
   }
}

XMLTagHandler *RealtimeEffectList::HandleXMLChild(const wxChar *tag)
{
   if (!wxStrcmp(tag, wxT("effect")))
   {
      return DoAdd();
   }

   return nullptr;
}

void RealtimeEffectList::WriteXML(XMLWriter &xmlFile) const
{
   if (mStates.size() == 0)
   {
      return;
   }

   xmlFile.StartTag(wxT("effects"));
   xmlFile.WriteAttr(wxT("bypass"), mBypass);

   for (const auto & state : mStates)
   {
      state->WriteXML(xmlFile);
   }
   
   xmlFile.EndTag(wxT("effects"));
}


RealtimeEffectState *RealtimeEffectList::DoAdd(const PluginID &id)
{
   mStates.emplace_back(std::make_unique<RealtimeEffectState>(id));
   auto & state = mStates.back();

   if (state->IsPreFade())
   {
      mPrefaders++;
   }
   else
   {
      mPostfaders++;
   }

   return state.get();
}
