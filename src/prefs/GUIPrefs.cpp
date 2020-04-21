/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class GUIPrefs
\brief A PrefsPanel for general GUI preferences.

*//*******************************************************************/


#include "GUIPrefs.h"

#include <wx/app.h>
#include <wx/defs.h>

#include "../FileNames.h"
#include "Languages.h"
#include "../Theme.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

#include "ThemePrefs.h"
#include "../AColor.h"
#include "../widgets/AudacityMessageBox.h"

GUIPrefs::GUIPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: refers to Audacity's user interface settings */
:  PrefsPanel(parent, winid, XC("Interface", "GUI"))
{
}

GUIPrefs::~GUIPrefs()
{
}

ComponentInterfaceSymbol GUIPrefs::GetSymbol()
{
   return GUI_PREFS_PLUGIN_SYMBOL;
}

TranslatableString GUIPrefs::GetDescription()
{
   return XO("Preferences for GUI");
}

wxString GUIPrefs::HelpPageName()
{
   return "Interface_Preferences";
}

void GUIPrefs::GetRangeChoices(
   TranslatableStrings *pChoices,
   std::vector<int> *pCodes )
{
   static const std::vector<int> sCodes =
      { 36, 48, 60, 72, 84, 96, 120, 145 };
   if (pCodes)
      *pCodes = sCodes;

   static const std::initializer_list<TranslatableString> sChoices = {
      XO("-36 dB (shallow range for high-amplitude editing)") ,
      XO("-48 dB (PCM range of 8 bit samples)") ,
      XO("-60 dB (PCM range of 10 bit samples)") ,
      XO("-72 dB (PCM range of 12 bit samples)") ,
      XO("-84 dB (PCM range of 14 bit samples)") ,
      XO("-96 dB (PCM range of 16 bit samples)") ,
      XO("-120 dB (approximate limit of human hearing)") ,
      XO("-145 dB (PCM range of 24 bit samples)") ,
   };

   if (pChoices)
      *pChoices = sChoices;
}

ChoiceSetting GUIManualLocation{
   L"/GUI/Help",
   {
      ByColumns,
      { XO("Local") ,  XO("From Internet") , },
      { L"Local" , L"FromInternet" , }
   },
   0 // "Local"
};

void GUIPrefs::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   
   TranslatableStrings langNames;
   Identifiers langCodes;
   Languages::GetLanguages(
      FileNames::AudacityPathList(), langCodes, langNames);

   TranslatableStrings rangeChoices;
   std::vector<int> rangeCodes;
   GetRangeChoices( &rangeChoices, &rangeCodes );

#if 0
   langCodes.insert( langCodes.end(), {
      // only for testing...
      "kg" ,
      "ep" ,
   } );

   langNames.insert( langNames.end(), {
      "Klingon" ,
      "Esperanto" ,
   } );
#endif

   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("Display"));
   {
      S.StartMultiColumn(2);
      {

         S
            .Target( Choice( LocaleLanguage, langNames, langCodes ) )
            .AddChoice( XXO("&Language:") );

         S
            .Target( GUIManualLocation )
            .AddChoice( XXO("Location of &Manual:") );

         S
            .Target( GUITheme )
            .AddChoice( XXO("Th&eme:") );

         S
            .Target( NumberChoice( GUIdBRange, rangeChoices, rangeCodes ) )
            .AddChoice( XXO("Meter dB &range:") );
      }
      S.EndMultiColumn();
//      S.AddSpace(10);
// JKC: This is a silly preference.  Kept here as a reminder that we may
// later want to have configurable button order.
//      S
//         .Target( GUIErgonomicTransportButtons )
//         .AddCheckBox( XXO("&Ergonomic order of Transport Toolbar buttons") );

   }
   S.EndStatic();

   S.StartStatic(XO("Options"));
   {
      // Start wording of options with a verb, if possible.
      S
         .Target( GUIShowSplashScreen )
         .AddCheckBox( XXO("Show 'How to Get &Help' at launch") );

      S
         .Target( GUIShowExtraMenus )
         .AddCheckBox( XXO("Show e&xtra menus") );

#ifdef EXPERIMENTAL_THEME_PREFS
      // We do not want to make this option mainstream.  It's a 
      // convenience for developers.
      S
          .Target( GUIShowMac )
         .AddCheckBox( XXO("Show alternative &styling (Mac vs PC)") );
#endif

      S
         .Target( GUIBeepOnCompletion )
         .AddCheckBox( XXO("&Beep on completion of longer activities") );

      S
         .Target( GUIRetainLabels )
         .AddCheckBox( XXO("Re&tain labels if selection snaps to a label") );

      S
         .Target( GUIBlendThemes )
         .AddCheckBox( XXO("B&lend system and Audacity theme") );

#ifndef __WXMAC__
      /* i18n-hint: RTL stands for 'Right to Left'  */
      S
         .Target( GUIRtlWorkaround )
         .AddCheckBox( XXO("Use mostly Left-to-Right layouts in RTL languages") );
#endif
#ifdef EXPERIMENTAL_CEE_NUMBERS_OPTION
      S.TieCheckBox(XXO("Never use comma as decimal point"),
                    {L"/Locale/CeeNumberFormat",
                     false});
#endif
   }
   S.EndStatic();

   S.StartStatic(XO("Timeline"));
   {
      S
         .Target( QuickPlayToolTips )
         .AddCheckBox( XXO("Show Timeline Tooltips") );
      S
         .Target( QuickPlayScrubbingEnabled )
         .AddCheckBox( XXO("Show Scrub Ruler") );
   }
   S.EndStatic();

   S.EndScroller();
}

bool GUIPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   // If language has changed, we want to change it now, not on the next reboot.
   Identifier lang = LocaleLanguage.Read();
   auto usedLang = SetLang(lang);
   // Bug 1523: Previously didn't check no-language (=System Language)
   if (!(lang.empty() || lang == L"System") && (lang != usedLang)) {
      // lang was not usable and is not system language.  We got overridden.
      LocaleLanguage.Write( usedLang.GET() );
      gPrefs->Flush();
   }

   // Reads preference GUITheme
   theTheme.LoadPreferredTheme();
   ThemePrefs::ApplyUpdatedImages();

   return true;
}

Identifier GUIPrefs::SetLang( const Identifier & lang )
{
   auto result = Languages::SetLang(FileNames::AudacityPathList(), lang);
   if (!(lang.empty() || lang == L"System") && result != lang)
      ::AudacityMessageBox(
         XO("Language \"%s\" is unknown").Format( lang.GET() ) );

#ifdef EXPERIMENTAL_CEE_NUMBERS_OPTION
   bool forceCeeNumbers;
   gPrefs->Read(L"/Locale/CeeNumberFormat", &forceCeeNumbers, false);
   if( forceCeeNumbers )
      Internat::SetCeeNumberFormat();
#endif

#ifdef __WXMAC__
      wxApp::s_macHelpMenuTitleName = _("&Help");
#endif

   return result;
}

int ShowClippingPrefsID()
{
   static int value = wxNewId();
   return value;
}

int ShowTrackNameInWaveformPrefsID()
{
   static int value = wxNewId();
   return value;
}

namespace{
PrefsPanel::Registration sAttachment{ "GUI",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew GUIPrefs(parent, winid);
   }
};
}

void RTL_WORKAROUND( wxWindow *pWnd )
{
#ifndef __WXMAC__
   if ( GUIRtlWorkaround.Read() )
      pWnd->SetLayoutDirection(wxLayout_LeftToRight);
#else
   (void)pWnd;
#endif
}

BoolSetting GUIErgonomicTransportButtons{
   L"/GUI/ErgonomicTransportButtons", true
};
BoolSetting GUIBeepOnCompletion{
   L"/GUI/BeepOnCompletion", false };
BoolSetting GUIBlendThemes{
   L"/GUI/BlendThemes",      true  };
BoolSetting GUIRetainLabels{
   L"/GUI/RetainLabels",     false };
BoolSetting GUIRtlWorkaround{
   L"/GUI/RtlWorkaround",    true  };
BoolSetting GUIShowExtraMenus{
   L"/GUI/ShowExtraMenus",   false };
BoolSetting GUIShowMac{
   L"/GUI/ShowMac",          false };
BoolSetting GUIShowSplashScreen{
   L"/GUI/ShowSplashScreen", true  };

// A dB value as a positivie number
IntSetting GUIdBRange{
   L"/GUI/EnvdBRange",       60 };

StringSetting LocaleLanguage{
   L"/Locale/Language",      L"" };

BoolSetting QuickPlayScrubbingEnabled{
   L"/QuickPlay/ScrubbingEnabled",   false};
BoolSetting QuickPlayToolTips{
   L"/QuickPlay/ToolTips",   true};

