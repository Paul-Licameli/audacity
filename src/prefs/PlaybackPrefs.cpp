/**********************************************************************

  Audacity: A Digital Audio Editor

  PlaybackPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class PlaybackPrefs
\brief A PrefsPanel used to select playback options.

  Presents interface for user to update the various playback options
  like previewing and seeking.

*//********************************************************************/


#include "PlaybackPrefs.h"

#include <wx/defs.h>

#include "../AudioIOBase.h"
#include "../ShuttleGui.h"
#include "../Prefs.h"

PlaybackPrefs::PlaybackPrefs(wxWindow * parent, wxWindowID winid)
:  PrefsPanel(parent, winid, XO("Playback"))
{
}

PlaybackPrefs::~PlaybackPrefs()
{
}

ComponentInterfaceSymbol PlaybackPrefs::GetSymbol()
{
   return PLAYBACK_PREFS_PLUGIN_SYMBOL;
}

TranslatableString PlaybackPrefs::GetDescription()
{
   return XO("Preferences for Playback");
}

wxString PlaybackPrefs::HelpPageName()
{
   return "Playback_Preferences";
}

namespace {
   int iPreferenceUnpinned = -1;
}

void PlaybackPrefs::PopulateOrExchange(ShuttleGui & S)
{
   const auto suffix = XO("seconds");

   S.StartScroller(0, 2);

   S.StartStatic(XO("Effects Preview"));
   {
      S.StartThreeColumn();
      {
         S
            .Text({ {}, suffix })
            .Target( AudioIOEffectsPreviewLen )
            .AddTextBox(XXO("&Length:"), {}, 9);
   
         S
            .AddUnits(XO("seconds"));
      }
      S.EndThreeColumn();
   }
   S.EndStatic();

   /* i18n-hint: (noun) this is a preview of the cut */
   S
      .StartStatic(XO("Cut Preview"));
   {
      S.StartThreeColumn();
      {
         S
         .Text({ {}, suffix })
            .Target( AudioIOCutPreviewBeforeLen )
            .AddTextBox(XXO("&Before cut region:"), {}, 9);

         S
            .AddUnits(XO("seconds"));

         S
            .Text({ {}, suffix })
            .Target( AudioIOCutPreviewAfterLen )
            .AddTextBox(XXO("&After cut region:"), {}, 9);

         S
            .AddUnits(XO("seconds"));
      }
      S.EndThreeColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("Seek Time when playing"));
   {
      S.StartThreeColumn();
      {
         S
            .Text({ {}, suffix })
            .Target( AudioIOSeekShortPeriod )
            .AddTextBox(XXO("&Short period:"), {}, 9);

         S
            .AddUnits(XO("seconds"));

         S
            .Text({ {}, suffix })
            .Target( AudioIOSeekLongPeriod )
            .AddTextBox(XXO("Lo&ng period:"), {}, 9);

         S
            .AddUnits(XO("seconds"));
      }
      S.EndThreeColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("Options"));
   {
      S.StartVerticalLay();
      {
         S
            .Target( AudioIOVariSpeedPlay )
            .AddCheckBox( XXO("&Vari-Speed Play") );

         S
            .Target( AudioIOMicrofades )
            .AddCheckBox( XXO("&Micro-fades") );

         S
            .Target( AudioIOUnpinnedScrubbing )
            .AddCheckBox( XXO("Always scrub un&pinned") );
      }
      S.EndVerticalLay();
   }
   S.EndStatic();


   S.EndScroller();

}

bool PlaybackPrefs::GetUnpinnedScrubbingPreference()
{
   if ( iPreferenceUnpinned >= 0 )
      return iPreferenceUnpinned == 1;
   auto bResult = AudioIOUnpinnedScrubbing.Read();
   iPreferenceUnpinned = bResult ? 1: 0;
   return bResult;
}

bool PlaybackPrefs::Commit()
{
   iPreferenceUnpinned = -1;

   wxPanel::TransferDataFromWindow();

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "Playback",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew PlaybackPrefs(parent, winid);
   }
};
}
