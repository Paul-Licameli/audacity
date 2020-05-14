/**********************************************************************

  Audacity: A Digital Audio Editor

  Normalize.cpp

  Dominic Mazzoni
  Vaughan Johnson (Preview)

*******************************************************************//**

\class EffectNormalize
\brief An Effect to bring the peak level up to a chosen level.

*//*******************************************************************/



#include "Normalize.h"
#include "LoadEffects.h"

#include <math.h>

#include <wx/checkbox.h>
#include <wx/stattext.h>

#include "../Prefs.h"
#include "../ProjectFileManager.h"
#include "../ShuttleGui.h"
#include "../WaveTrack.h"
#include "../widgets/ProgressDialog.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name         Type     Key                        Def      Min      Max   Scale
static auto PeakLevel = Parameter<double>(
                           L"PeakLevel",           -1.0,    -145.0,  0.0,  1  );
static auto RemoveDC = Parameter<bool>(
                           L"RemoveDcOffset",      true,    false,   true, 1  );
static auto ApplyGain = Parameter<bool>(
                           L"ApplyGain",           true,    false,   true, 1  );
static auto StereoInd = Parameter<bool>(
                           L"StereoIndependent",   false,   false,   true, 1  );

const ComponentInterfaceSymbol EffectNormalize::Symbol
{ XO("Normalize") };

namespace{ BuiltinEffectsModule::Registration< EffectNormalize > reg; }

EffectNormalize::EffectNormalize()
   : mParameters {
      mPeakLevel, PeakLevel,
      mGain, ApplyGain,
      mDC, RemoveDC,
      mStereoInd, StereoInd,
   }
{
   Parameters().Reset();
   SetLinearEffectFlag(false);
}

EffectNormalize::~EffectNormalize()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectNormalize::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectNormalize::GetDescription()
{
   return XO("Sets the peak amplitude of one or more tracks");
}

wxString EffectNormalize::ManualPage()
{
   return L"Normalize";
}

// EffectDefinitionInterface implementation

EffectType EffectNormalize::GetType()
{
   return EffectTypeProcess;
}

// Effect implementation

bool EffectNormalize::CheckWhetherSkipEffect()
{
   return ((mGain == false) && (mDC == false));
}

bool EffectNormalize::Startup()
{
   wxString base = L"/Effects/Normalize/";

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + L"Migrated"))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      int boolProxy = gPrefs->Read(base + L"RemoveDcOffset", 1);
      mDC = (boolProxy == 1);
      boolProxy = gPrefs->Read(base + L"Normalize", 1);
      mGain = (boolProxy == 1);
      gPrefs->Read(base + L"Level", &mPeakLevel, -1.0);
      if(mPeakLevel > 0.0)  // this should never happen
         mPeakLevel = -mPeakLevel;
      boolProxy = gPrefs->Read(base + L"StereoIndependent", 0L);
      mStereoInd = (boolProxy == 1);

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + L"Migrated", true);
      gPrefs->Flush();
   }

   return true;
}

bool EffectNormalize::Process()
{
   if (mGain == false && mDC == false)
      return true;

   float ratio;
   if( mGain )
   {
      // same value used for all tracks
      ratio = DB_TO_LINEAR(TrapDouble(mPeakLevel, PeakLevel.min, PeakLevel.max));
   }
   else {
      ratio = 1.0;
   }

   //Iterate over each track
   this->CopyInputTracks(); // Set up mOutputTracks.
   bool bGoodResult = true;
   unsigned count = 0;
   TranslatableString topMsg;
   if(mDC && mGain)
      topMsg = XO("Removing DC offset and Normalizing...\n");
   else if(mDC && !mGain)
      topMsg = XO("Removing DC offset...\n");
   else if(!mDC && mGain)
      topMsg = XO("Normalizing without removing DC offset...\n");
   else if(!mDC && !mGain)
      topMsg = XO("Not doing anything...\n");   // shouldn't get here

   for ( auto track : mOutputTracks->Selected< WaveTrack >()
            + ( mStereoInd ? &Track::Any : &Track::IsLeader ) ) {
      //Get start and end times from track
      // PRL:  No accounting for multiple channels?
      double trackStart = track->GetStartTime();
      double trackEnd = track->GetEndTime();

      //Set the current bounds to whichever left marker is
      //greater and whichever right marker is less:
      mCurT0 = mT0 < trackStart? trackStart: mT0;
      mCurT1 = mT1 > trackEnd? trackEnd: mT1;

      auto range = mStereoInd
         ? TrackList::SingletonRange(track)
         : TrackList::Channels(track);

      // Process only if the right marker is to the right of the left marker
      if (mCurT1 > mCurT0) {
         auto trackName = track->GetName();

         float extent;
         // Will compute a maximum
         extent = std::numeric_limits<float>::lowest();
         std::vector<float> offsets;

         auto msg = (range.size() == 1)
            // mono or 'stereo tracks independently'
            ? topMsg +
               XO("Analyzing: %s").Format( trackName )
            : topMsg +
               // TODO: more-than-two-channels-message
               XO("Analyzing first track of stereo pair: %s").Format( trackName );
         
         // Analysis loop over channels collects offsets and extent
         for (auto channel : range) {
            float offset = 0;
            float extent2 = 0;
            bGoodResult =
               AnalyseTrack( channel, msg, count++, offset, extent2 );
            if ( ! bGoodResult )
               goto break2;
            extent = std::max( extent, extent2 );
            offsets.push_back(offset);
            // TODO: more-than-two-channels-message
            msg = topMsg +
               XO("Analyzing second track of stereo pair: %s").Format( trackName );
         }

         // Compute the multiplier using extent
         if( (extent > 0) && mGain ) {
            mMult = ratio / extent;
         }
         else
            mMult = 1.0;

         if (range.size() == 1) {
            if (TrackList::Channels(track).size() == 1)
               // really mono
               msg = topMsg +
                  XO("Processing: %s").Format( trackName );
            else
               //'stereo tracks independently'
               // TODO: more-than-two-channels-message
               msg = topMsg +
                  XO("Processing stereo channels independently: %s").Format( trackName );
         }
         else
            msg = topMsg +
               // TODO: more-than-two-channels-message
               XO("Processing first track of stereo pair: %s").Format( trackName );

         // Use multiplier in the second, processing loop over channels
         auto pOffset = offsets.begin();
         for (auto channel : range) {
            if (false ==
                (bGoodResult = ProcessOne(channel, msg, count++, *pOffset++)) )
               goto break2;
            // TODO: more-than-two-channels-message
            msg = topMsg +
               XO("Processing second track of stereo pair: %s").Format( trackName );
         }
      }
   }

   break2:

   this->ReplaceProcessedTracks(bGoodResult);
   return bGoodResult;
}

void EffectNormalize::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   const auto pState = S.GetValidationState();
   const auto enabler = [this]{ return mGain; };

   mCreating = true;

   S.StartVerticalLay(0);
   {
      S.StartMultiColumn(2, wxALIGN_CENTER);
      {
         S.StartVerticalLay(false);
         {
            S
               .Target( mDC )
               .AddCheckBox(XXO("&Remove DC offset (center on 0.0 vertically)"),
                                        mDC);

            S.StartHorizontalLay(wxALIGN_LEFT, false);
            {
               S
                  .MinSize()
                  .Target( mGain )
                  .AddCheckBox(XXO("&Normalize peak amplitude to   "),
                     mGain);

               S
                  .Text(XO("Peak amplitude dB"))
                  .Target( mPeakLevel,
                     NumValidatorStyle::ONE_TRAILING_ZERO, 2,
                     PeakLevel.min,
                     PeakLevel.max )
                  .Enable( enabler )
                  .AddTextBox( {}, L"", 10);

               S
                  .Enable( enabler )
                  .AddVariableText(XO("dB"), false,
                     wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

               // Warning label when the text boxes aren't okay
               S
                  .VariableText( [pState]{ return Label(
                     // TODO: recalculate layout here
                     pState->Ok()
                        ? TranslatableString{}
                        : XO("(Maximum 0dB)") ); } )
                  .AddVariableText( {}, false,
                     wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
            }
            S.EndHorizontalLay();

            S
               .Target( mStereoInd )
               .Enable( enabler )
               .AddCheckBox(XXO("N&ormalize stereo channels independently"),
                                               mStereoInd);
         }
         S.EndVerticalLay();
      }
      S.EndMultiColumn();
   }
   S.EndVerticalLay();
   mCreating = false;
}

// EffectNormalize implementation

bool EffectNormalize::AnalyseTrack(const WaveTrack * track, const TranslatableString &msg,
                                   unsigned count, float &offset, float &extent)
{
   bool result = true;
   float min, max;

   if(mGain)
   {
      // set mMin, mMax.  No progress bar here as it's fast.
      auto pair = track->GetMinMax(mCurT0, mCurT1); // may throw
      min = pair.first, max = pair.second;

      if(mDC)
      {
         result = AnalyseTrackData(track, msg, count, offset);
         min += offset;
         max += offset;
      }
   }
   else if(mDC)
   {
      min = -1.0, max = 1.0;   // sensible defaults?
      result = AnalyseTrackData(track, msg, count, offset);
      min += offset;
      max += offset;
   }
   else
   {
      wxFAIL_MSG("Analysing Track when nothing to do!");
      min = -1.0, max = 1.0;   // sensible defaults?
      offset = 0.0;
   }
   extent = fmax(fabs(min), fabs(max));

   return result;
}

//AnalyseTrackData() takes a track, transforms it to bunch of buffer-blocks,
//and executes selected AnalyseOperation on it...
bool EffectNormalize::AnalyseTrackData(const WaveTrack * track, const TranslatableString &msg,
                                unsigned count, float &offset)
{
   //Transform the marker timepoints to samples
   auto start = track->TimeToLongSamples(mCurT0);
   auto end = track->TimeToLongSamples(mCurT1);

   //Get the length of the buffer (as double). len is
   //used simply to calculate a progress meter, so it is easier
   //to make it a double now than it is to do it later
   auto len = (end - start).as_double();

   mSum   = 0.0; // dc offset inits

   size_t blockSamples;
   sampleCount totalSamples = 0;

   bool rc = ForEachBlock({ track }, start, end, 0,
   [&]( sampleCount s, size_t block, float *const *buffers, size_t blockSamples ){
      totalSamples += blockSamples;
      AnalyseDataDC(buffers[0], block);
      return true;
   }, count, 2 * GetNumWaveTracks(), msg);
   if( totalSamples > 0 )
      offset = -mSum / totalSamples.as_double();  // calculate actual offset (amount that needs to be added on)
   else
      offset = 0.0;

   //Return true because the effect processing succeeded ... unless cancelled
   return rc;
}

//ProcessOne() takes a track, transforms it to bunch of buffer-blocks,
//and executes ProcessData, on it...
// uses mMult and offset to normalize a track.
// mMult must be set before this is called
bool EffectNormalize::ProcessOne(
   WaveTrack * track, const TranslatableString &msg, unsigned count, float offset)
{
   //Transform the marker timepoints to samples
   auto start = track->TimeToLongSamples(mCurT0);
   auto end = track->TimeToLongSamples(mCurT1);

   //Get the length of the buffer (as double). len is
   //used simply to calculate a progress meter, so it is easier
   //to make it a double now than it is to do it later
   auto len = (end - start).as_double();

   //Go through the track one buffer at a time. s counts which
   //sample the current buffer starts at.
   bool rc = InPlaceTransformBlocks({ track }, start, end, 0,
   [&]( sampleCount s, size_t block, float *const *buffers, size_t ){
      ProcessData(buffers[0], block, offset);
      return true;
   }, count, 2 * GetNumWaveTracks(), msg );

   //Return true because the effect processing succeeded ... unless cancelled
   return rc;
}

/// @see AnalyseDataLoudnessDC
void EffectNormalize::AnalyseDataDC(float *buffer, size_t len)
{
   for(decltype(len) i = 0; i < len; i++)
      mSum += (double)buffer[i];
}

void EffectNormalize::ProcessData(float *buffer, size_t len, float offset)
{
   for(decltype(len) i = 0; i < len; i++) {
      float adjFrame = (buffer[i] + offset) * mMult;
      buffer[i] = adjFrame;
   }
}

bool EffectNormalize::CanApply()
{
   return mGain || mDC;
}
