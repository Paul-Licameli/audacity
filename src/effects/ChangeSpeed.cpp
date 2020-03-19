/**********************************************************************

  Audacity: A Digital Audio Editor

  ChangeSpeed.cpp

  Vaughan Johnson, Dominic Mazzoni

*******************************************************************//**

\class EffectChangeSpeed
\brief An Effect that affects both pitch & speed.

*//*******************************************************************/


#include "ChangeSpeed.h"
#include "LoadEffects.h"

#include <math.h>

#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/textctrl.h>

#include "../LabelTrack.h"
#include "../Prefs.h"
#include "../Resample.h"
#include "../ShuttleGui.h"
#include "../widgets/NumericTextCtrl.h"

#include "TimeWarper.h"
#include "../WaveClip.h"
#include "../WaveTrack.h"

enum
{
   ID_PercentChange = 10000,
   ID_Multiplier,
   ID_FromVinyl,
   ID_ToVinyl,
   ID_ToLength
};

// the standard vinyl rpm choices
// If the percent change is not one of these ratios, the choice control gets "n/a".
enum kVinyl
{
   kVinyl_33AndAThird = 0,
   kVinyl_45,
   kVinyl_78,
   kVinyl_NA
};

static const TranslatableStrings kVinylStrings{
   XO("33\u2153"),
   XO("45"),
   XO("78"),
   /* i18n-hint: n/a is an English abbreviation meaning "not applicable". */
   XO("n/a"),
};

// Soundtouch is not reasonable below -99% or above 3000%.

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name          Type     Key               Def   Min      Max      Scale
static auto Percentage = Parameter<double>(
                           L"Percentage", 0.0,  -99.0,   4900.0,  1  );

// We warp the slider to go up to 400%, but user can enter higher values
static const double kSliderMax = 100.0;         // warped above zero to actually go up to 400%
static const double kSliderWarp = 1.30105;      // warp power takes max from 100 to 400.

//
// EffectChangeSpeed
//

const ComponentInterfaceSymbol EffectChangeSpeed::Symbol
{ XO("Change Speed") };

namespace{ BuiltinEffectsModule::Registration< EffectChangeSpeed > reg; }

BEGIN_EVENT_TABLE(EffectChangeSpeed, wxEvtHandler)
    EVT_TEXT(ID_PercentChange, EffectChangeSpeed::OnText_PercentChange)
    EVT_TEXT(ID_Multiplier, EffectChangeSpeed::OnText_Multiplier)
    EVT_SLIDER(ID_PercentChange, EffectChangeSpeed::OnSlider_PercentChange)
    EVT_TEXT(ID_ToLength, EffectChangeSpeed::OnTimeCtrl_ToLength)
    EVT_COMMAND(ID_ToLength, EVT_TIMETEXTCTRL_UPDATED, EffectChangeSpeed::OnTimeCtrlUpdate)
END_EVENT_TABLE()

EffectChangeSpeed::EffectChangeSpeed()
: mParameters{ m_PercentChange, Percentage }
{
   Parameters().Reset();

   mFromVinyl = kVinyl_33AndAThird;
   mToVinyl = kVinyl_33AndAThird;
   mFromLength = 0.0;
   mToLength = 0.0;
   mFormat = NumericConverter::DefaultSelectionFormat();

   SetLinearEffectFlag(true);
}

EffectChangeSpeed::~EffectChangeSpeed()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectChangeSpeed::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectChangeSpeed::GetDescription()
{
   return XO("Changes the speed of a track, also changing its pitch");
}

wxString EffectChangeSpeed::ManualPage()
{
   return L"Change_Speed";
}


// EffectDefinitionInterface implementation

EffectType EffectChangeSpeed::GetType()
{
   return EffectTypeProcess;
}

bool EffectChangeSpeed::LoadFactoryDefaults()
{
   mFromVinyl = kVinyl_33AndAThird;
   mFormat = NumericConverter::DefaultSelectionFormat();

   return Effect::LoadFactoryDefaults();
}

// Effect implementation

bool EffectChangeSpeed::CheckWhetherSkipEffect()
{
   return (m_PercentChange == 0.0);
}

double EffectChangeSpeed::CalcPreviewInputLength(double previewLength)
{
   return previewLength * (100.0 + m_PercentChange) / 100.0;
}

bool EffectChangeSpeed::Startup()
{
   wxString base = L"/Effects/ChangeSpeed/";

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + L"Migrated"))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      // Retrieve last used control values
      gPrefs->Read(base + L"PercentChange", &m_PercentChange, 0);

      wxString format;
      gPrefs->Read(base + L"TimeFormat", &format, wxString{});
      mFormat = NumericConverter::LookupFormat( NumericConverter::TIME, format );

      gPrefs->Read(base + L"VinylChoice", &mFromVinyl, 0);
      if (mFromVinyl == kVinyl_NA)
      {
         mFromVinyl = kVinyl_33AndAThird;
      }

      SetPrivateConfig(GetCurrentSettingsGroup(), L"TimeFormat", mFormat.Internal());
      SetPrivateConfig(GetCurrentSettingsGroup(), L"VinylChoice", mFromVinyl);

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + L"Migrated", true);
      gPrefs->Flush();
   }

   return true;
}

bool EffectChangeSpeed::Init()
{
   // The selection might have changed since the last time EffectChangeSpeed
   // was invoked, so recalculate the Length parameters.
   mFromLength = mT1 - mT0;
   return true;
}

bool EffectChangeSpeed::Process()
{
   // Similar to EffectSoundTouch::Process()

   // Iterate over each track.
   // All needed because this effect needs to introduce
   // silence in the sync-lock group tracks to keep sync
   CopyInputTracks(true); // Set up mOutputTracks.
   bool bGoodResult = true;

   mCurTrackNum = 0;
   mMaxNewLength = 0.0;

   mFactor = 100.0 / (100.0 + m_PercentChange);

   mOutputTracks->Any().VisitWhile( bGoodResult,
      [&](LabelTrack *lt) {
         if (lt->GetSelected() || lt->IsSyncLockSelected())
         {
            if (!ProcessLabelTrack(lt))
               bGoodResult = false;
         }
      },
      [&](WaveTrack *pOutWaveTrack, const Track::Fallthrough &fallthrough) {
         if (!pOutWaveTrack->GetSelected())
            return fallthrough();

         //Get start and end times from track
         mCurT0 = pOutWaveTrack->GetStartTime();
         mCurT1 = pOutWaveTrack->GetEndTime();

         //Set the current bounds to whichever left marker is
         //greater and whichever right marker is less:
         mCurT0 = std::max(mT0, mCurT0);
         mCurT1 = std::min(mT1, mCurT1);

         // Process only if the right marker is to the right of the left marker
         if (mCurT1 > mCurT0) {
            //Transform the marker timepoints to samples
            auto start = pOutWaveTrack->TimeToLongSamples(mCurT0);
            auto end = pOutWaveTrack->TimeToLongSamples(mCurT1);

            //ProcessOne() (implemented below) processes a single track
            if (!ProcessOne(pOutWaveTrack, start, end))
               bGoodResult = false;
         }
         mCurTrackNum++;
      },
      [&](Track *t) {
         if (t->IsSyncLockSelected())
            t->SyncLockAdjust(mT1, mT0 + (mT1 - mT0) * mFactor);
      }
   );

   if (bGoodResult)
      ReplaceProcessedTracks(bGoodResult);

   // Update selection.
   mT1 = mT0 + (((mT1 - mT0) * 100.0) / (100.0 + m_PercentChange));

   return bGoodResult;
}

void EffectChangeSpeed::PopulateOrExchange(ShuttleGui & S)
{
   {
      wxString formatId;
      GetPrivateConfig(GetCurrentSettingsGroup(), L"TimeFormat",
                       formatId, mFormat.Internal());
      mFormat = NumericConverter::LookupFormat(
         NumericConverter::TIME, formatId );
   }
   GetPrivateConfig(GetCurrentSettingsGroup(), L"VinylChoice", mFromVinyl, mFromVinyl);

   S.StartVerticalLay(0);
   {
      S.AddSpace(0, 5);
   
      S
         .AddTitle(XO("Change Speed, affecting both Tempo and Pitch"));

      S.AddSpace(0, 10);

      // Speed multiplier and percent change controls.
      S.StartMultiColumn(4, wxCENTER);
      {
         S
            .Id(ID_Multiplier)
            .Target( mMultiplier,
               NumValidatorStyle::THREE_TRAILING_ZEROES, 3,
               Percentage.min / 100.0, ((Percentage.min / 100.0) + 1) )
            .AddTextBox(XXO("&Speed Multiplier:"), L"", 12)
            .Assign(mpTextCtrl_Multiplier);

         S
            .Id(ID_PercentChange)
            .Target( m_ApproximatePercentChange,
               NumValidatorStyle::THREE_TRAILING_ZEROES, 3,
               Percentage.min, Percentage.max )
            .AddTextBox(XXO("Percent C&hange:"), L"", 12)
            .Assign(mpTextCtrl_PercentChange);
      }
      S.EndMultiColumn();

      // Percent change slider.
      S.StartHorizontalLay(wxEXPAND);
      {
         S
            .Id(ID_PercentChange)
            .Text(XO("Percent Change"))
            .Style(wxSL_HORIZONTAL)
            .AddSlider( {}, 0, (int)kSliderMax, (int)Percentage.min)
            .Assign(mpSlider_PercentChange);
      }
      S.EndHorizontalLay();

      // Vinyl rpm controls.
      S.StartMultiColumn(5, wxCENTER);
      {
         /* i18n-hint: "rpm" is an English abbreviation meaning "revolutions per minute".
            "vinyl" refers to old-fashioned phonograph records */
         S
            .AddUnits(XO("Standard Vinyl rpm:"));

         S
            .Id(ID_FromVinyl)
            /* i18n-hint: changing speed of audio "from" one value "to" another
             "rpm" means "revolutions per minute" as on a vinyl record turntable
             */
            .Text(XO("From rpm"))
            .MinSize( { 100, -1 } )
            .Target( mFromVinyl )
            .Action( [this]{ OnChoice_Vinyl(); } )
            /* i18n-hint: changing speed of audio "from" one value "to" another */
            .AddChoice(XXC("&from", "change speed"), kVinylStrings)
            .Assign(mpChoice_FromVinyl);

         S
            .Id(ID_ToVinyl)
            /* i18n-hint: changing speed of audio "from" one value "to" another
             "rpm" means "revolutions per minute" as on a vinyl record turntable
             */
            .Text(XO("To rpm"))
            .MinSize( { 100, -1 } )
            .Target( mToVinyl )
            .Action( [this]{ OnChoice_Vinyl(); } )
            /* i18n-hint: changing speed of audio "from" one value "to" another */
            .AddChoice(XXC("&to", "change speed"), kVinylStrings)
            .Assign(mpChoice_ToVinyl);
      }
      S.EndMultiColumn();

      // From/To time controls.
      S.StartStatic(XO("Selection Length"), 0);
      {
         S.StartMultiColumn(2, wxALIGN_LEFT);
         {
            S
               .AddPrompt(XXO("C&urrent Length:"));

            S
               /* i18n-hint: changing speed of audio "from" one value "to" another */
               .Text({ XO("from"), {},
                  XO("Current length of selection.")})
               .Position(wxALIGN_LEFT)
               .AddNumericTextCtrl(NumericConverter::TIME,
                  mFormat,
                  mFromLength,
                  mProjectRate,
                  NumericTextCtrl::Options{}
                     .ReadOnly(true)
                     .MenuEnabled(false))
               .Assign(mpFromLengthCtrl);

            S.AddPrompt(XXO("&New Length:"));

            S
               /* i18n-hint: changing a quantity "from" one value "to" another */
               .Text(XC("to", "change speed"))
               .Position(wxALIGN_LEFT)
               .AddNumericTextCtrl(NumericConverter::TIME,
                  mFormat,
                  mToLength,
                  mProjectRate)
               .Assign(mpToLengthCtrl);
         }
         S.EndMultiColumn();
      }
      S.EndStatic();
   }
   S.EndVerticalLay();
}

bool EffectChangeSpeed::TransferDataToWindow()
{
   if (mFromVinyl == kVinyl_NA)
   {
      mFromVinyl = kVinyl_33AndAThird;
   }

   UpdateApproximatePercentChange();
   UpdateMultiplier();
   Update_Slider_PercentChange();
   Update_TimeCtrl_ToLength();

   // Set from/to Vinyl controls - mFromVinyl must be set first.
   mpChoice_FromVinyl->SetSelection(mFromVinyl);
   // Then update to get correct mToVinyl.
   Update_Vinyl();
   // Then update ToVinyl control.
   mpChoice_ToVinyl->SetSelection(mToVinyl);

   // Set From Length control.
   // Set the format first so we can get sample accuracy.
   mpFromLengthCtrl->SetFormatName(mFormat);
   mpFromLengthCtrl->SetValue(mFromLength);

   return true;
}

bool EffectChangeSpeed::TransferDataFromWindow()
{
   SetPrivateConfig(GetCurrentSettingsGroup(), L"TimeFormat", mFormat.Internal());
   SetPrivateConfig(GetCurrentSettingsGroup(), L"VinylChoice", mFromVinyl);

   return true;
}

// EffectChangeSpeed implementation

// Labels are time-scaled linearly inside the affected region, and labels after
// the region are shifted along according to how the region size changed.
bool EffectChangeSpeed::ProcessLabelTrack(LabelTrack *lt)
{
   RegionTimeWarper warper { mT0, mT1,
      std::make_unique<LinearTimeWarper>(mT0, mT0,
                                         mT1, mT0 + (mT1-mT0)*mFactor) };
   lt->WarpLabels(warper);
   return true;
}

// ProcessOne() takes a track, transforms it to bunch of buffer-blocks,
// and calls libsamplerate code on these blocks.
bool EffectChangeSpeed::ProcessOne(WaveTrack * track,
                           sampleCount start, sampleCount end)
{
   if (track == NULL)
      return false;

   // initialization, per examples of Mixer::Mixer and
   // EffectSoundTouch::ProcessOne

   
   auto outputTrack = track->EmptyCopy();

   //Get the length of the selection (as double). len is
   //used simple to calculate a progress meter, so it is easier
   //to make it a double now than it is to do it later
   auto len = (end - start).as_double();

   // Initiate processing buffers, most likely shorter than
   // the length of the selection being processed.
   auto inBufferSize = track->GetMaxBlockSize();

   // mFactor is at most 100-fold so this shouldn't overflow size_t
   auto outBufferSize = size_t( mFactor * inBufferSize + 10 );
   Floats outBuffer{ outBufferSize };

   // Set up the resampling stuff for this track.
   Resample resample(true, mFactor, mFactor); // constant rate resampling

   bool bResult = ForEachBlock( { track }, start, end, inBufferSize,
   [&]( sampleCount samplePos, size_t blockSize, float *const *buffers, size_t ){
      auto inBuffer = buffers[0];
      const auto results = resample.Process(mFactor,
         (float*)inBuffer,
         blockSize,
         ((samplePos + blockSize) >= end),
         outBuffer.get(),
         outBufferSize);
      const auto outgen = results.second;

      if (outgen > 0)
         outputTrack->Append((samplePtr)outBuffer.get(), floatSample,
                             outgen);
      return true;
   }, mCurTrackNum);

   // Flush the output WaveTrack (since it's buffered, too)
   outputTrack->Flush();

   // Take the output track and insert it in place of the original
   // sample data
   double newLength = outputTrack->GetEndTime();
   if (bResult)
   {
      // Silenced samples will be inserted in gaps between clips, so capture where these
      // gaps are for later deletion
      std::vector<std::pair<double, double>> gaps;
      double last = mCurT0;
      auto clips = track->SortedClipArray();
      auto front = clips.front();
      auto back = clips.back();
      for (auto &clip : clips) {
         auto st = clip->GetStartTime();
         auto et = clip->GetEndTime();

         if (st >= mCurT0 || et < mCurT1) {
            if (mCurT0 < st && clip == front) {
               gaps.push_back(std::make_pair(mCurT0, st));
            }
            else if (last < st && mCurT0 <= last ) {
               gaps.push_back(std::make_pair(last, st));
            }

            if (et < mCurT1 && clip == back) {
               gaps.push_back(std::make_pair(et, mCurT1));
            }
         }
         last = et;
      }

      LinearTimeWarper warper { mCurT0, mCurT0, mCurT1, mCurT0 + newLength };

      // Take the output track and insert it in place of the original sample data
      track->ClearAndPaste(mCurT0, mCurT1, outputTrack.get(), true, true, &warper);

      // Finally, recreate the gaps
      for (auto gap : gaps) {
         auto st = track->LongSamplesToTime(track->TimeToLongSamples(gap.first));
         auto et = track->LongSamplesToTime(track->TimeToLongSamples(gap.second));
         if (st >= mCurT0 && et <= mCurT1 && st != et)
         {
            track->SplitDelete(warper.Warp(st), warper.Warp(et));
         }
      }
   }

   if (newLength > mMaxNewLength)
      mMaxNewLength = newLength;

   return bResult;
}

// handler implementations for EffectChangeSpeed

void EffectChangeSpeed::OnText_PercentChange(wxCommandEvent & WXUNUSED(evt))
{
   mpTextCtrl_PercentChange->GetValidator()->TransferFromWindow();
   m_PercentChange = m_ApproximatePercentChange;
   UpdateUI();

   Update_Text_Multiplier();
   Update_Slider_PercentChange();
   Update_VinylControls();
   Update_TimeCtrl_ToLength();
}

void EffectChangeSpeed::OnText_Multiplier(wxCommandEvent & WXUNUSED(evt))
{
   mpTextCtrl_Multiplier->GetValidator()->TransferFromWindow();
   m_PercentChange = 100 * (mMultiplier - 1);
   UpdateUI();

   Update_Text_PercentChange();
   Update_Slider_PercentChange();
   Update_VinylControls();
   Update_TimeCtrl_ToLength();
}

void EffectChangeSpeed::OnSlider_PercentChange(wxCommandEvent & WXUNUSED(evt))
{
   m_PercentChange = (double)(mpSlider_PercentChange->GetValue());
   // Warp positive values to actually go up faster & further than negatives.
   if (m_PercentChange > 0.0)
      m_PercentChange = pow(m_PercentChange, kSliderWarp);
   UpdateUI();

   Update_Text_PercentChange();
   Update_Text_Multiplier();
   Update_VinylControls();
   Update_TimeCtrl_ToLength();
}

void EffectChangeSpeed::OnChoice_Vinyl()
{
   // Treat mpChoice_FromVinyl and mpChoice_ToVinyl as one control since we need
   // both to calculate Percent Change.
   // Use this as the 'preferred' choice.
   if (mFromVinyl != kVinyl_NA) {
      SetPrivateConfig(GetCurrentSettingsGroup(), L"VinylChoice", mFromVinyl);
   }

   // If mFromVinyl & mToVinyl are set, then there's a NEW percent change.
   if ((mFromVinyl != kVinyl_NA) && (mToVinyl != kVinyl_NA))
   {
      double fromRPM;
      double toRPM;
      switch (mFromVinyl) {
      default:
      case kVinyl_33AndAThird:   fromRPM = 33.0 + (1.0 / 3.0); break;
      case kVinyl_45:            fromRPM = 45.0; break;
      case kVinyl_78:            fromRPM = 78; break;
      }
      switch (mToVinyl) {
      default:
      case kVinyl_33AndAThird:   toRPM = 33.0 + (1.0 / 3.0); break;
      case kVinyl_45:            toRPM = 45.0; break;
      case kVinyl_78:            toRPM = 78; break;
      }
      m_PercentChange = ((toRPM * 100.0) / fromRPM) - 100.0;
      UpdateUI();

      Update_Text_PercentChange();
      Update_Text_Multiplier();
      Update_Slider_PercentChange();
      Update_TimeCtrl_ToLength();
   }
}

void EffectChangeSpeed::OnTimeCtrl_ToLength(wxCommandEvent & WXUNUSED(evt))
{
   mToLength = mpToLengthCtrl->GetValue();
   // Division by (double) 0.0 is not an error and we want to show "infinite" in
   // text controls, so take care that we handle infinite values when they occur.
   m_PercentChange = ((mFromLength * 100.0) / mToLength) - 100.0;
   UpdateUI();

   Update_Text_PercentChange();
   Update_Text_Multiplier();
   Update_Slider_PercentChange();
   Update_VinylControls();
}

void EffectChangeSpeed::OnTimeCtrlUpdate(wxCommandEvent & evt)
{
   mFormat = NumericConverter::LookupFormat(
      NumericConverter::TIME, evt.GetString() );

   mpFromLengthCtrl->SetFormatName(mFormat);
   // Update From/To Length controls (precision has changed).
   mpToLengthCtrl->SetValue(mToLength);
   mpFromLengthCtrl->SetValue(mFromLength);
}

// helper functions

// Update Approximate change value from the exact
void EffectChangeSpeed::UpdateApproximatePercentChange()
{
   m_ApproximatePercentChange = m_PercentChange;
}

void EffectChangeSpeed::Update_Text_PercentChange()
// Update Text Percent control from percent change.
{
   UpdateApproximatePercentChange();
   mpTextCtrl_PercentChange->GetValidator()->TransferToWindow();
}

// Update Multiplier control from percent change.
void EffectChangeSpeed::UpdateMultiplier()
{
   mMultiplier =  1 + (m_PercentChange) / 100.0;
}

void EffectChangeSpeed::Update_Text_Multiplier()
{
   UpdateMultiplier();
   mpTextCtrl_Multiplier->GetValidator()->TransferToWindow();
}

void EffectChangeSpeed::Update_Slider_PercentChange()
// Update Slider Percent control from percent change.
{
   auto unwarped = std::min<double>(m_PercentChange, Percentage.max);
   if (unwarped > 0.0)
      // Un-warp values above zero to actually go up to kSliderMax.
      unwarped = pow(m_PercentChange, (1.0 / kSliderWarp));

   // Caution: m_PercentChange could be infinite.
   int unwarpedi = (int)(unwarped + 0.5);
   unwarpedi = std::min<int>(unwarpedi, (int)kSliderMax);

   mpSlider_PercentChange->SetValue(unwarpedi);
}

void EffectChangeSpeed::Update_Vinyl()
// Update Vinyl values from percent change.
{
   // Match Vinyl rpm when within 0.01% of a standard ratio.
   // Ratios calculated as: ((toRPM / fromRPM) - 1) * 100 * 100

   // Caution: m_PercentChange could be infinite
   int ratio = (int)((m_PercentChange * 100) + 0.5);

   switch (ratio)
   {
      case 0: // toRPM is the same as fromRPM
         if (mFromVinyl != kVinyl_NA) {
            mToVinyl = mFromVinyl;
         } else {
            // Use the last saved option.
            GetPrivateConfig(GetCurrentSettingsGroup(), L"VinylChoice", mFromVinyl, 0);
            mToVinyl = mFromVinyl;
         }
         break;
      case 3500:
         mFromVinyl = kVinyl_33AndAThird;
         mToVinyl = kVinyl_45;
         break;
      case 13400:
         mFromVinyl = kVinyl_33AndAThird;
         mToVinyl = kVinyl_78;
         break;
      case -2593:
         mFromVinyl = kVinyl_45;
         mToVinyl = kVinyl_33AndAThird;
         break;
      case 7333:
         mFromVinyl = kVinyl_45;
         mToVinyl = kVinyl_78;
         break;
      case -5727:
         mFromVinyl = kVinyl_78;
         mToVinyl = kVinyl_33AndAThird;
         break;
      case -4231:
         mFromVinyl = kVinyl_78;
         mToVinyl = kVinyl_45;
         break;
      default:
         mToVinyl = kVinyl_NA;
   }
   // and update variables.
   mFromVinyl = mpChoice_FromVinyl->GetSelection();
   mToVinyl = mpChoice_ToVinyl->GetSelection();
}

void EffectChangeSpeed::Update_VinylControls()
{
   Update_Vinyl();
   mpChoice_FromVinyl->SetSelection( mFromVinyl );
   mpChoice_ToVinyl->SetSelection( mToVinyl );
}

void EffectChangeSpeed::Update_TimeCtrl_ToLength()
// Update ToLength control from percent change.
{
   mToLength = (mFromLength * 100.0) / (100.0 + m_PercentChange);

   // Set the format first so we can get sample accuracy.
   mpToLengthCtrl->SetFormatName(mFormat);
   // Negative times do not make sense.
   // 359999 = 99h:59m:59s which is a little less disturbing than overflow characters
   // though it may still look a bit strange with some formats.
   mToLength = TrapDouble(mToLength, 0.0, 359999.0);
   mpToLengthCtrl->SetValue(mToLength);
}

void EffectChangeSpeed::UpdateUI()
{
   EnableApply(CanApply());
}

bool EffectChangeSpeed::CanApply()
// Disable OK and Preview if not in sensible range.
{
   return m_PercentChange >= Percentage.min && m_PercentChange <= Percentage.max;
}

