/**********************************************************************

  Audacity: A Digital Audio Editor

  ThemePrefs.cpp

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

********************************************************************//**

\class ThemePrefs
\brief A PrefsPanel that configures dynamic loading of Theme
icons and colours.

Provides:
 - Button to save current theme as a single png image.
 - Button to load theme from a single png image.
 - Button to save current theme to multiple png images.
 - Button to load theme from multiple png images.
 - (Optional) Button to save theme as Cee data.
 - Button to read theme from default values in program.
 - CheckBox for loading custom themes at startup.

\see \ref Themability

*//********************************************************************/


#include "ThemePrefs.h"

#include <wx/app.h>
#include "../Prefs.h"
#include "../Theme.h"
#include "../ShuttleGui.h"
#include "../AColor.h"

wxDEFINE_EVENT(EVT_THEME_CHANGE, wxCommandEvent);

ThemePrefs::ThemePrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: A theme is a consistent visual style across an application's
 graphical user interface, including choices of colors, and similarity of images
 such as those on button controls.  Audacity can load and save alternative
 themes. */
:  PrefsPanel(parent, winid, XO("Theme"))
{
}

ThemePrefs::~ThemePrefs(void)
{
}

ComponentInterfaceSymbol ThemePrefs::GetSymbol()
{
   return THEME_PREFS_PLUGIN_SYMBOL;
}

TranslatableString ThemePrefs::GetDescription()
{
   return XO("Preferences for Theme");
}

wxString ThemePrefs::HelpPageName()
{
   return "Theme_Preferences";
}

/// Create the dialog contents, or exchange data with it.
void ThemePrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S
      .StartStatic(XO("Info"));
   {
      S
         .AddFixedText( XO(
"Themability is an experimental feature.\n\nTo try it out, click \"Save Theme Cache\" then find and modify the images and colors in\nImageCacheVxx.png using an image editor such as the Gimp.\n\nClick \"Load Theme Cache\" to load the changed images and colors back into Audacity.\n\n(Only the Transport Toolbar and the colors on the wavetrack are currently affected, even\nthough the image file shows other icons too.)") );

#ifdef _DEBUG
      S
         .AddFixedText( Verbatim(
"This is a debug version of Audacity, with an extra button, 'Output Sourcery'. This will save a\nC version of the image cache that can be compiled in as a default.") );
#endif

      S
         .AddFixedText( XO(
"Saving and loading individual theme files uses a separate file for each image, but is\notherwise the same idea.") );
   }
   S.EndStatic();

   /* i18n-hint: && in here is an escape character to get a single & on screen,
    * so keep it as is */
   S
      .StartStatic( XO("Theme Cache - Images && Color") );
   {
      S.StartHorizontalLay(wxALIGN_LEFT);
      {
         S
            .Action( [this]{ OnSaveThemeCache(); } )
            .AddButton(XXO("Save Theme Cache"));

         S
            .Action( [this]{ OnLoadThemeCache(); } )
            .AddButton(XXO("Load Theme Cache"));

         // This next button is only provided in Debug mode.
         // It is for developers who are compiling Audacity themselves
         // and who wish to generate a NEW ThemeAsCeeCode.h and compile it in.
#ifdef _DEBUG
         S
            .Action( [this]{ OnSaveThemeAsCode(); } )
            .AddButton( VerbatimLabel("Output Sourcery") );
#endif

         S
            .Action( [this]{ OnReadThemeInternal(); } )
            .AddButton(XXO("&Defaults"));
      }
      S.EndHorizontalLay();
   }
   S.EndStatic();

   // JKC: 'Ergonomic' details:
   // Theme components are used much less frequently than
   // the ImageCache.  Yet it's easy to click them 'by mistake'.
   //
   // To reduce that risk, we use a separate box to separate them off.
   // And choose text on the buttons that is shorter, making the
   // buttons smaller and less tempting to click.
   S
      .StartStatic( XO("Individual Theme Files"),1);
   {
      S.StartHorizontalLay(wxALIGN_LEFT);
      {
         S
            .Action( [this]{ OnSaveThemeComponents(); } )
            .AddButton( XXO("Save Files"));

         S
            .Action( [this]{ OnLoadThemeComponents(); } )
            .AddButton( XXO("Load Files"));
      }
      S.EndHorizontalLay();
   }
   S.EndStatic();
   S.EndScroller();

}

/// Load Theme from multiple png files.
void ThemePrefs::OnLoadThemeComponents()
{
   theTheme.LoadComponents();
   ApplyUpdatedImages();
}

/// Save Theme to multiple png files.
void ThemePrefs::OnSaveThemeComponents()
{
   theTheme.SaveComponents();
}

/// Load Theme from single png file.
void ThemePrefs::OnLoadThemeCache()
{
   theTheme.ReadImageCache();
   ApplyUpdatedImages();
}

/// Save Theme to single png file.
void ThemePrefs::OnSaveThemeCache()
{
   theTheme.CreateImageCache();
   theTheme.WriteImageMap();// bonus - give them the html version.
}

/// Read Theme from internal storage.
void ThemePrefs::OnReadThemeInternal()
{
   theTheme.ReadImageCache( theTheme.GetFallbackThemeType() );
   ApplyUpdatedImages();
}

/// Save Theme as C source code.
void ThemePrefs::OnSaveThemeAsCode()
{
   theTheme.SaveThemeAsCode();
   theTheme.WriteImageDefs();// bonus - give them the Defs too.
}

void ThemePrefs::ApplyUpdatedImages()
{
   AColor::ReInit();

   wxCommandEvent e{ EVT_THEME_CHANGE };
   wxTheApp->SafelyProcessEvent( e );
}

/// Update the preferences stored on disk.
bool ThemePrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

#ifdef EXPERIMENTAL_THEME_PREFS
namespace{
PrefsPanel::Registration sAttachment{ "Theme",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew ThemePrefs(parent, winid);
   },
   false,
   // Register with an explicit ordering hint because this one is
   // only conditionally compiled
   { "", { Registry::OrderingHint::After, "Effects" } }
};
}
#endif
