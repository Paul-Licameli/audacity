/**********************************************************************

  Audacity: A Digital Audio Editor

  LibraryPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class LibraryPrefs
\brief A PrefsPanel used to select manage external libraries like the
MP3 and FFmpeg encoding libraries.

*//*******************************************************************/


#include "LibraryPrefs.h"

#include <wx/defs.h>
#include <wx/stattext.h>

#include "../FFmpeg.h"
#include "../export/ExportMP3.h"
#include "../widgets/HelpSystem.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/ReadOnlyText.h"
#include "../widgets/wxTextCtrlWrapper.h"


////////////////////////////////////////////////////////////////////////////////

LibraryPrefs::LibraryPrefs(wxWindow * parent, wxWindowID winid)
/* i18-hint: refers to optional plug-in software libraries */
:   PrefsPanel(parent, winid, XO("Libraries"))
{
}

LibraryPrefs::~LibraryPrefs()
{
}

ComponentInterfaceSymbol LibraryPrefs::GetSymbol()
{
   return LIBRARY_PREFS_PLUGIN_SYMBOL;
}

TranslatableString LibraryPrefs::GetDescription()
{
   return XO("Preferences for Library");
}

wxString LibraryPrefs::HelpPageName()
{
   return "Libraries_Preferences";
}

bool LibraryPrefs::TransferDataToWindow()
{
   // Set the MP3 Version string.
   SetMP3VersionText();
   SetFFmpegVersionText();
   return true;
}

/// This PopulateOrExchange function is a good example of mixing the fully
/// automatic style of reading/writing from GUI to prefs with the partial form.
///
/// You'll notice that some of the Tie functions have Prefs identifiers in them
/// and others don't.
void LibraryPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("LAME MP3 Export Library"));
   {
      S.StartTwoColumn();
      {
         mMP3Version = S
            .Position(wxALIGN_CENTRE_VERTICAL)
            .AddReadOnlyText(XXO("MP3 Library Version:"), "");
      }
      S.EndTwoColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("FFmpeg Import/Export Library"));
   {
      S.StartTwoColumn();
      {
         auto version =
#if defined(USE_FFMPEG)
            XO("No compatible FFmpeg library was found");
#else
            XO("FFmpeg support is not compiled in");
#endif

         mFFmpegVersion = S
            .Position(wxALIGN_CENTRE_VERTICAL)
            .AddReadOnlyText(XXO("FFmpeg Library Version:"), version.Translation());

         S
            .AddVariableText(XO("FFmpeg Library:"),
               true, wxALL | wxALIGN_RIGHT | wxALIGN_CENTRE_VERTICAL);

         S
#if !defined(USE_FFMPEG) || defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
            .Disable()
#endif
            .Action([this]{ OnFFmpegFindButton(); } )
            .AddButton(XXO("Loca&te..."),
                       wxALL | wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);

         S
#if !defined(USE_FFMPEG) || defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
            .Disable()
#endif
            .Action([this]{ OnFFmpegDownButton(); } )
            .AddButton(XXO("Dow&nload"),
               wxALL | wxALIGN_LEFT | wxALIGN_CENTRE_VERTICAL);
      }
      S.EndTwoColumn();
   }
   S.EndStatic();
   S.EndScroller();

}

/// Sets the a text area on the dialog to have the name
/// of the MP3 Library version.
void LibraryPrefs::SetMP3VersionText(bool prompt)
{
   mMP3Version->SetValue(GetMP3Version(this, prompt));
}

/// Opens a file-finder dialog so that the user can
/// tell us where the MP3 library is.
void LibraryPrefs::OnMP3FindButton()
{
   SetMP3VersionText(true);
}

/// Opens help on downloading a suitable MP3 library is.
void LibraryPrefs::OnMP3DownButton()
{
   // Modal help dialogue required here
   HelpSystem::ShowHelp(this, L"FAQ:Installing_the_LAME_MP3_Encoder", true);
}

void LibraryPrefs::SetFFmpegVersionText()
{
   mFFmpegVersion->SetValue(GetFFmpegVersion());
}

void LibraryPrefs::OnFFmpegFindButton()
{
#ifdef USE_FFMPEG
   FFmpegLibs* FFmpegLibsPtr = PickFFmpegLibs();
   bool showerrs =
#if defined(_DEBUG)
      true;
#else
      false;
#endif

   FFmpegLibsPtr->FreeLibs();
   // Load the libs ('true' means that all errors will be shown)
   bool locate = !LoadFFmpeg(showerrs);

   // Libs are fine, don't show "locate" dialog unless user really wants it
   if (!locate) {
      int response = AudacityMessageBox(
         XO(
"Audacity has automatically detected valid FFmpeg libraries.\nDo you still want to locate them manually?"),
         XO("Success"),
         wxCENTRE | wxYES_NO | wxNO_DEFAULT |wxICON_QUESTION);
      if (response == wxYES) {
        locate = true;
      }
   }

   if (locate) {
      // Show "Locate FFmpeg" dialog
      FFmpegLibsPtr->FindLibs(this);
      FFmpegLibsPtr->FreeLibs();
      LoadFFmpeg(showerrs);
   }
   SetFFmpegVersionText();

   DropFFmpegLibs();
#endif
}

void LibraryPrefs::OnFFmpegDownButton()
{
   HelpSystem::ShowHelp(this, L"FAQ:Installing_the_FFmpeg_Import_Export_Library", true);
}

bool LibraryPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

#if !defined(DISABLE_DYNAMIC_LOADING_FFMPEG) || !defined(DISABLE_DYNAMIC_LOADING_LAME)
namespace{
PrefsPanel::Registration sAttachment{ "Library",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew LibraryPrefs(parent, winid);
   },
   false,
   // Register with an explicit ordering hint because this one is
   // only conditionally compiled
   { "", { Registry::OrderingHint::Before, "Directories" } }
};
}
#endif

#ifdef EXPERIMENTAL_OD_FFMPEG
BoolSetting LibraryFFmpegOnDemand{L"/Library/FFmpegOnDemand", false};
#endif

