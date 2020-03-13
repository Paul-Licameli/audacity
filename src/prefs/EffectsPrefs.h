/**********************************************************************

  Audacity: A Digital Audio Editor

  EffectsPrefs.h

  Brian Gunlogson
  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_EFFECTS_PREFS__
#define __AUDACITY_EFFECTS_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

class ChoiceSetting;
class ShuttleGui;

#define EFFECTS_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Effects") }

class EffectsPrefs final : public PrefsPanel
{
 public:
   EffectsPrefs(wxWindow * parent, wxWindowID winid);
   ~EffectsPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   wxString HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;
};

extern AUDACITY_DLL_API BoolSetting
     PluginsCheckForUpdates
   , PluginsRescan
;

extern AUDACITY_DLL_API BoolSetting
     SSEGUI
;

extern AUDACITY_DLL_API ChoiceSetting EffectsGroupBy;
extern AUDACITY_DLL_API IntSetting EffectsMaxPerGroup;

#endif
