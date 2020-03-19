/**********************************************************************

  Audacity: A Digital Audio Editor

  Loudness.h

  Max Maisel (based on Normalize effect)

**********************************************************************/

#ifndef __AUDACITY_EFFECT_LOUDNESS__
#define __AUDACITY_EFFECT_LOUDNESS__

#include "Effect.h"
#include "Biquad.h"
#include "EBUR128.h"
#include "../Shuttle.h"

class ShuttleGui;

class EffectLoudness final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectLoudness();
   virtual ~EffectLoudness();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // Effect implementation

   bool CheckWhetherSkipEffect() override;
   bool Startup() override;
   bool Process() override;
   void PopulateOrExchange(ShuttleGui & S) override;

private:
   // EffectLoudness implementation

   void FindBufferCapacity();
   bool GetTrackRMS(WaveTrack* track, float& rms);
   bool ProcessOne(TrackIterRange<WaveTrack> range, bool analyse);

   void AnalyseBufferBlock( size_t blockLen, float *const *buffers );
   void ProcessBufferBlock( size_t blockLen, float *const *buffers );

private:
   bool   mStereoInd;
   double mLUFSLevel;
   double mRMSLevel;
   bool   mDualMono;
   int    mNormalizeTo;

   double mCurT0;
   double mCurT1;
   unsigned mTrackCount;
   int    mSteps;
   TranslatableString mProgressMsg;
   double mCurRate;

   float  mMult;
   float  mRatio;
   float  mRMS[2];
   std::unique_ptr<EBUR128> mLoudnessProcessor;

   size_t mTrackBufferCapacity;
   bool   mProcStereo;

   CapturedParameters mParameters;
   CapturedParameters& Parameters() override { return mParameters; }
};

#endif
