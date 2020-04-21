/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.h

  Brian Gunlogson
  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_GUI_PREFS__
#define __AUDACITY_GUI_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

class BoolSetting;
class ChoiceSetting;
class ShuttleGui;

#define GUI_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("GUI") }

class AUDACITY_DLL_API GUIPrefs final : public PrefsPanel
{
 public:
   GUIPrefs(wxWindow * parent, wxWindowID winid);
   ~GUIPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   wxString HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

   static void GetRangeChoices(
      TranslatableStrings *pChoices,
      std::vector<int> *pCodes );

   // If no input language given, defaults to system language.
   // Returns the language actually used which is not lang if lang cannot be found.
   static Identifier SetLang( const Identifier & lang );
};

AUDACITY_DLL_API
int ShowClippingPrefsID();
AUDACITY_DLL_API
int ShowTrackNameInWaveformPrefsID();

extern AUDACITY_DLL_API BoolSetting
     GUIErgonomicTransportButtons
   , GUIBeepOnCompletion
   , GUIBlendThemes
   , GUIRetainLabels
   , GUIRtlWorkaround
   , GUIShowExtraMenus
   , GUIShowMac
   , GUIShowSplashScreen
;

extern AUDACITY_DLL_API ChoiceSetting
     GUIManualLocation
;

extern AUDACITY_DLL_API IntSetting
     GUIdBRange
;

extern AUDACITY_DLL_API StringSetting
     LocaleLanguage
;

extern AUDACITY_DLL_API BoolSetting
     QuickPlayScrubbingEnabled
   , QuickPlayToolTips
;

// Right to left languages fail in many wx3 dialogs with missing buttons.
// The workaround is to use LTR in those dialogs.
void RTL_WORKAROUND( wxWindow *pWnd );

#endif
