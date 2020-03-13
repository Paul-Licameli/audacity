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
#include <wx/textctrl.h>

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

   S.StartScroller();
   S.SetBorder(2);

   S.StartStatic(XO("Effects Preview"));
   {
      S.StartThreeColumn();
      {
         S
            .Text({ {}, suffix })
            .TieNumericTextBox(XXO("&Length:"),
                                 AudioIOEffectsPreviewLen,
                                 9);
   
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
            .TieNumericTextBox(XXO("&Before cut region:"),
                                 AudioIOCutPreviewBeforeLen,
                                 9);

         S
            .AddUnits(XO("seconds"));

         S
            .Text({ {}, suffix })
            .TieNumericTextBox(XXO("&After cut region:"),
                                 AudioIOCutPreviewAfterLen,
                                 9);

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
            .TieNumericTextBox(XXO("&Short period:"),
                                 AudioIOSeekShortPeriod,
                                 9);

         S
            .AddUnits(XO("seconds"));

         S
            .Text({ {}, suffix })
            .TieNumericTextBox(XXO("Lo&ng period:"),
                                 AudioIOSeekLongPeriod,
                                 9);

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
            .TieCheckBox(XXO("&Vari-Speed Play"), AudioIOVariSpeedPlay);

         S
            .TieCheckBox(XXO("&Micro-fades"), AudioIOMicrofades);

         S
            .TieCheckBox(XXO("Always scrub un&pinned"),
               AudioIOUnpinnedScrubbing);
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

   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

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
