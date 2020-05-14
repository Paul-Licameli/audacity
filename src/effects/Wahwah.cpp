/**********************************************************************

  Audacity: A Digital Audio Editor

  Wahwah.cpp

  Effect programming:
  Nasca Octavian Paul (Paul Nasca)

  UI programming:
  Dominic Mazzoni (with the help of wxDesigner)
  Vaughan Johnson (Preview)

*******************************************************************//**

\class EffectWahwah
\brief An Effect that adds a 'spectral glide'.

*//*******************************************************************/


#include "Wahwah.h"
#include "LoadEffects.h"

#include <math.h>

#include <wx/slider.h>

#include "../ShuttleGui.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key               Def      Min      Max      Scale
static auto Freq = Parameter<double>(
                           L"Freq",       1.5,     0.1,     4.0,     10  );
static auto Phase = Parameter<double>(
                           L"Phase",      0.0,     0.0,     360.0,   1   );
static auto Depth = Parameter<int>(
                           L"Depth",      70,      0,       100,     1   ); // scaled to 0-1 before processing
static auto Res = Parameter<double>(
                           L"Resonance",  2.5,     0.1,     10.0,    10  );
static auto FreqOfs = Parameter<int>(
                           L"Offset",     30,      0,       100,     1   ); // scaled to 0-1 before processing
static auto OutGain = Parameter<double>(
                           L"Gain",      -6.0,    -30.0,    30.0,    1   );

// How many samples are processed before recomputing the lfo value again
#define lfoskipsamples 30

//
// EffectWahwah
//

const ComponentInterfaceSymbol EffectWahwah::Symbol
{ XO("Wahwah") };

namespace{ BuiltinEffectsModule::Registration< EffectWahwah > reg; }

EffectWahwah::EffectWahwah()
   :mParameters{
      mFreq, Freq,
      mPhase, Phase,
      mDepth, Depth,
      mRes, Res,
      mFreqOfs, FreqOfs,
      mOutGain, OutGain,
   }
{
   Parameters().Reset();
   SetLinearEffectFlag(true);
}

EffectWahwah::~EffectWahwah()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectWahwah::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectWahwah::GetDescription()
{
   return XO("Rapid tone quality variations, like that guitar sound so popular in the 1970's");
}

wxString EffectWahwah::ManualPage()
{
   return L"Wahwah";
}

// EffectDefinitionInterface implementation

EffectType EffectWahwah::GetType()
{
   return EffectTypeProcess;
}

bool EffectWahwah::SupportsRealtime()
{
#if defined(EXPERIMENTAL_REALTIME_AUDACITY_EFFECTS)
   return true;
#else
   return false;
#endif
}

// EffectClientInterface implementation

unsigned EffectWahwah::GetAudioInCount()
{
   return 1;
}

unsigned EffectWahwah::GetAudioOutCount()
{
   return 1;
}

bool EffectWahwah::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames chanMap)
{
   InstanceInit(mMaster, mSampleRate);

   if (chanMap[0] == ChannelNameFrontRight)
   {
      mMaster.phase += M_PI;
   }

   return true;
}

size_t EffectWahwah::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return InstanceProcess(mMaster, inBlock, outBlock, blockLen);
}

bool EffectWahwah::RealtimeInitialize()
{
   SetBlockSize(512);

   mSlaves.clear();

   return true;
}

bool EffectWahwah::RealtimeAddProcessor(unsigned WXUNUSED(numChannels), float sampleRate)
{
   EffectWahwahState slave;

   InstanceInit(slave, sampleRate);

   mSlaves.push_back(slave);

   return true;
}

bool EffectWahwah::RealtimeFinalize()
{
   mSlaves.clear();

   return true;
}

size_t EffectWahwah::RealtimeProcess(int group,
                                          float **inbuf,
                                          float **outbuf,
                                          size_t numSamples)
{

   return InstanceProcess(mSlaves[group], inbuf, outbuf, numSamples);
}

// Effect implementation

void EffectWahwah::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.AddSpace(0, 5);

   S.StartMultiColumn(3, GroupOptions{ wxEXPAND }.StretchyColumn(2));
   {
      S
         .Target( mFreq,
            NumValidatorStyle::ONE_TRAILING_ZERO, 5, Freq.min, Freq.max )
         .AddTextBox(XXO("LFO Freq&uency (Hz):"), L"", 12);

      S
         .Text(XO("LFO frequency in hertz"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mFreq, Freq.scale ) )
         .AddSlider( {}, Freq.def * Freq.scale, Freq.max * Freq.scale, Freq.min * Freq.scale);

      S
         .Target( mPhase, NumValidatorStyle::DEFAULT, 1, Phase.min, Phase.max )
         .AddTextBox(XXO("LFO Sta&rt Phase (deg.):"), L"", 12);

      S
         .Text(XO("LFO start phase in degrees"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Transform( mPhase,
            []( double output ){ return output * Phase.scale; },
            []( double input ){
               // round to nearest multiple of 10
               auto rounded = ( ( input + 5 ) / 10 ) * 10;
               return std::min( Phase.max, rounded / Phase.scale ); } ) )
         .AddSlider( {}, Phase.def * Phase.scale, Phase.max * Phase.scale, Phase.min * Phase.scale,
            10 /* line size */ );

      S
         .Target( mDepth, NumValidatorStyle::DEFAULT, Depth.min, Depth.max )
         .AddTextBox(XXO("Dept&h (%):"), L"", 12);

      S
         .Text(XO("Depth in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mDepth, Depth.scale ) )
         .AddSlider( {}, Depth.def * Depth.scale, Depth.max * Depth.scale, Depth.min * Depth.scale);

      S
         .Target( mRes, NumValidatorStyle::DEFAULT, 1, Res.min, Res.max )
         .AddTextBox(XXO("Reso&nance:"), L"", 12);

      S
         .Text(XO("Resonance"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mRes, Res.scale ) )
         .AddSlider( {}, Res.def * Res.scale, Res.max * Res.scale, Res.min * Res.scale);

      S
         .Target( mFreqOfs,
            NumValidatorStyle::DEFAULT, FreqOfs.min, FreqOfs.max )
         .AddTextBox(XXO("Wah Frequency Offse&t (%):"), L"", 12);

      S
         .Text(XO("Wah frequency offset in percent"))
         .Style(wxSL_HORIZONTAL)
         .MinSize( { 100, -1 } )
         .Target( Scale( mFreqOfs, FreqOfs.scale ) )
         .AddSlider( {}, FreqOfs.def * FreqOfs.scale, FreqOfs.max * FreqOfs.scale, FreqOfs.min * FreqOfs.scale);

      S
         .Target( mOutGain,
            NumValidatorStyle::DEFAULT, 1, OutGain.min, OutGain.max )
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

// EffectWahwah implementation

void EffectWahwah::InstanceInit(EffectWahwahState & data, float sampleRate)
{
   data.samplerate = sampleRate;
   data.lfoskip = mFreq * 2 * M_PI / sampleRate;
   data.skipcount = 0;
   data.xn1 = 0;
   data.xn2 = 0;
   data.yn1 = 0;
   data.yn2 = 0;
   data.b0 = 0;
   data.b1 = 0;
   data.b2 = 0;
   data.a0 = 0;
   data.a1 = 0;
   data.a2 = 0;

   data.depth = mDepth / 100.0;
   data.freqofs = mFreqOfs / 100.0;
   data.phase = mPhase * M_PI / 180.0;
   data.outgain = DB_TO_LINEAR(mOutGain);
}

size_t EffectWahwah::InstanceProcess(EffectWahwahState & data, float **inBlock, float **outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];
   double frequency, omega, sn, cs, alpha;
   double in, out;

   data.lfoskip = mFreq * 2 * M_PI / data.samplerate;
   data.depth = mDepth / 100.0;
   data.freqofs = mFreqOfs / 100.0;

   data.phase = mPhase * M_PI / 180.0;
   data.outgain = DB_TO_LINEAR(mOutGain);

   for (decltype(blockLen) i = 0; i < blockLen; i++)
   {
      in = (double) ibuf[i];

      if ((data.skipcount++) % lfoskipsamples == 0)
      {
         frequency = (1 + cos(data.skipcount * data.lfoskip + data.phase)) / 2;
         frequency = frequency * data.depth * (1 - data.freqofs) + data.freqofs;
         frequency = exp((frequency - 1) * 6);
         omega = M_PI * frequency;
         sn = sin(omega);
         cs = cos(omega);
         alpha = sn / (2 * mRes);
         data.b0 = (1 - cs) / 2;
         data.b1 = 1 - cs;
         data.b2 = (1 - cs) / 2;
         data.a0 = 1 + alpha;
         data.a1 = -2 * cs;
         data.a2 = 1 - alpha;
      };
      out = (data.b0 * in + data.b1 * data.xn1 + data.b2 * data.xn2 - data.a1 * data.yn1 - data.a2 * data.yn2) / data.a0;
      data.xn2 = data.xn1;
      data.xn1 = in;
      data.yn2 = data.yn1;
      data.yn1 = out;
      out *= data.outgain;

      obuf[i] = (float) out;
   }

   return blockLen;
}
