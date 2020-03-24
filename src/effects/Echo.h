/**********************************************************************

  Audacity: A Digital Audio Editor

  Echo.h

  Dominic Mazzoni
  Vaughan Johnson (dialog)

**********************************************************************/

#ifndef __AUDACITY_EFFECT_ECHO__
#define __AUDACITY_EFFECT_ECHO__

#include "Effect.h"
#include "../Shuttle.h"

class ShuttleGui;

class EffectEcho final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectEcho();
   virtual ~EffectEcho();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;
   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   bool ProcessFinalize() override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;

   // Effect implementation
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

private:
   // EffectEcho implementation

private:
   double delay;
   double decay;
   Floats history;
   size_t histPos;
   size_t histLen;

   CapturedParameters mParameters;
   CapturedParameters& Parameters() override { return mParameters; }
};

#endif // __AUDACITY_EFFECT_ECHO__
