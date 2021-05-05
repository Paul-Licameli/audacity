/**********************************************************************

  Audacity: A Digital Audio Editor

  RealtimeEffectRenderer.h

**********************************************************************/

#ifndef __AUDACITY_EFFECT_REALTIME_EFFECT_RENDERER__
#define __AUDACITY_EFFECT_REALTIME_EFFECT_RENDERER__

#include "Effect.h"
#include "Track.h"

class RealtimeEffectManager;
class WaveTrack;

class RealtimeEffectRenderer : public Effect
{
public:
   RealtimeEffectRenderer();
   virtual ~RealtimeEffectRenderer();

   // ComponentInterface implementation
   static const ComponentInterfaceSymbol Symbol;

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   bool IsInteractive() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;

   void SetSampleRate(double rate) override;
   size_t SetBlockSize(size_t maxBlockSize) override;
   size_t GetBlockSize() const override;

   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;
   bool ProcessFinalize() override;

private:
   // RealtimeEffectRenderer implementation
   RealtimeEffectManager *mManager;

   WaveTrack *mLeader;
   size_t mBufferSize;
   size_t mBlockSize;
   size_t mNumChans;
   float mGain;

};

#endif
