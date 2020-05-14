/**********************************************************************

  Audacity: A Digital Audio Editor

  ToneGen.cpp

  Steve Jolly
  James Crook (Adapted for 'Chirps')

  This class implements a tone generator effect.

*******************************************************************//**

\class EffectToneGen
\brief An Effect that can generate a sine, square or sawtooth wave.
An extended mode of EffectToneGen supports 'chirps' where the
frequency changes smoothly during the tone.

*//*******************************************************************/


#include "ToneGen.h"
#include "LoadEffects.h"

#include <math.h>
#include <float.h>

#include "../Project.h"
#include "../ProjectSettings.h"
#include "../ShuttleGui.h"
#include "../widgets/NumericTextCtrl.h"

enum kInterpolations
{
   kLinear,
   kLogarithmic,
   nInterpolations
};

static const EnumValueSymbol kInterStrings[nInterpolations] =
{
   // These are acceptable dual purpose internal/visible names
   { XO("Linear") },
   { XO("Logarithmic") }
};

enum kWaveforms
{
   kSine,
   kSquare,
   kSawtooth,
   kSquareNoAlias,
   nWaveforms
};

static const EnumValueSymbol kWaveStrings[nWaveforms] =
{
   { XO("Sine") },
   { XO("Square") },
   { XO("Sawtooth") },
   { XO("Square, no alias") }
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key                  Def      Min      Max                     Scale
static auto StartFreq = Parameter<double>(
                           L"StartFreq",     440.0,   1.0,     DBL_MAX,                1  );
static auto EndFreq = Parameter<double>(
                           L"EndFreq",       1320.0,  1.0,     DBL_MAX,                1  );
static auto StartAmp = Parameter<double>(
                           L"StartAmp",      0.8,     0.0,     1.0,                    1  );
static auto EndAmp = Parameter<double>(
                           L"EndAmp",        0.1,     0.0,     1.0,                    1  );
static auto Frequency = Parameter<double>(
                           L"Frequency",     440.0,   1.0,     DBL_MAX,                1  );
static auto Amplitude = Parameter<double>(
                           L"Amplitude",     0.8,     0.0,     1.0,                    1  );
static auto Waveform = EnumParameter(
                           L"Waveform",      0,       0,       nWaveforms - 1,      1, kWaveStrings, nWaveforms  );
static auto Interp = EnumParameter(
                           L"Interpolation", 0,       0,       nInterpolations - 1, 1, kInterStrings, nInterpolations  );

//
// EffectToneGen
//

const ComponentInterfaceSymbol EffectChirp::Symbol
{ XO("Chirp") };

namespace{ BuiltinEffectsModule::Registration< EffectChirp > reg; }

const ComponentInterfaceSymbol EffectTone::Symbol
{ XO("Tone") };

namespace{ BuiltinEffectsModule::Registration< EffectTone > reg2; }

EffectToneGen::EffectToneGen(bool isChirp)
   : mChirp{ isChirp }
   , mParameters{ mChirp
      ? CapturedParameters{
         [this]{ PostSet(); return true; },
         mFrequency[0], StartFreq,
         mFrequency[1], EndFreq,
         mAmplitude[0], StartAmp,
         mAmplitude[1], EndAmp,
         mWaveform, Waveform,
         mInterpolation, Interp
      }
      : CapturedParameters {
         [this]{ PostSet(); return true; },
         mFrequency[0], Frequency,
         mAmplitude[0], Amplitude,
         mWaveform, Waveform,
         mInterpolation, Interp
      }
   }
{
   Parameters().Reset();

   wxASSERT(nWaveforms == WXSIZEOF(kWaveStrings));
   wxASSERT(nInterpolations == WXSIZEOF(kInterStrings));

   // Chirp varies over time so must use selected duration.
   // TODO: When previewing, calculate only the first 'preview length'.
   if (isChirp)
      SetLinearEffectFlag(false);
   else
      SetLinearEffectFlag(true);
}

EffectToneGen::~EffectToneGen()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectToneGen::GetSymbol()
{
   return mChirp
      ? EffectChirp::Symbol
      : EffectTone::Symbol;
}

TranslatableString EffectToneGen::GetDescription()
{
   return mChirp
      ? XO("Generates an ascending or descending tone of one of four types")
      : XO("Generates a constant frequency tone of one of four types");
}

wxString EffectToneGen::ManualPage()
{
   return mChirp
      ? L"Chirp"
      : L"Tone";
}

// EffectDefinitionInterface implementation

EffectType EffectToneGen::GetType()
{
   return EffectTypeGenerate;
}

// EffectClientInterface implementation

unsigned EffectToneGen::GetAudioOutCount()
{
   return 1;
}

bool EffectToneGen::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   mPositionInCycles = 0.0;
   mSample = 0;

   return true;
}

size_t EffectToneGen::ProcessBlock(float **WXUNUSED(inBlock), float **outBlock, size_t blockLen)
{
   float *buffer = outBlock[0];
   double throwaway = 0;        //passed to modf but never used
   double f = 0.0;
   double a, b;
   int k;

   double frequencyQuantum;
   double BlendedFrequency;
   double BlendedAmplitude;
   double BlendedLogFrequency = 0.0;

   // calculate delta, and reposition from where we left
   auto doubleSampleCount = mSampleCnt.as_double();
   auto doubleSample = mSample.as_double();
   double amplitudeQuantum =
      (mAmplitude[1] - mAmplitude[0]) / doubleSampleCount;
   BlendedAmplitude = mAmplitude[0] +
      amplitudeQuantum * doubleSample;

   // precalculations:
   double pre2PI = 2.0 * M_PI;
   double pre4divPI = 4.0 / M_PI;

   // initial setup should calculate deltas
   if (mInterpolation == kLogarithmic)
   {
      // this for log interpolation
      mLogFrequency[0] = log10(mFrequency[0]);
      mLogFrequency[1] = log10(mFrequency[1]);
      // calculate delta, and reposition from where we left
      frequencyQuantum = (mLogFrequency[1] - mLogFrequency[0]) / doubleSampleCount;
      BlendedLogFrequency = mLogFrequency[0] + frequencyQuantum * doubleSample;
      BlendedFrequency = pow(10.0, BlendedLogFrequency);
   }
   else
   {
      // this for regular case, linear interpolation
      frequencyQuantum = (mFrequency[1] - mFrequency[0]) / doubleSampleCount;
      BlendedFrequency = mFrequency[0] + frequencyQuantum * doubleSample;
   }

   // synth loop
   for (decltype(blockLen) i = 0; i < blockLen; i++)
   {
      switch (mWaveform)
      {
      case kSine:
         f = sin(pre2PI * mPositionInCycles / mSampleRate);
         break;
      case kSquare:
         f = (modf(mPositionInCycles / mSampleRate, &throwaway) < 0.5) ? 1.0 : -1.0;
         break;
      case kSawtooth:
         f = (2.0 * modf(mPositionInCycles / mSampleRate + 0.5, &throwaway)) - 1.0;
         break;
      case kSquareNoAlias:    // Good down to 110Hz @ 44100Hz sampling.
         //do fundamental (k=1) outside loop
         b = (1.0 + cos((pre2PI * BlendedFrequency) / mSampleRate)) / pre4divPI;  //scaling
         f = pre4divPI * sin(pre2PI * mPositionInCycles / mSampleRate);
         for (k = 3; (k < 200) && (k * BlendedFrequency < mSampleRate / 2.0); k += 2)
         {
            //Hann Window in freq domain
            a = 1.0 + cos((pre2PI * k * BlendedFrequency) / mSampleRate);
            //calc harmonic, apply window, scale to amplitude of fundamental
            f += a * sin(pre2PI * mPositionInCycles / mSampleRate * k) / (b * k);
         }
      }
      // insert value in buffer
      buffer[i] = (float) (BlendedAmplitude * f);
      // update freq,amplitude
      mPositionInCycles += BlendedFrequency;
      BlendedAmplitude += amplitudeQuantum;
      if (mInterpolation == kLogarithmic)
      {
         BlendedLogFrequency += frequencyQuantum;
         BlendedFrequency = pow(10.0, BlendedLogFrequency);
      }
      else
      {
         BlendedFrequency += frequencyQuantum;
      }
   }

   // update external placeholder
   mSample += blockLen;

   return blockLen;
}

void EffectToneGen::PostSet()
{
   if (!mChirp) {
      mFrequency[1] = mFrequency[0];
      mAmplitude[1] = mAmplitude[0];
   }
   double freqMax =
      (FindProject()
         ? ProjectSettings::Get( *FindProject() ).GetRate()
         : 44100.0)
      / 2.0;
   mFrequency[1] = TrapDouble(mFrequency[1], EndFreq.min, freqMax);
}

// Effect implementation

void EffectToneGen::PopulateOrExchange(ShuttleGui & S)
{
   S.StartMultiColumn(2, wxCENTER);
   {
      S
         .Target( mWaveform )
         .AddChoice(XXO("&Waveform:"),
            Msgids( kWaveStrings, nWaveforms ) );

      //
      //
      if (mChirp)
      {
         S.AddFixedText( {} );
         S.StartHorizontalLay(wxEXPAND);
         {
            S.StartHorizontalLay(wxLEFT, 50);
            {
               S
                  .AddTitle(XO("Start"));
            }
            S.EndHorizontalLay();

            S.StartHorizontalLay(wxLEFT, 50);
            {
               S
                  .AddTitle(XO("End"));
            }
            S.EndHorizontalLay();
         }
         S.EndHorizontalLay();

         S
            .AddPrompt(XXO("&Frequency (Hz):"));

         S.StartHorizontalLay(wxEXPAND);
         {
            S.StartHorizontalLay(wxLEFT, 50);
            {
               S
                  .Text(XO("Frequency Hertz Start"))
                  .Target( mFrequency[0],
                     NumValidatorStyle::NO_TRAILING_ZEROES, 6,
                     StartFreq.min,
                     mProjectRate / 2.0 )
                  .AddTextBox( {}, L"", 12);
            }
            S.EndHorizontalLay();

            S.StartHorizontalLay(wxLEFT, 50);
            {
               S
                  .Text(XO("Frequency Hertz End"))
                  .Target( mFrequency[1],
                     NumValidatorStyle::NO_TRAILING_ZEROES, 6,
                     EndFreq.min,
                     mProjectRate / 2.0 )
                  .AddTextBox( {}, L"", 12);
            }
            S.EndHorizontalLay();
         }
         S.EndHorizontalLay();

         S
            .AddPrompt(XXO("&Amplitude (0-1):"));

         S.StartHorizontalLay(wxEXPAND);
         {
            S.StartHorizontalLay(wxLEFT, 50);
            {
               S
                  .Text(XO("Amplitude Start"))
                  .Target( mAmplitude[0],
                     NumValidatorStyle::NO_TRAILING_ZEROES, 6,
                     StartAmp.min, StartAmp.max )
                  .AddTextBox( {}, L"", 12);
            }
            S.EndHorizontalLay();

            S.StartHorizontalLay(wxLEFT, 50);
            {
               S
                  .Text(XO("Amplitude End"))
                  .Target( mAmplitude[1],
                     NumValidatorStyle::NO_TRAILING_ZEROES, 6,
                     EndAmp.min, EndAmp.max )
                  .AddTextBox( {}, L"", 12);
            }
            S.EndHorizontalLay();
         }
         S.EndHorizontalLay();

         S
            .Target( mInterpolation )
            .AddChoice(XXO("I&nterpolation:"),
               Msgids( kInterStrings, nInterpolations ) );
      }
      else
      {
         S
            .Target( mFrequency[0],
               NumValidatorStyle::NO_TRAILING_ZEROES, 6,
               Frequency.min,
               mProjectRate / 2.0 )
            .AddTextBox(XXO("&Frequency (Hz):"), L"", 12);

         S
            .Target( mAmplitude[0],
               NumValidatorStyle::NO_TRAILING_ZEROES, 6,
               Amplitude.min, Amplitude.max )
            .AddTextBox(XXO("&Amplitude (0-1):"), L"", 12);
      }

      S.AddPrompt(XXO("&Duration:"));

      S
         .Text(XO("Duration"))
         .Position(wxALIGN_LEFT | wxALL)
         .Target( DurationTarget() )
         .AddNumericTextCtrl( NumericConverter::TIME,
            GetDurationFormat(),
            GetDuration(),
            mProjectRate,
            NumericTextCtrl::Options{}
               .AutoPos(true));
   }
   S.EndMultiColumn();

   return;
}

bool EffectToneGen::TransferDataFromWindow()
{
   if (!mChirp)
   {
      mFrequency[1] = mFrequency[0];
      mAmplitude[1] = mAmplitude[0];
   }

   return true;
}

