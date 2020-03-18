/**********************************************************************

  Audacity: A Digital Audio Editor

  Distortion.h

  Steve Daulton

**********************************************************************/

#ifndef __AUDACITY_EFFECT_DISTORTION__
#define __AUDACITY_EFFECT_DISTORTION__

#include <queue>

#include "Effect.h"
#include "../Shuttle.h"

class ShuttleGui;

#define STEPS 1024      // number of +ve or -ve steps in lookup tabe
#define TABLESIZE 2049  // size of lookup table (steps * 2 + 1)

class EffectDistortionState
{
public:
   float       samplerate;
   sampleCount skipcount;
   int         tablechoiceindx;
   bool        dcblock;
   double      threshold;
   double      noisefloor;
   double      param1;
   double      param2;
   int         repeats;

   // DC block filter variables
   std::queue<float> queuesamples;
   double queuetotal;
};

enum kTableType
{
   kHardClip = 0,
   kSoftClip,
   kHalfSinCurve,
   kExpCurve,
   kLogCurve,
   kCubic,
   kEvenHarmonics,
   kSinCurve,
   kLeveller,
   kRectifier,
   kHardLimiter,
   nTableTypes
};

class EffectDistortion final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectDistortion();
   virtual ~EffectDistortion();

   int    mTableChoiceIndx;

   struct Params
   {
      bool   mDCBlock;
      double mThreshold_dB;
      double mNoiseFloor;
      double mParam1;
      double mParam2;
      int    mRepeats;
   };
   
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
   RegistryPaths GetFactoryPresets() override;
   bool LoadFactoryPreset(int id) override;

   // Effect implementation

   struct UISpec;
   void PopulateCheckboxPage(ShuttleGui &S, const UISpec &spec, size_t index);
   void PopulateThresholdPage(ShuttleGui &S, const UISpec &spec, size_t index);
   void PopulateParameterPage(ShuttleGui &S, const UISpec &spec, size_t index);
   void PopulateOrExchange(ShuttleGui & S) override;
   bool Init() override;
   bool TransferDataFromWindow() override;

private:

   // EffectDistortion implementation

   void InstanceInit(EffectDistortionState & data, float sampleRate);
   size_t InstanceProcess(EffectDistortionState & data,
                               float **inBlock,
                               float **outBlock,
                               size_t blockLen);

   void MakeTable();
   float WaveShaper(float sample);
   float DCFilter(EffectDistortionState & data, float sample);

   // Preset tables for gain lookup

   void HardClip();           // hard clipping
   void SoftClip();           // soft clipping
   void ExponentialTable();   // exponential mapping
   void LogarithmicTable();   // logarithmic mapping
   void HalfSinTable();
   void CubicTable();
   void EvenHarmonicTable();
   void SineTable();
   void Leveller();           // 'Leveller' wavetable is modeled on the legacy effect of the same name.
   void Rectifier();          // 0% = Dry, 50% = half-wave rectified, 100% = full-wave rectified (abs value).
   void HardLimiter();        // Same effect as the LADSPA "hardLimiter 1413"

   // Wavetable helper functions

   void CopyHalfTable();   // for symmetric tables

   // Used by Soft Clipping but could be used for other tables.
   // Log curve formula: y = T + (((e^(RT - Rx)) - 1) / -R)
   // where R is the ratio, T is the threshold, and x is from T to 1. 
   inline float LogCurve(double threshold, float value, double ratio);

   // Used by Cubic curve but could be used for other tables
   // Cubic formula: y = x - (x^3 / 3.0)
   inline double Cubic(double x);


private:
   EffectDistortionState mMaster;
   std::vector<EffectDistortionState> mSlaves;

   double mTable[TABLESIZE];

   // mMakeupGain is used by some distortion types to pass the
   // amount of gain required to bring overall effect gain to unity
   double mMakeupGain;

   int mTypChoiceIndex;

   Params mParams;
   Params mPageParams[ nTableTypes ];

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
};

#endif
