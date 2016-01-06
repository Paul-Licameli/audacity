/**********************************************************************

  Audacity: A Digital Audio Editor

  SpectrumPrefs.h

  Dominic Mazzoni
  James Crook

**********************************************************************/
/*
  Salvo Ventura
  November 2006

  Added selection box for windowType

  All params are saved in config file.
*/


#ifndef __AUDACITY_SPECTRUM_PREFS__
#define __AUDACITY_SPECTRUM_PREFS__

#include "../Experimental.h"

#include <vector>
#include <wx/defs.h>

#include "PrefsPanel.h"
#include "SpectrogramSettings.h"

class wxChoice;
class wxCheckBox;
class wxTextCtrl;
class wxSlider;
struct FFTParam;
class ShuttleGui;
class SpectrogramSettings;
class WaveTrack;
struct WaveTrackSubViewPlacement;

#define SPECTRUM_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Spectrum") }

class SpectrumPrefs final : public PrefsPanel
{
 public:
   SpectrumPrefs(wxWindow * parent, wxWindowID winid,
      AudacityProject *pProject, WaveTrack *wt);
   virtual ~SpectrumPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   void Preview() override;
   bool Commit() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   void Rollback();
   bool ShowsPreviewButton() override;
   bool Validate() override;
   wxString HelpPageName() override;

 private:
   void Populate(size_t windowSize);
   void PopulatePaddingChoices(size_t windowSize);

   void OnControl(wxCommandEvent &event);
   void OnWindowSize(wxCommandEvent &event);
   void OnDefaults(wxCommandEvent&);
   void OnAlgorithm(wxCommandEvent &);
   DECLARE_EVENT_TABLE()

   void EnableDisableSTFTOnlyControls();

#ifdef EXPERIMENTAL_WATERFALL_SPECTROGRAMS
   void OnStyle(wxCommandEvent &event);
   void EnableDisableWaterfallOnlyControls();
#endif

   AudacityProject *mProject{};

   WaveTrack *const mWt;
   bool mDefaulted, mOrigDefaulted;

   wxSlider *mMinFreq;
   wxSlider *mMaxFreq;
   wxSlider *mGain;
   wxSlider *mRange;
   wxSlider *mFrequencyGain;

#ifdef EXPERIMENTAL_WATERFALL_SPECTROGRAMS
   wxChoice *mStyleChoice;

   wxSlider *mWaterfallSlope;
   wxSlider *mWaterfallHeight;
#endif

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   int mZeroPaddingChoice;
   wxChoice *mZeroPaddingChoiceCtrl;
   TranslatableStrings mZeroPaddingChoices;
#endif

   TranslatableStrings mTypeChoices;

   wxChoice *mAlgorithmChoice;


#ifdef EXPERIMENTAL_FIND_NOTES
   wxTextCtrl *mFindNotesMinA;
   wxTextCtrl *mFindNotesN;
#endif

   wxCheckBox *mDefaultsCheckbox;

   SpectrogramSettings mTempSettings, mOrigSettings;

   std::vector<WaveTrackSubViewPlacement> mOrigPlacements;
   float mOrigMin, mOrigMax;

   bool mPopulating;
   bool mCommitted{};
};

/// A PrefsPanel::Factory that creates one SpectrumPrefs panel.
/// This factory can be parametrized by a single track, to change settings
/// non-globally
extern PrefsPanel::Factory SpectrumPrefsFactory( WaveTrack *wt = 0 );
#endif
