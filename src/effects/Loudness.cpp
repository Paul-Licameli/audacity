/**********************************************************************

  Audacity: A Digital Audio Editor

  Loudness.cpp

  Max Maisel

*******************************************************************//**

\class EffectLoudness
\brief An Effect to bring the loudness level up to a chosen level.

*//*******************************************************************/



#include "Loudness.h"

#include <math.h>

#include "Internat.h"
#include "../Prefs.h"
#include "../ProjectFileManager.h"
#include "../ShuttleGui.h"
#include "../WaveTrack.h"
#include "../widgets/ProgressDialog.h"

#include "LoadEffects.h"

enum kNormalizeTargets
{
   kLoudness,
   kRMS,
   nAlgos
};

static const EnumValueSymbol kNormalizeTargetStrings[nAlgos] =
{
   { XO("perceived loudness") },
   { XO("RMS") }
};
// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name         Type     Key                        Def         Min      Max       Scale
static auto StereoInd = Parameter<bool>(
                           L"StereoIndependent",   false,      false,   true,     1  );
static auto LUFSLevel = Parameter<double>(
                           L"LUFSLevel",           -23.0,      -145.0,  0.0,      1  );
static auto RMSLevel = Parameter<double>(
                           L"RMSLevel",            -20.0,      -145.0,  0.0,      1  );
static auto DualMono = Parameter<bool>(
                           L"DualMono",            true,       false,   true,     1  );
static auto NormalizeTo = Parameter<int>(
                           L"NormalizeTo",         kLoudness , 0    ,   nAlgos-1, 1  );

const ComponentInterfaceSymbol EffectLoudness::Symbol
{ XO("Loudness Normalization") };

namespace{ BuiltinEffectsModule::Registration< EffectLoudness > reg; }

EffectLoudness::EffectLoudness()
   : mParameters{
      mStereoInd, StereoInd,
      mLUFSLevel, LUFSLevel,
      mRMSLevel, RMSLevel,
      mDualMono, DualMono,
      mNormalizeTo, NormalizeTo,
   }
{
   Parameters().Reset();
   SetLinearEffectFlag(false);
}

EffectLoudness::~EffectLoudness()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectLoudness::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectLoudness::GetDescription()
{
   return XO("Sets the loudness of one or more tracks");
}

wxString EffectLoudness::ManualPage()
{
   return L"Loudness_Normalization";
}

// EffectDefinitionInterface implementation

EffectType EffectLoudness::GetType()
{
   return EffectTypeProcess;
}

// Effect implementation

bool EffectLoudness::CheckWhetherSkipEffect()
{
   return false;
}

bool EffectLoudness::Startup()
{
   wxString base = L"/Effects/Loudness/";
   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      mStereoInd = false;
      mDualMono = DualMono.def;
      mNormalizeTo = kLoudness;
      mLUFSLevel = LUFSLevel.def;
      mRMSLevel = RMSLevel.def;

      SaveUserPreset(GetCurrentSettingsGroup());

      gPrefs->Flush();
   }
   return true;
}

bool EffectLoudness::Process()
{
   if(mNormalizeTo == kLoudness)
      // LU use 10*log10(...) instead of 20*log10(...)
      // so multiply level by 2 and use standard DB_TO_LINEAR macro.
      mRatio = DB_TO_LINEAR(TrapDouble(mLUFSLevel*2, LUFSLevel.min, LUFSLevel.max));
   else // RMS
      mRatio = DB_TO_LINEAR(TrapDouble(mRMSLevel, RMSLevel.min, RMSLevel.max));

   // Iterate over each track
   this->CopyInputTracks(); // Set up mOutputTracks.
   bool bGoodResult = true;
   auto topMsg = XO("Normalizing Loudness...\n");

   FindBufferCapacity();

   for(auto track : mOutputTracks->Selected<WaveTrack>()
       + (mStereoInd ? &Track::Any : &Track::IsLeader))
   {
      // Get start and end times from track
      // PRL: No accounting for multiple channels ?
      double trackStart = track->GetStartTime();
      double trackEnd = track->GetEndTime();

      // Set the current bounds to whichever left marker is
      // greater and whichever right marker is less:
      mCurT0 = mT0 < trackStart? trackStart: mT0;
      mCurT1 = mT1 > trackEnd? trackEnd: mT1;

      // Get the track rate
      mCurRate = track->GetRate();

      wxString msg;
      auto trackName = track->GetName();
      mSteps = 2;

      mProgressMsg =
         topMsg + XO("Analyzing: %s").Format( trackName );

      auto range = mStereoInd
         ? TrackList::SingletonRange(track)
         : TrackList::Channels(track);

      mProcStereo = range.size() > 1;

      if(mNormalizeTo == kLoudness)
      {
         mLoudnessProcessor.reset(safenew EBUR128(mCurRate, range.size()));
         mLoudnessProcessor->Initialize();
         if(!ProcessOne(range, true))
         {
            // Processing failed -> abort
            bGoodResult = false;
            break;
         }
      }
      else // RMS
      {
         size_t idx = 0;
         for(auto channel : range)
         {
            if(!GetTrackRMS(channel, mRMS[idx]))
            {
               bGoodResult = false;
               return false;
            }
            ++idx;
         }
         mSteps = 1;
      }

      // Calculate normalization values the analysis results
      float extent;
      if(mNormalizeTo == kLoudness)
         extent = mLoudnessProcessor->IntegrativeLoudness();
      else // RMS
      {
         extent = mRMS[0];
         if(mProcStereo)
            // RMS: use average RMS, average must be calculated in quadratic domain.
            extent = sqrt((mRMS[0] * mRMS[0] + mRMS[1] * mRMS[1]) / 2.0);
      }

      if(extent == 0.0)
      {
         mLoudnessProcessor.reset();
         return false;
      }
      mMult = mRatio / extent;

      if(mNormalizeTo == kLoudness)
      {
         // Target half the LUFS value if mono (or independent processed stereo)
         // shall be treated as dual mono.
         if(range.size() == 1 && (mDualMono || track->GetChannel() != Track::MonoChannel))
            mMult /= 2.0;

         // LUFS are related to square values so the multiplier must be the root.
         mMult = sqrt(mMult);
      }

      mProgressMsg = topMsg + XO("Processing: %s").Format( trackName );
      if(!ProcessOne(range, false))
      {
         // Processing failed -> abort
         bGoodResult = false;
         break;
      }
   }

   this->ReplaceProcessedTracks(bGoodResult);
   mLoudnessProcessor.reset();
   return bGoodResult;
}

void EffectLoudness::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   auto pState = S.GetValidationState();

   S.StartVerticalLay(0);
   {
      S.StartMultiColumn(2, wxALIGN_CENTER);
      {
         S.StartVerticalLay(false);
         {
            S.StartHorizontalLay(wxALIGN_LEFT, false);
            {
               S
                  .AddVariableText(XO("&Normalize"), false,
                     wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

               S
                  .Target( mNormalizeTo )
                  .AddChoice( {},
                     Msgids(kNormalizeTargetStrings, nAlgos),
                     mNormalizeTo );

               S
                  .AddVariableText(XO("t&o"), false,
                     wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

               // Use a notebook so we can have two controls but show only one
               // They target different variables with their validators
               S
                  .Target( mNormalizeTo )
                  .StartSimplebook();
               {
                  S.StartNotebookPage({});
                  {
                     S.StartHorizontalLay(wxALIGN_LEFT, false);
                     {
                        S
                           /* i18n-hint: LUFS is a particular method for measuring loudnesss */
                           .Text( XO("Loudness LUFS") )
                           .Target( mLUFSLevel,
                              NumValidatorStyle::ONE_TRAILING_ZERO, 2,
                              LUFSLevel.min, LUFSLevel.max )
                           .AddTextBox( {}, L"", 10);

                        /* i18n-hint: LUFS is a particular method for measuring loudnesss */
                        S
                           .AddVariableText(XO("LUFS"), false,
                              wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
                     }
                     S.EndHorizontalLay();
                  }
                  S.EndNotebookPage();

                  S.StartNotebookPage({});
                  {
                     S.StartHorizontalLay(wxALIGN_LEFT, false);
                     {
                        S
                           .Text( XO("RMS dB") )
                           .Target( mRMSLevel,
                              NumValidatorStyle::ONE_TRAILING_ZERO, 2,
                              RMSLevel.min, RMSLevel.max )
                           .AddTextBox( {}, L"", 10);

                        S
                           .AddVariableText(XO("dB"), false,
                              wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);
                     }
                     S.EndHorizontalLay();
                  }
                  S.EndNotebookPage();
               }
               S.EndSimplebook();

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
               .AddCheckBox(XXO("Normalize &stereo channels independently"),
                  mStereoInd );

            S
               .Target( mDualMono )
               .Enable( [this]{ return mNormalizeTo == kLoudness; } )
               .AddCheckBox(XXO("&Treat mono as dual-mono (recommended)"),
                  mDualMono );
         }
         S.EndVerticalLay();
      }
      S.EndMultiColumn();
   }
   S.EndVerticalLay();
}

// EffectLoudness implementation

/// Get required buffer size for the largest whole track and allocate buffers.
/// This reduces the amount of allocations required.
void EffectLoudness::FindBufferCapacity()
{
   mTrackBufferCapacity = 0;
   bool stereoTrackFound = false;
   double maxSampleRate = 0;
   mProcStereo = false;

   for(auto track : mOutputTracks->Selected<WaveTrack>() + &Track::Any)
   {
      mTrackBufferCapacity = std::max(mTrackBufferCapacity, track->GetMaxBlockSize());
      maxSampleRate = std::max(maxSampleRate, track->GetRate());

      // There is a stereo track
      if(track->IsLeader())
         stereoTrackFound = true;
   }

   mTrackCount = 0;

}

bool EffectLoudness::GetTrackRMS(WaveTrack* track, float& rms)
{
   // set mRMS.  No progress bar here as it's fast.
   float _rms = track->GetRMS(mCurT0, mCurT1); // may throw
   rms = _rms;
   return true;
}

/// ProcessOne() takes a track, transforms it to bunch of buffer-blocks,
/// and executes ProcessData, on it...
///  uses mMult to normalize a track.
///  mMult must be set before this is called
/// In analyse mode, it executes the selected analyse operation on it...
///  mMult does not have to be set before this is called
bool EffectLoudness::ProcessOne(TrackIterRange<WaveTrack> range, bool analyse)
{
   WaveTrack* track = *range.begin();

   // Transform the marker timepoints to samples
   auto start = track->TimeToLongSamples(mCurT0);
   auto end   = track->TimeToLongSamples(mCurT1);

   // Abort if the right marker is not to the right of the left marker
   if(mCurT1 <= mCurT0)
      return false;

   // Go through the track one buffer at a time. s counts which
   // sample the current buffer starts at.
   auto result = analyse
      ? ForEachBlock( { range.begin(), range.end() },
         start, end, mTrackBufferCapacity,
      [&]( sampleCount s, size_t blockLen, float *const *buffers, size_t ) {
         AnalyseBufferBlock( blockLen, buffers );
         return true;
      }, mTrackCount, GetNumWaveTracks() * mSteps, mProgressMsg )

      : InPlaceTransformBlocks(
         { range.begin(), range.end() },
         start, end, mTrackBufferCapacity,
      [&]( sampleCount s, size_t blockLen, float *const *buffers, size_t ) {
         ProcessBufferBlock( blockLen, buffers );
         return true;
      }, mTrackCount, GetNumWaveTracks() * mSteps, mProgressMsg )
   ;
   mTrackCount += range.size();
   return result;
}

/// Calculates sample sum (for DC) and EBU R128 weighted square sum
/// (for loudness).
void EffectLoudness::AnalyseBufferBlock( size_t blockLen, float *const *buffers )
{
   for(size_t i = 0; i < blockLen; i++)
   {
      mLoudnessProcessor->ProcessSampleFromChannel(buffers[0][i], 0);
      if(mProcStereo)
         mLoudnessProcessor->ProcessSampleFromChannel(buffers[1][i], 1);
      mLoudnessProcessor->NextSample();
   }
}

void EffectLoudness::ProcessBufferBlock( size_t blockLen, float *const *buffers )
{
   for(size_t i = 0; i < blockLen; i++)
   {
      buffers[0][i] *= mMult;
      if(mProcStereo)
         buffers[1][i] *= mMult;
   }
}

