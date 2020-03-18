/**********************************************************************

  Audacity: A Digital Audio Editor

  Phaser

  Effect programming:
  Nasca Octavian Paul (Paul Nasca)

  UI programming:
  Dominic Mazzoni (with the help of wxDesigner)
  Vaughan Johnson (Preview)

**********************************************************************/

#ifndef __AUDACITY_EFFECT_PHASER__
#define __AUDACITY_EFFECT_PHASER__

#include "Effect.h"
#include "../Shuttle.h"

class ShuttleGui;

#define NUM_STAGES 24

class EffectPhaserState
{
public:
   // state variables
   float samplerate;
   sampleCount skipcount;
   double old[NUM_STAGES]; // must be as large as MAX_STAGES
   double gain;
   double fbout;
   double outgain;
   double lfoskip;
   double phase;
   int laststages;
};

class EffectPhaser final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectPhaser();
   virtual ~EffectPhaser();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   bool SupportsRealtime() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;
   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;
   bool RealtimeInitialize() override;
   bool RealtimeAddProcessor(unsigned numChannels, float sampleRate) override;
   bool RealtimeFinalize() override;
   size_t RealtimeProcess(int group,
                                       float **inbuf,
                                       float **outbuf,
                                       size_t numSamples) override;

   // Effect implementation

   void PopulateOrExchange(ShuttleGui & S) override;

private:
   // EffectPhaser implementation

   void InstanceInit(EffectPhaserState & data, float sampleRate);
   size_t InstanceProcess(EffectPhaserState & data, float **inBlock, float **outBlock, size_t blockLen);

/*
    Phaser Parameters

 mFreq       - Phaser's LFO frequency
 mPhase      - Phaser's LFO startphase (radians), needed for stereo Phasers
 mDepth      - Phaser depth (0 - no depth, 255 - max depth)
 mStages     - Phaser stages (recomanded from 2 to 16-24, and EVEN NUMBER)
 mDryWet     - Dry/wet mix, (0 - dry, 128 - dry=wet, 255 - wet)
 mFeedback   - Phaser FeedBack (0 - no feedback, 100 = 100% Feedback,
                               -100 = -100% FeedBack)
*/

private:
   EffectPhaserState mMaster;
   std::vector<EffectPhaserState> mSlaves;

   // parameters
   int mStages;
   int mDryWet;
   double mFreq;
   double mPhase;
   int mDepth;
   int mFeedback;
   double mOutGain;

   CapturedParameters mParameters;
   CapturedParameters& Parameters() override { return mParameters; }
};

#endif
