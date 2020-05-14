/**********************************************************************

  Audacity: A Digital Audio Editor

  Amplify.cpp

  Dominic Mazzoni
  Vaughan Johnson (Preview)

*******************************************************************//**

\class EffectAmplify
\brief An Effect that makes a sound louder or softer.

  This rewritten class supports a smart Amplify effect - it calculates
  the maximum amount of gain that can be applied to all tracks without
  causing clipping and selects this as the default parameter.

*//*******************************************************************/


#include "Amplify.h"
#include "LoadEffects.h"

#include <math.h>
#include <float.h>

#include "../ShuttleGui.h"
#include "../WaveTrack.h"


// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key                     Def         Min         Max            Scale
static auto Ratio = Parameter<float>(
                           L"Ratio",            0.9f,       0.003162f,  316.227766f,   1.0f  );
// PRL:  Amp was never a parameter saved in settings!  But it
// causes definition of some constants used below
static auto Amp = Parameter<float>(
                           L"",                -0.91515f,  -50.0f,     50.0f,         10.0f );
static auto Clipping = Parameter<bool>(
                           L"AllowClipping",    false,    false,  true,    1  );


//
// EffectAmplify
//

const ComponentInterfaceSymbol EffectAmplify::Symbol
{ XO("Amplify") };

namespace{ BuiltinEffectsModule::Registration< EffectAmplify > reg; }

EffectAmplify::EffectAmplify()
   : mParameters{
      // Interactive case
      mRatio, Ratio,
      mCanClip, Clipping
   }
   , mBatchParameters{
      // If invoking Amplify from a macro, mCanClip is not a parameter
      // but is always true
      [this]{ mCanClip = true; return true; },
      mRatio, Ratio
   }
{
   Parameters().Reset();
//   mRatio = DB_TO_LINEAR(Amp.def);
   mCanClip = false;
   mPeak = 0.0;

   SetLinearEffectFlag(true);
}

EffectAmplify::~EffectAmplify()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectAmplify::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectAmplify::GetDescription()
{
   // Note: This is useful only after ratio has been set.
   return XO("Increases or decreases the volume of the audio you have selected");
}

wxString EffectAmplify::ManualPage()
{
   return L"Amplify";
}

// EffectDefinitionInterface implementation

EffectType EffectAmplify::GetType()
{
   return EffectTypeProcess;
}

// EffectClientInterface implementation

unsigned EffectAmplify::GetAudioInCount()
{
   return 1;
}

unsigned EffectAmplify::GetAudioOutCount()
{
   return 1;
}

size_t EffectAmplify::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   if ( !IsBatchProcessing() && !mCanClip && mRatio * mPeak > 1.0 )
      mRatio = 1.0 / mPeak;

   for (decltype(blockLen) i = 0; i < blockLen; i++)
      outBlock[0][i] = inBlock[0][i] * mRatio;
   return blockLen;
}

bool EffectAmplify::LoadFactoryDefaults()
{
   Init();

   if (mPeak > 0.0)
      mRatio = 1.0 / mPeak;
   else
      mRatio = 1.0;
   mCanClip = false;

   return !mUIParent || mUIParent->TransferDataToWindow();
}

// Effect implementation

bool EffectAmplify::Init()
{
   mPeak = 0.0;

   for (auto t : inputTracks()->Selected< const WaveTrack >())
   {
      auto pair = t->GetMinMax(mT0, mT1); // may throw
      const float min = pair.first, max = pair.second;
      float newpeak = (fabs(min) > fabs(max) ? fabs(min) : fabs(max));

      if (newpeak > mPeak)
      {
         mPeak = newpeak;
      }
   }

   return true;
}

void EffectAmplify::Preview(bool dryOnly)
{
   auto cleanup1 = valueRestorer( mRatio );
   auto cleanup2 = valueRestorer( mPeak );

   Effect::Preview(dryOnly);
}

void EffectAmplify::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   enum{ precision = 3 }; // allow (a generous) 3 decimal  places for Amplification (dB)

   bool batch = IsBatchProcessing();
   if ( batch )
   {
      mCanClip = true;
      mPeak = 1.0;
   }
   else 
   {
      if (mPeak > 0.0)
         mRatio = 1.0 / mPeak;
      else
         mRatio = 1.0;
   }

   // establish limited range of gain
   mRatio = ClipRatio( mRatio );

   S.AddSpace(0, 5);

   // There are three controls that all target mRatio, presenting it to the user
   // in different terms.
   S.StartVerticalLay(0);
   {
      S.StartMultiColumn(2, GroupOptions{}.Position(wxCENTER));
      {
         // Amplitude as text
         S
            .Target( Transform( mRatio,
                  [](double output){ return LINEAR_TO_DB( output ); },
                  [this](double input){
                     // maintain limited range of gain
                     return ClipRatio( DB_TO_LINEAR( input ) ); } ),
               NumValidatorStyle::ONE_TRAILING_ZERO, precision,
               Amp.min, Amp.max )
            .AddTextBox(XXO("&Amplification (dB):"), L"", 12);
      }
      S.EndMultiColumn();

      // Amplitude as slider
      S.StartHorizontalLay(wxEXPAND);
      {
         S
            .Style(wxSL_HORIZONTAL)
            .Text(XO("Amplification dB"))
            .Target( Transform( mRatio,
               [](double output){
                  return LINEAR_TO_DB(output) * Amp.scale + 0.5; },
               [this](double input){
                  // maintain limited range of gain
                  return ClipRatio( DB_TO_LINEAR( input / Amp.scale ) ); } ) )
            .AddSlider( {}, 0, Amp.max * Amp.scale, Amp.min * Amp.scale);
      }
      S.EndHorizontalLay();

      S.StartMultiColumn(2, GroupOptions{}.Position(wxCENTER));
      {
         // New peak as text
         S
            .Target( Transform( mRatio,
               // Transformation depends on mPeak which is fixed after
               // initializaton
               [this](double output){ return LINEAR_TO_DB(output * mPeak); },
               [this](double input) {
                  // maintain limited range of gain
                  return ClipRatio( (input == 0.0)
                     ? 1.0 / mPeak
                     : DB_TO_LINEAR(input) / mPeak ); } ),
               NumValidatorStyle::ONE_TRAILING_ZERO,
               // One extra decimal place so that rounding is visible to user
               // (see: bug 958)
               precision + 1,
               // min and max need same precision as what we're validating (bug 963)
               RoundValue( precision + 1, Amp.min + LINEAR_TO_DB(mPeak) ),
               RoundValue( precision + 1, Amp.max + LINEAR_TO_DB(mPeak) ) )
            .AddTextBox(XXO("&New Peak Amplitude (dB):"), L"", 12);
      }
      S.EndMultiColumn();

      // Clipping checkbox
      S.StartHorizontalLay(wxCENTER);
      {
         S
            .Disable( batch )
            .Target( mCanClip )
            .AddCheckBox(XXO("Allo&w clipping"), false);
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   return;
}

double EffectAmplify::ClipRatio( double ratio )
{
   static const auto MIN = DB_TO_LINEAR( Amp.min );
   static const auto MAX = DB_TO_LINEAR( Amp.max );
   ratio = TrapDouble( ratio, MIN, MAX );

   return ratio;
}

// EffectAmplify implementation

bool EffectAmplify::CanApply()
{
   return mCanClip || (mPeak > 0.0 && mRatio <= 1.0 / mPeak);
}

