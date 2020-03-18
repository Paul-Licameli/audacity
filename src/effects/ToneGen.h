/**********************************************************************

  Audacity: A Digital Audio Editor

  ToneGen.h

  Steve Jolly

  This class implements a tone generator effect.

**********************************************************************/

#ifndef __AUDACITY_EFFECT_TONEGEN__
#define __AUDACITY_EFFECT_TONEGEN__

#include "Effect.h"
#include "../Shuttle.h"

class NumericTextCtrl;
class ShuttleGui;

class EffectToneGen : public Effect
{
public:
   EffectToneGen(bool isChirp);
   virtual ~EffectToneGen();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   unsigned GetAudioOutCount() override;
   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;

   // Effect implementation

   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataFromWindow() override;

private:
   // EffectToneGen implementation

private:
   const bool mChirp;

   // mSample is an external placeholder to remember the last "buffer"
   // position so we use it to reinitialize from where we left
   sampleCount mSample;
   double mPositionInCycles;

   // If we made these static variables,
   // Tone and Chirp would share the same parameters.
   int mWaveform;
   int mInterpolation;
   double mFrequency[2];
   double mAmplitude[2];
   double mLogFrequency[2];

   void PostSet();
   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
};

class EffectChirp final : public EffectToneGen
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectChirp() : EffectToneGen{ true } {}
};


class EffectTone final : public EffectToneGen
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectTone() : EffectToneGen{ false } {}
};

#endif
