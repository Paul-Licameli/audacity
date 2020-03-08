/**********************************************************************

  Audacity: A Digital Audio Editor

  ThemePrefs.h

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#ifndef __AUDACITY_THEME_PREFS__
#define __AUDACITY_THEME_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

class ShuttleGui;

// An event sent to the application when the user changes choice of theme
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_THEME_CHANGE, wxCommandEvent);

#define THEME_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Theme") }

class ThemePrefs final : public PrefsPanel
{
 public:
   ThemePrefs(wxWindow * parent, wxWindowID winid);
   ~ThemePrefs(void);
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   wxString HelpPageName() override;

   static void ApplyUpdatedImages();

 private:
   void PopulateOrExchange(ShuttleGui & S) override;
   void OnLoadThemeComponents();
   void OnSaveThemeComponents();
   void OnLoadThemeCache();
   void OnSaveThemeCache();
   void OnReadThemeInternal();
   void OnSaveThemeAsCode();
};

#endif
