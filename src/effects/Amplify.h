/**********************************************************************

  Audacity: A Digital Audio Editor

  Amplify.h

  Dominic Mazzoni

  This rewritten class supports a smart Amplify effect - it calculates
  the maximum amount of gain that can be applied to all tracks without
  causing clipping and selects this as the default parameter.

**********************************************************************/

#ifndef __AUDACITY_EFFECT_AMPLIFY__
#define __AUDACITY_EFFECT_AMPLIFY__

#include "Effect.h"
#include "../Shuttle.h"


class ShuttleGui;

class EffectAmplify final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectAmplify();
   virtual ~EffectAmplify();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;
   bool LoadFactoryDefaults() override;

   // Effect implementation

   bool Init() override;
   void Preview(bool dryOnly) override;
   void PopulateOrExchange(ShuttleGui & S) override;

private:
   // EffectAmplify implementation

   double ClipRatio( double value );
   bool CanApply() override;

private:
   double mPeak;

   double mRatio;
   bool mCanClip;

   CapturedParameters mParameters;
   CapturedParameters mBatchParameters;
   CapturedParameters &Parameters() override {
      // Parameters differ depending on batch mode.  Option to disable clipping
      // is interactive only.
      return IsBatchProcessing() ? mBatchParameters : mParameters;
   }
};

#endif // __AUDACITY_EFFECT_AMPLIFY__
