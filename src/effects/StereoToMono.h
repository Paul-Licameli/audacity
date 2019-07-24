/**********************************************************************

  Audacity: A Digital Audio Editor

  StereoToMono.h

  Lynn Allan

**********************************************************************/

#ifndef __AUDACITY_EFFECT_STEREO_TO_MONO__
#define __AUDACITY_EFFECT_STEREO_TO_MONO__

#include "Effect.h"

#define STEREOTOMONO_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Stereo To Mono") }

class EffectStereoToMono final : public Effect
{
public:
   EffectStereoToMono();
   virtual ~EffectStereoToMono();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   bool IsInteractive() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;

   // Effect implementation

   bool Process( EffectContext &context ) override;
   bool IsHidden() override;

private:
   // EffectStereoToMono implementation

   bool ProcessOne( const EffectContext &context, int count );

private:
   sampleCount mStart;
   sampleCount mEnd;
   WaveTrack *mLeftTrack;
   WaveTrack *mRightTrack;
};

#endif

