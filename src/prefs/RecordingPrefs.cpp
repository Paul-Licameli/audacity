/**********************************************************************

  Audacity: A Digital Audio Editor

  RecordingPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class RecordingPrefs
\brief A PrefsPanel used to select recording options.

  Presents interface for user to update the various recording options
  like playthrough, latency correction, and others.

*//********************************************************************/


#include "RecordingPrefs.h"
#include "AudioIOBase.h"

#include <wx/defs.h>
#include <wx/textctrl.h>
#include <algorithm>

#include "../prefs/GUISettings.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

using std::min;

enum {
   UseCustomTrackNameID = 1000,
};

BEGIN_EVENT_TABLE(RecordingPrefs, PrefsPanel)
   EVT_CHECKBOX(UseCustomTrackNameID, RecordingPrefs::OnToggleCustomName)
END_EVENT_TABLE()

RecordingPrefs::RecordingPrefs(wxWindow * parent, wxWindowID winid)
:  PrefsPanel(parent, winid, XO("Recording")) // XC("Recording", "preference")
{
   gPrefs->Read(L"/GUI/TrackNames/RecordingNameCustom", &mUseCustomTrackName, false);
   mOldNameChoice = mUseCustomTrackName;
   Populate();
}

RecordingPrefs::~RecordingPrefs()
{
}

ComponentInterfaceSymbol RecordingPrefs::GetSymbol()
{
   return RECORDING_PREFS_PLUGIN_SYMBOL;
}

TranslatableString RecordingPrefs::GetDescription()
{
   return XO("Preferences for Recording");
}

wxString RecordingPrefs::HelpPageName()
{
   return "Recording_Preferences";
}

void RecordingPrefs::Populate()
{
   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void RecordingPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("Options"));
   {
      // Start wording of options with a verb, if possible.
      S
         .TieCheckBox(XXO("Play &other tracks while recording (overdub)"),
            {L"/AudioIO/Duplex",
#ifdef EXPERIMENTAL_DA
             false
#else
             true
#endif
            });

//#if defined(__WXMAC__)
// Bug 388.  Feature not supported on any Mac Hardware.
#if 0
      S
         .TieCheckBox(XO("Use &hardware to play other tracks"),
            {L"/AudioIO/Playthrough", false});
#endif

      S
         .TieCheckBox(XXO("&Software playthrough of input"),
            {L"/AudioIO/SWPlaythrough", false});

#if !defined(__WXMAC__)
      //S
      //   .AddUnits(XO("     (uncheck when recording computer playback)"));
#endif

       S
         .TieCheckBox(XXO("Record on a new track"),
            {L"/GUI/PreferNewTrackRecord", false});

/* i18n-hint: Dropout is a loss of a short sequence audio sample data from the recording */
       S
         .TieCheckBox(XXO("Detect dropouts"),
            {WarningDialogKey(L"DropoutDetected"), true});
   }
   S.EndStatic();

   S
      .StartStatic(XO("Sound Activated Recording"));
   {
      S
         .TieCheckBox(XXO("&Enable"),
            {L"/AudioIO/SoundActivatedRecord", false});

      S.StartMultiColumn(2, wxEXPAND);
      {
         S.SetStretchyCol(1);

         S
            .TieSlider(XXO("Le&vel (dB):"),
               {L"/AudioIO/SilenceLevel", -50},
               0, -gPrefs->Read(ENV_DB_KEY, ENV_DB_RANGE));
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   /* i18n-hint: start of two-part phrase, "Name newly recorded tracks with:" */
   S
      .StartStatic(XO("Name newly recorded tracks"));
   {
      // Nested multicolumns to indent by 'With:' width, in a way that works if 
      // translated.
      // This extra step is worth doing to get the check boxes lined up nicely.
      S.StartMultiColumn( 2 );
      {
         S
            /* i18n-hint: end of two-part phrase, "Name newly recorded tracks with:" */
            .AddFixedText(XO("With:")) ;

         S.StartMultiColumn(3);
         {
            S
               .Id(UseCustomTrackNameID)
               .TieCheckBox(XXO("Custom Track &Name"),
                  {L"/GUI/TrackNames/RecordingNameCustom",
                   mUseCustomTrackName});

            mToggleCustomName =
            S
               .Text(XO("Custom name text"))
               .Disable(!mUseCustomTrackName)
               .TieTextBox( {},
                  {L"/GUI/TrackNames/RecodingTrackName",
                   _("Recorded_Audio")},
                  30);
         }

         S.EndMultiColumn();

         S
            .AddFixedText(  {} );

         S.StartMultiColumn(3);
         {
            S
               .TieCheckBox(XXO("&Track Number"),
                  {L"/GUI/TrackNames/TrackNumber", false});

            S
               .TieCheckBox(XXO("System &Date"),
                  {L"/GUI/TrackNames/DateStamp", false});

            S
               .TieCheckBox(XXO("System T&ime"),
                  {L"/GUI/TrackNames/TimeStamp", false});
         }
         S.EndMultiColumn();
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   #ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
      S
         .StartStatic(XO("Automated Recording Level Adjustment"));
      {
         S
            .TieCheckBox(XXO("Enable Automated Recording Level Adjustment."),
               {L"/AudioIO/AutomatedInputLevelAdjustment", false});

         S.StartMultiColumn(2, wxEXPAND);
         {
            S.SetStretchyCol(1);

            S
               /* i18n-hint: Desired maximum (peak) volume for sound */
               .TieSlider(XXO("Target Peak:"),
                  {L"/AudioIO/TargetPeak",
                   AILA_DEF_TARGET_PEAK},
                  100,
                  0);

            S
               .TieSlider(XXO("Within:"),
                  {L"/AudioIO/DeltaPeakVolume",
                   AILA_DEF_DELTA_PEAK},
                  100,
                  0);
         }
         S.EndMultiColumn();

         S.StartThreeColumn();
         {
            S
               .TieIntegerTextBox(XXO("Analysis Time:"),
                  {L"/AudioIO/AnalysisTime",
                   AILA_DEF_ANALYSIS_TIME},
                  9);
   
            S
              .AddUnits(XO("milliseconds (time of one analysis)"));

            S
               .TieIntegerTextBox(XXO("Number of consecutive analysis:"),
                  {L"/AudioIO/NumberAnalysis",
                   AILA_DEF_NUMBER_ANALYSIS},
                  2);

            S
               .AddUnits(XO("0 means endless"));
          }
          S.EndThreeColumn();
      }
      S.EndStatic();
   #endif

#ifdef EXPERIMENTAL_PUNCH_AND_ROLL
      S.StartStatic(XO("Punch and Roll Recording"));
      {
         S.StartThreeColumn();
         {
            auto w =
            S
               .Text({ {}, XO("seconds") })
               .TieNumericTextBox(XXO("Pre-ro&ll:"),
                  {AUDIO_PRE_ROLL_KEY,
                   DEFAULT_PRE_ROLL_SECONDS},
                  9);

            S
               .AddUnits(XO("seconds"));
         }
         {
            auto w =
            S
               .Text({ {}, XO("milliseconds") })
               .TieNumericTextBox(XXO("Cross&fade:"),
                  {AUDIO_ROLL_CROSSFADE_KEY,
                   DEFAULT_ROLL_CROSSFADE_MS},
                  9);

            S
               .AddUnits(XO("milliseconds"));
         }
         S.EndThreeColumn();
      }
      S.EndStatic();
#endif

      S.EndScroller();
}

bool RecordingPrefs::Commit()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   if (AudioIOLatencyDuration.Read() < 0)
      AudioIOLatencyDuration.Reset();

   #ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
      double targetpeak, deltapeak;
      gPrefs->Read(L"/AudioIO/TargetPeak",  &targetpeak);
      gPrefs->Read(L"/AudioIO/DeltaPeakVolume", &deltapeak);
      if (targetpeak + deltapeak > 100.0 || targetpeak - deltapeak < 0.0)
      {
         gPrefs->Write(L"/AudioIO/DeltaPeakVolume", min(100.0 - targetpeak, targetpeak));
      }

      int value;
      gPrefs->Read(L"/AudioIO/AnalysisTime", &value);
      if (value <= 0)
         gPrefs->Write(L"/AudioIO/AnalysisTime", AILA_DEF_ANALYSIS_TIME);

      gPrefs->Read(L"/AudioIO/NumberAnalysis", &value);
      if (value < 0)
         gPrefs->Write(L"/AudioIO/NumberAnalysis", AILA_DEF_NUMBER_ANALYSIS);
   #endif
   return true;
}

void RecordingPrefs::OnToggleCustomName(wxCommandEvent & /* Evt */)
{
   mUseCustomTrackName = !mUseCustomTrackName;
   mToggleCustomName->Enable(mUseCustomTrackName);
}

namespace{
PrefsPanel::Registration sAttachment{ "Recording",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew RecordingPrefs(parent, winid);
   }
};
}
