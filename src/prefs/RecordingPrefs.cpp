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
#include "RecordingSettings.h"
#include "AudioIOBase.h"

#include <wx/defs.h>
#include <wx/textctrl.h>
#include <algorithm>

#include "../Prefs.h"
#include "../ShuttleGui.h"

#include "GUIPrefs.h"

using std::min;

RecordingPrefs::RecordingPrefs(wxWindow * parent, wxWindowID winid)
:  PrefsPanel(parent, winid, XO("Recording")) // XC("Recording", "preference")
{
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

void RecordingPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("Options"));
   {
      // Start wording of options with a verb, if possible.
      S
         .Target( AudioIODuplex )
         .AddCheckBox( XXO("Play &other tracks while recording (overdub)") );

//#if defined(__WXMAC__)
// Bug 388.  Feature not supported on any Mac Hardware.
#if 0
      S
         .Target( AudioIOPlaythrough )
         .AddCheckBox( XO("Use &hardware to play other tracks") );
#endif

      S
         .Target( AudioIOSWPlaythrough )
         .AddCheckBox( XXO("&Software playthrough of input") );

#if !defined(__WXMAC__)
      //S
      //   .AddUnits(XO("     (uncheck when recording computer playback)"));
#endif

      S
         .Target( RecordingPreferNewTrack )
         .AddCheckBox( XXO("Record on a new track") );

      S
         .Target( WarningsDropoutDetected )
         /* i18n-hint: Dropout is a loss of a short sequence audio sample data from the recording */
         .AddCheckBox( XXO("Detect dropouts") );


   }
   S.EndStatic();

   S
      .StartStatic(XO("Sound Activated Recording"));
   {
      S
         .Target( AudioIOSoundActivatedRecord )
         .AddCheckBox( XXO("&Enable") );

      S.StartMultiColumn(2, GroupOptions{ wxEXPAND }.StretchyColumn(1));
      {
         S
            .Target( AudioIOSilenceLevel )
            .AddSlider(XXO("Le&vel (dB):"),
               0, 0, -GUIdBRange.Read());
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
               .Target( RecordingSettings::CustomName )
               .AddCheckBox(XXO("Custom Track &Name"));

            S
               .Text(XO("Custom name text"))
               .Enable( []{ return RecordingSettings::CustomName.Read(); } )
               .Target( RecordingTrackName )
               .AddTextBox( {}, {}, 30);
         }

         S.EndMultiColumn();

         S
            .AddFixedText(  {} );

         S.StartMultiColumn(3);
         {
            S
               .Target( RecordingTrackNumber )
               .AddCheckBox( XXO("&Track Number") );

            S
               .Target( RecordingDateStamp )
               .AddCheckBox( XXO("System &Date") );

            S
               .Target( RecordingTimeStamp )
               .AddCheckBox( XXO("System T&ime") );
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
            .Target( AudioIOAutomatedInputLevelAdjustment )
            .AddCheckBox( XXO("Enable Automated Recording Level Adjustment.") );

         S.StartMultiColumn(2, GroupOptions{}.Position(wxEXPAND).StretchyColumn(1));
         {

            S
               .Target( AudioIOTargetPeak )
               /* i18n-hint: Desired maximum (peak) volume for sound */
               .AddSlider(XXO("Target Peak:"),
                  0, 100, 0);

            S
               .Target( AudioIODeltaPeakVolume )
               .AddSlider(XXO("Within:"),
                  0, 100, 0);
         }
         S.EndMultiColumn();

         S.StartThreeColumn();
         {
            S
               .Target( AudioIOAnalysisTime )
               .AddTextBox(XXO("Analysis Time:"), {}, 9);
   
            S
              .AddUnits(XO("milliseconds (time of one analysis)"));

            S
               .Target( AudioIONumberAnalysis )
               .AddTextBox(XXO("Number of consecutive analysis:"), {}, 2);

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
               .Target( AudioIOPreRoll )
               .AddTextBox(XXO("Pre-ro&ll:"), {}, 9);

            S
               .AddUnits(XO("seconds"));
         }
         {
            auto w =
            S
               .Text({ {}, XO("milliseconds") })
               .Target( AudioIOCrossfade )
               .AddTextBox(XXO("Cross&fade:"), {}, 9);

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
   wxPanel::TransferDataFromWindow();

   if (AudioIOLatencyDuration.Read() < 0)
      AudioIOLatencyDuration.Reset();

   #ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
      double targetpeak =  AudioIOTargetPeak.Read();
      double deltapeak = AudioIODeltaPeakVolume.Read();
      if (targetpeak + deltapeak > 100.0 || targetpeak - deltapeak < 0.0)
         AudioIODeltaPeakVolume.Write( min(100 - targetpeak, targetpeak ) );

      auto value = AudioIOAnalysisTime.Read();
      if (value <= 0)
         AudioIOAnalysisTime.Reset();

      value = AudioIONumberAnalysis.Read();
      if (value < 0)
         AudioIONumberAnalysis.Reset();
   #endif
   return true;
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

BoolSetting AudioIODuplex {
   L"/AudioIO/Duplex",
#ifdef EXPERIMENTAL_DA
                                                     false
#else
                                                     true
#endif
};

BoolSetting RecordingDateStamp{
   L"/GUI/TrackNames/DateStamp",           false };

BoolSetting RecordingPreferNewTrack{
   L"/GUI/PreferNewTrackRecord",           false };
BoolSetting RecordingTimeStamp{
   L"/GUI/TrackNames/TimeStamp",           false };
StringSetting RecordingTrackName{
   L"/GUI/TrackNames/RecodingTrackName", // sic, don't change, be compatible
   // Default value depends on current language preference
   [] { return XO("Recorded_Audio").Translation(); }
};
BoolSetting RecordingTrackNumber{
   L"/GUI/TrackNames/TrackNumber",         false };
//BoolSetting AudioIOPlaythrough{
// L"/AudioIO/Playthrough",                      false };

DoubleSetting AudioIOCrossfade{
   L"/AudioIO/Crossfade",                        10.0 };
DoubleSetting AudioIOPreRoll{
   L"/AudioIO/PreRoll",                          5.0 };

IntSetting AudioIOSilenceLevel{
   L"/AudioIO/SilenceLevel",                     -50 };

BoolSetting AudioIOSoundActivatedRecord{
   L"/AudioIO/SoundActivatedRecord",             false };
BoolSetting AudioIOSWPlaythrough{
   L"/AudioIO/SWPlaythrough",                    false };

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
BoolSetting AudioIOAutomatedInputLevelAdjustment{
   L"/AudioIO/AutomatedInputLevelAdjustment",    false };
IntSetting AudioIODeltaPeakVolume{
   L"/AudioIO/DeltaPeak",                        2 };
IntSetting AudioIOTargetPeak{
   L"/AudioIO/TargetPeak",                       92 };

IntSetting AudioIOAnalysisTime{
   L"/AudioIO/AnalysisTime",                     1000 };
IntSetting AudioIONumberAnalysis{
   L"/AudioIO/NumberAnalysis",                   5 };
#endif
