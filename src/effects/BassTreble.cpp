/**********************************************************************

   Audacity: A Digital Audio Editor
   Audacity(R) is copyright (c) 1999-2016 Audacity Team.
   License: GPL v2.  See License.txt.

   BassTreble.cpp
   Steve Daulton

******************************************************************//**

\class EffectBassTreble
\brief A high shelf and low shelf filter.

*//*******************************************************************/


#include "BassTreble.h"
#include "LoadEffects.h"

#include <math.h>
#include <algorithm>

#include <wx/panel.h>

#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../WaveTrack.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key                  Def      Min      Max      Scale
static auto Bass = Parameter<double>(
                           L"Bass",          0.0,     -30.0,   30.0,    1  );
static auto Treble = Parameter<double>(
                           L"Treble",        0.0,     -30.0,   30.0,    1  );
static auto Gain = Parameter<double>(
                           L"Gain",          0.0,     -30.0,   30.0,    1  );
static auto Link = Parameter<bool>(
                           L"Link Sliders",  false,    false,  true,    1  );

// Used to communicate the type of the filter.
enum kShelfType
{
   kBass,
   kTreble
};

const ComponentInterfaceSymbol EffectBassTreble::Symbol
{ XO("Bass and Treble") };

namespace{ BuiltinEffectsModule::Registration< EffectBassTreble > reg; }

EffectBassTreble::EffectBassTreble()
   : mParameters{
      mBass, Bass,
      mTreble, Treble,
      mGain, Gain,
      mLink, Link
   }
{
   Parameters().Reset();

   SetLinearEffectFlag(true);
}

EffectBassTreble::~EffectBassTreble()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectBassTreble::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectBassTreble::GetDescription()
{
   return XO("Simple tone control effect");
}

wxString EffectBassTreble::ManualPage()
{
   return L"Bass_and_Treble";
}

// EffectDefinitionInterface implementation

EffectType EffectBassTreble::GetType()
{
   return EffectTypeProcess;
}

bool EffectBassTreble::SupportsRealtime()
{
#if defined(EXPERIMENTAL_REALTIME_AUDACITY_EFFECTS)
   return true;
#else
   return false;
#endif
}


// EffectClientInterface implementation

unsigned EffectBassTreble::GetAudioInCount()
{
   return 1;
}

unsigned EffectBassTreble::GetAudioOutCount()
{
   return 1;
}

bool EffectBassTreble::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   InstanceInit(mMaster, mSampleRate);

   return true;
}

size_t EffectBassTreble::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return InstanceProcess(mMaster, inBlock, outBlock, blockLen);
}

bool EffectBassTreble::RealtimeInitialize()
{
   SetBlockSize(512);

   mSlaves.clear();

   return true;
}

bool EffectBassTreble::RealtimeAddProcessor(unsigned WXUNUSED(numChannels), float sampleRate)
{
   EffectBassTrebleState slave;

   InstanceInit(slave, sampleRate);

   mSlaves.push_back(slave);

   return true;
}

bool EffectBassTreble::RealtimeFinalize()
{
   mSlaves.clear();

   return true;
}

size_t EffectBassTreble::RealtimeProcess(int group,
                                              float **inbuf,
                                              float **outbuf,
                                              size_t numSamples)
{
   return InstanceProcess(mSlaves[group], inbuf, outbuf, numSamples);
}

bool EffectBassTreble::CheckWhetherSkipEffect()
{
   return (mBass == 0.0 && mTreble == 0.0 && mGain == 0.0);
}


// Effect implementation

void EffectBassTreble::PopulateOrExchange(ShuttleGui & S)
{
   mOldBass = mBass;
   mOldTreble = mTreble;

   using namespace DialogDefinition;

   S.SetBorder(5);
   S.AddSpace(0, 5);

   S.StartStatic(XO("Tone controls"));
   {
      S.StartMultiColumn(3, GroupOptions{ wxEXPAND }.StretchyColumn(2));
      {
         // Bass control
         S
            .Text(XO("Bass (dB):"))
            .Target( mBass, NumValidatorStyle::DEFAULT, 1, Bass.min, Bass.max )
            .Action( [this]{ UpdateGain(); } )
            .AddTextBox(XXO("Ba&ss (dB):"), L"", 10);

         S
            .Text(XO("Bass"))
            .Style(wxSL_HORIZONTAL)
            .Target( Scale( mBass, Bass.scale ) )
            .Action( [this]{ UpdateGain(); } )
            .AddSlider( {}, 0, Bass.max * Bass.scale, Bass.min * Bass.scale);

         // Treble control
         S
            .Target( mTreble,
               NumValidatorStyle::DEFAULT, 1, Treble.min, Treble.max)
            .Action( [this]{ UpdateGain(); } )
            .AddTextBox(XXO("&Treble (dB):"), L"", 10);

         S
            .Text(XO("Treble"))
            .Style(wxSL_HORIZONTAL)
            .Target( Scale( mTreble, Treble.scale ) )
            .Action( [this]{ UpdateGain(); } )
            .AddSlider( {}, 0, Treble.max * Treble.scale, Treble.min * Treble.scale);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("Output"));
   {
      S.StartMultiColumn(3, GroupOptions{ wxEXPAND }.StretchyColumn(2));
      {
         // Gain control
         S
            .Target( mGain, NumValidatorStyle::DEFAULT, 1, Gain.min, Gain.max )
            .AddTextBox(XXO("&Volume (dB):"), L"", 10);

         S
            .Text(XO("Level"))
            .Style(wxSL_HORIZONTAL)
            .Target( Scale( mGain, Gain.scale ) )
            .AddSlider( {}, 0, Gain.max * Gain.scale, Gain.min * Gain.scale);
      }
      S.EndMultiColumn();

      S.StartMultiColumn(2, wxCENTER);
      {
         // Link checkbox
         S
            .Target( mLink )
            .AddCheckBox(XXO("&Link Volume control to Tone controls"),
               Link.def);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
}

bool EffectBassTreble::TransferDataFromWindow()
{
   return true;
}


// EffectBassTreble implementation

void EffectBassTreble::InstanceInit(EffectBassTrebleState & data, float sampleRate)
{
   data.samplerate = sampleRate;
   data.slope = 0.4f;   // same slope for both filters
   data.hzBass = 250.0f;   // could be tunable in a more advanced version
   data.hzTreble = 4000.0f;   // could be tunable in a more advanced version

   data.a0Bass = 1;
   data.a1Bass = 0;
   data.a2Bass = 0;
   data.b0Bass = 0;
   data.b1Bass = 0;
   data.b2Bass = 0;

   data.a0Treble = 1;
   data.a1Treble = 0;
   data.a2Treble = 0;
   data.b0Treble = 0;
   data.b1Treble = 0;
   data.b2Treble = 0;

   data.xn1Bass = 0;
   data.xn2Bass = 0;
   data.yn1Bass = 0;
   data.yn2Bass = 0;

   data.xn1Treble = 0;
   data.xn2Treble = 0;
   data.yn1Treble = 0;
   data.yn2Treble = 0;

   data.bass = -1;
   data.treble = -1;
   data.gain = DB_TO_LINEAR(mGain);

}


// EffectClientInterface implementation


size_t EffectBassTreble::InstanceProcess(EffectBassTrebleState & data,
                                              float **inBlock,
                                              float **outBlock,
                                              size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];

   // Set value to ensure correct rounding
   double oldBass = DB_TO_LINEAR(mBass);
   double oldTreble = DB_TO_LINEAR(mTreble);

   data.gain = DB_TO_LINEAR(mGain);

   // Compute coefficients of the low shelf biquand IIR filter
   if (data.bass != oldBass)
      Coefficients(data.hzBass, data.slope, mBass, data.samplerate, kBass,
                  data.a0Bass, data.a1Bass, data.a2Bass,
                  data.b0Bass, data.b1Bass, data.b2Bass);

   // Compute coefficients of the high shelf biquand IIR filter
   if (data.treble != oldTreble)
      Coefficients(data.hzTreble, data.slope, mTreble, data.samplerate, kTreble,
                  data.a0Treble, data.a1Treble, data.a2Treble,
                  data.b0Treble, data.b1Treble, data.b2Treble);

   for (decltype(blockLen) i = 0; i < blockLen; i++) {
      obuf[i] = DoFilter(data, ibuf[i]) * data.gain;
   }

   return blockLen;
}



// Effect implementation


void EffectBassTreble::Coefficients(double hz, double slope, double gain, double samplerate, int type,
                                   double& a0, double& a1, double& a2,
                                   double& b0, double& b1, double& b2)
{
   double w = 2 * M_PI * hz / samplerate;
   double a = exp(log(10.0) * gain / 40);
   double b = sqrt((a * a + 1) / slope - (pow((a - 1), 2)));

   if (type == kBass)
   {
      b0 = a * ((a + 1) - (a - 1) * cos(w) + b * sin(w));
      b1 = 2 * a * ((a - 1) - (a + 1) * cos(w));
      b2 = a * ((a + 1) - (a - 1) * cos(w) - b * sin(w));
      a0 = ((a + 1) + (a - 1) * cos(w) + b * sin(w));
      a1 = -2 * ((a - 1) + (a + 1) * cos(w));
      a2 = (a + 1) + (a - 1) * cos(w) - b * sin(w);
   }
   else //assumed kTreble
   {
      b0 = a * ((a + 1) + (a - 1) * cos(w) + b * sin(w));
      b1 = -2 * a * ((a - 1) + (a + 1) * cos(w));
      b2 = a * ((a + 1) + (a - 1) * cos(w) - b * sin(w));
      a0 = ((a + 1) - (a - 1) * cos(w) + b * sin(w));
      a1 = 2 * ((a - 1) - (a + 1) * cos(w));
      a2 = (a + 1) - (a - 1) * cos(w) - b * sin(w);
   }
}

float EffectBassTreble::DoFilter(EffectBassTrebleState & data, float in)
{
   // Bass filter
   float out = (data.b0Bass * in + data.b1Bass * data.xn1Bass + data.b2Bass * data.xn2Bass -
         data.a1Bass * data.yn1Bass - data.a2Bass * data.yn2Bass) / data.a0Bass;
   data.xn2Bass = data.xn1Bass;
   data.xn1Bass = in;
   data.yn2Bass = data.yn1Bass;
   data.yn1Bass = out;

   // Treble filter
   in = out;
   out = (data.b0Treble * in + data.b1Treble * data.xn1Treble + data.b2Treble * data.xn2Treble -
         data.a1Treble * data.yn1Treble - data.a2Treble * data.yn2Treble) / data.a0Treble;
   data.xn2Treble = data.xn1Treble;
   data.xn1Treble = in;
   data.yn2Treble = data.yn1Treble;
   data.yn1Treble = out;

   return out;
}

void EffectBassTreble::UpdateGain()
{
   if ( mLink ) {
      // Which one changed?
      int control = ( mOldBass != mBass ) ? kBass : kTreble;
      int oldVal = ( control == kBass ) ? mOldBass : mOldTreble;

      double newVal;
      oldVal = (oldVal > 0)? oldVal / 2.0 : oldVal / 4.0;

      if (control == kBass)
         newVal = (mBass > 0)? mBass / 2.0 : mBass / 4.0;
      else
         newVal = (mTreble > 0)? mTreble / 2.0 : mTreble / 4.0;

      mGain -= newVal - oldVal;
      mGain = std::min(Gain.max, std::max(Gain.min, mGain));
   }

   mOldBass = mBass;
   mOldTreble = mTreble;
}
