/**********************************************************************

  Audacity: A Digital Audio Editor

  Phaser.cpp

  Effect programming:
  Nasca Octavian Paul (Paul Nasca)

  UI programming:
  Dominic Mazzoni (with the help of wxDesigner)
  Vaughan Johnson (Preview)

*******************************************************************//**

\class EffectPhaser
\brief An Effect that changes frequencies in a time varying manner.

*//*******************************************************************/



#include "Phaser.h"
#include "LoadEffects.h"

#include <math.h>

#include "../ShuttleGui.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key               Def   Min   Max         Scale
static auto Stages = Parameter<int>(
                           L"Stages",     2,    2,    NUM_STAGES, 1  );
static auto DryWet = Parameter<int>(
                           L"DryWet",     128,  0,    255,        1  );
static auto Freq = Parameter<double>(
                           L"Freq",       0.4,  0.001,4.0,        10.0 );
static auto Phase = Parameter<double>(
                           L"Phase",      0.0,  0.0,  360.0,      1  );
static auto Depth = Parameter<int>(
                           L"Depth",      100,  0,    255,        1  );
static auto Feedback = Parameter<int>(
                           L"Feedback",   0,    -100, 100,        1  );
static auto OutGain = Parameter<double>(
                           L"Gain",      -6.0,    -30.0,    30.0,    1   );

//
#define phaserlfoshape 4.0

// How many samples are processed before recomputing the lfo value again
#define lfoskipsamples 20

//
// EffectPhaser
//

const ComponentInterfaceSymbol EffectPhaser::Symbol
{ XO("Phaser") };

namespace{ BuiltinEffectsModule::Registration< EffectPhaser > reg; }

EffectPhaser::EffectPhaser()
   :mParameters {
      [this]{ mStages &= ~1; // must be even, but don't complain about it
         return true; },
      mStages,    Stages,
      mDryWet,    DryWet,
      mFreq,      Freq,
      mPhase,     Phase,
      mDepth,     Depth,
      mFeedback,  Feedback,
      mOutGain,   OutGain
   }
{
   Parameters().Reset();
   SetLinearEffectFlag(true);
}

EffectPhaser::~EffectPhaser()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectPhaser::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectPhaser::GetDescription()
{
   return XO("Combines phase-shifted signals with the original signal");
}

wxString EffectPhaser::ManualPage()
{
   return L"Phaser";
}

// EffectDefinitionInterface implementation

EffectType EffectPhaser::GetType()
{
   return EffectTypeProcess;
}

bool EffectPhaser::SupportsRealtime()
{
#if defined(EXPERIMENTAL_REALTIME_AUDACITY_EFFECTS)
   return true;
#else
   return false;
#endif
}

// EffectClientInterface implementation

unsigned EffectPhaser::GetAudioInCount()
{
   return 1;
}

unsigned EffectPhaser::GetAudioOutCount()
{
   return 1;
}

bool EffectPhaser::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames chanMap)
{
   InstanceInit(mMaster, mSampleRate);
   if (chanMap[0] == ChannelNameFrontRight)
   {
      mMaster.phase += M_PI;
   }

   return true;
}

size_t EffectPhaser::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return InstanceProcess(mMaster, inBlock, outBlock, blockLen);
}

bool EffectPhaser::RealtimeInitialize()
{
   SetBlockSize(512);

   mSlaves.clear();

   return true;
}

bool EffectPhaser::RealtimeAddProcessor(unsigned WXUNUSED(numChannels), float sampleRate)
{
   EffectPhaserState slave;

   InstanceInit(slave, sampleRate);

   mSlaves.push_back(slave);

   return true;
}

bool EffectPhaser::RealtimeFinalize()
{
   mSlaves.clear();

   return true;
}

size_t EffectPhaser::RealtimeProcess(int group,
                                          float **inbuf,
                                          float **outbuf,
                                          size_t numSamples)
{

   return InstanceProcess(mSlaves[group], inbuf, outbuf, numSamples);
}
// Effect implementation

void EffectPhaser::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;

   const    auto stagesTarget = Transform( mStages,
      [](int output){ return output; },
      [](int input){ return input & ~1; /* must be even */ } );

   S.SetBorder(5);
   S.AddSpace(0, 5);

   S.StartMultiColumn(3, GroupOptions{ wxEXPAND }.StretchyColumn(2));
   {
      S
         .Target( stagesTarget,
            NumValidatorStyle::DEFAULT, Stages.min, Stages.max)
         .AddTextBox(XXO("&Stages:"), L"", 15);

      S
         .Text(XO("Stages"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( stagesTarget )
         .AddSlider( {}, Stages.def * Stages.scale, Stages.max * Stages.scale, Stages.min * Stages.scale,
            2 /* line size */ );

      S
         .Target( mDryWet, NumValidatorStyle::DEFAULT, DryWet.min, DryWet.max )
         .AddTextBox(XXO("&Dry/Wet:"), L"", 15);

      S
         .Text(XO("Dry Wet"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mDryWet, DryWet.scale ) )
         .AddSlider( {}, DryWet.def * DryWet.scale, DryWet.max * DryWet.scale, DryWet.min * DryWet.scale);

      S
         .Target( mFreq,
            NumValidatorStyle::ONE_TRAILING_ZERO, 5, Freq.min, Freq.max)
         .AddTextBox(XXO("LFO Freq&uency (Hz):"), L"", 15);

      S
         .Text(XO("LFO frequency in hertz"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Transform( mFreq,
            [](int output){ return output * Freq.scale; },
            // Is this value clamp on slider input really needed?
            [](int input){ return std::max( Freq.min, input / Freq.scale ); } ) )
         .AddSlider( {}, Freq.def * Freq.scale, Freq.max * Freq.scale, 0.0);

      S
         .Target( mPhase, NumValidatorStyle::DEFAULT, 1, Phase.min, Phase.max )
         .AddTextBox(XXO("LFO Sta&rt Phase (deg.):"), L"", 15);

      S
         .Text(XO("LFO start phase in degrees"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Transform( mPhase,
            [](int output){ return output * Phase.scale; },
            [](int input){
               input = ((input + 5) / 10) * 10; // round to nearest multiple of 10
               return std::min( Phase.max, input / Phase.scale ); } ) )
         .AddSlider( {}, Phase.def * Phase.scale, Phase.max * Phase.scale, Phase.min * Phase.scale,
            10 /* line size */ );

      S
         .Target( mDepth, NumValidatorStyle::DEFAULT, Depth.min, Depth.max )
         .AddTextBox(XXO("Dept&h:"), L"", 15);

      S
         .Text(XO("Depth in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mDepth, Depth.scale ) )
         .AddSlider( {}, Depth.def * Depth.scale, Depth.max * Depth.scale, Depth.min * Depth.scale);

      S
         .Target( mFeedback,
            NumValidatorStyle::DEFAULT, Feedback.min, Feedback.max )
         .AddTextBox(XXO("Feedbac&k (%):"), L"", 15);

      S
         .Text(XO("Feedback in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Transform( mPhase,
            [](int output){ return output * Feedback.scale; },
            [](int input){
//         val = ((val + (val > 0 ? 5 : -5)) / 10) * 10; // round to nearest multiple of 10
               input = ((input + 5) / 10) * 10; // round to nearest multiple of 10
               return std::min( Feedback.max, input / Feedback.scale ); } ) )
         .AddSlider( {}, Feedback.def * Feedback.scale, Feedback.max * Feedback.scale, Feedback.min * Feedback.scale,
            10 /* line size */ );

      S
         .Target( mOutGain,
            NumValidatorStyle::DEFAULT, 1, OutGain.min, OutGain.max)
         .AddTextBox(XXO("&Output gain (dB):"), L"", 12);

      S
         .Text(XO("Output gain (dB)"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mOutGain, OutGain.scale ) )
         .AddSlider( {}, OutGain.def * OutGain.scale, OutGain.max * OutGain.scale, OutGain.min * OutGain.scale);
   }
   S.EndMultiColumn();
}

// EffectPhaser implementation

void EffectPhaser::InstanceInit(EffectPhaserState & data, float sampleRate)
{
   data.samplerate = sampleRate;

   for (int j = 0; j < mStages; j++)
   {
      data.old[j] = 0;
   }

   data.skipcount = 0;
   data.gain = 0;
   data.fbout = 0;
   data.laststages = 0;
   data.outgain = 0;

   return;
}

size_t EffectPhaser::InstanceProcess(EffectPhaserState & data, float **inBlock, float **outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];

   for (int j = data.laststages; j < mStages; j++)
   {
      data.old[j] = 0;
   }
   data.laststages = mStages;

   data.lfoskip = mFreq * 2 * M_PI / data.samplerate;
   data.phase = mPhase * M_PI / 180;
   data.outgain = DB_TO_LINEAR(mOutGain);

   for (decltype(blockLen) i = 0; i < blockLen; i++)
   {
      double in = ibuf[i];

      double m = in + data.fbout * mFeedback / 101;  // Feedback must be less than 100% to avoid infinite gain.

      if (((data.skipcount++) % lfoskipsamples) == 0)
      {
         //compute sine between 0 and 1
         data.gain =
            (1.0 +
             cos(data.skipcount.as_double() * data.lfoskip
                 + data.phase)) / 2.0;

         // change lfo shape
         data.gain = expm1(data.gain * phaserlfoshape) / expm1(phaserlfoshape);

         // attenuate the lfo
         data.gain = 1.0 - data.gain / 255.0 * mDepth;
      }

      // phasing routine
      for (int j = 0; j < mStages; j++)
      {
         double tmp = data.old[j];
         data.old[j] = data.gain * tmp + m;
         m = tmp - data.gain * data.old[j];
      }
      data.fbout = m;

      obuf[i] = (float) (data.outgain * (m * mDryWet + in * (255 - mDryWet)) / 255);
   }

   return blockLen;
}

