/**********************************************************************

  Audacity: A Digital Audio Editor

  RecordingPrefs.h

  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_RECORDING_PREFS__
#define __AUDACITY_RECORDING_PREFS__



#include <wx/defs.h>

#include "PrefsPanel.h"

class wxTextCtrl;
class BoolSetting;
class ShuttleGui;
class StringSetting;

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
   #define AILA_DEF_DELTA_PEAK 2
   #define AILA_DEF_ANALYSIS_TIME 1000
   #define AILA_DEF_NUMBER_ANALYSIS 5
#endif

#define RECORDING_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ \
   L"Recording", \
   XO("Recording")  /* XC("Recording", "preference") */ \
}

class RecordingPrefs final : public PrefsPanel
{
 public:
   RecordingPrefs(wxWindow * parent, wxWindowID winid);
   virtual ~RecordingPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   wxString HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

 private:
};

extern BoolSetting
     RecordingDateStamp
   , RecordingPreferNewTrack
   , RecordingTimeStamp
   , RecordingTrackNumber
;

// extern BoolSetting AudioIOPlaythrough;

extern DoubleSetting AudioIOCrossfade; // milliseconds
extern BoolSetting AudioIODuplex;
extern DoubleSetting AudioIOPreRoll; //seconds
extern IntSetting AudioIOSilenceLevel; // dB
extern BoolSetting AudioIOSoundActivatedRecord;
extern BoolSetting AudioIOSWPlaythrough;

extern StringSetting
     RecordingTrackName
;

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
extern BoolSetting AudioIOAutomatedInputLevelAdjustment;
extern IntSetting AudioIODeltaPeakVolume; // percentage
extern IntSetting AudioIOTargetPeak; // percentage

extern IntSetting AudioIOAnalysisTime; // milliseconds
extern IntSetting AudioIONumberAnalysis; // Limit number of iterations
#endif

#endif
