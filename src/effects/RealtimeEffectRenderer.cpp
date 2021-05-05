/**********************************************************************

  Audacity: A Digital Audio Editor

  RealtimeEffectRenderer.h

**********************************************************************/

#include "RealtimeEffectRenderer.h"
#include "LoadEffects.h"

#include "RealtimeEffectManager.h"

#include "../WaveTrack.h"

const ComponentInterfaceSymbol RealtimeEffectRenderer::Symbol
{
   XO("Realtime Effect Renderer")
};

namespace
{
   BuiltinEffectsModule::Registration<RealtimeEffectRenderer> reg;
}

RealtimeEffectRenderer::RealtimeEffectRenderer()
:  mLeader(nullptr),
   mBufferSize(0),
   mBlockSize(0),
   mNumChans(0),
   mGain(1.0f)
{
}

RealtimeEffectRenderer::~RealtimeEffectRenderer()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol RealtimeEffectRenderer::GetSymbol()
{
   return RealtimeEffectRenderer::Symbol;
}

TranslatableString RealtimeEffectRenderer::GetDescription()
{
   return XO("Applies master and track effects to the selected audio");
}

// EffectDefinitionInterface implementation

EffectType RealtimeEffectRenderer::GetType()
{
   return EffectTypeProcess;
}

bool RealtimeEffectRenderer::IsInteractive()
{
   return false;
}

// EffectClientInterface implementation

unsigned RealtimeEffectRenderer::GetAudioInCount()
{
   return 2;
}

unsigned RealtimeEffectRenderer::GetAudioOutCount()
{
   return 2;
}

void RealtimeEffectRenderer::SetSampleRate(double rate)
{
   mSampleRate = rate;
}


size_t RealtimeEffectRenderer::SetBlockSize(size_t maxBlockSize)
{
   mBlockSize = 8192;

   return mBlockSize;
}

size_t RealtimeEffectRenderer::GetBlockSize() const
{
   return mBlockSize;
}

bool RealtimeEffectRenderer::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   auto leaders = mOutputTracks->Leaders();

   if (mLeader != nullptr)
   {
      leaders = leaders.StartingWith(mLeader);
   }

   if (leaders.begin() == leaders.end())
   {
      return false;
   }

   mLeader = static_cast<WaveTrack *>(*leaders.begin());

   mNumChans = wxMin(TrackList::Channels(mLeader).size(), 2);
   mGain = mLeader->GetGain();

   mManager = &RealtimeEffectManager::Get(*const_cast<AudacityProject *>(FindProject()));
   mManager->Initialize(mSampleRate);
   mManager->AddProcessor(mLeader, mNumChans, mSampleRate);
   mManager->ProcessStart();

   return true;
}

size_t RealtimeEffectRenderer::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   for (auto c = 0; c < mNumChans; ++c)
   {
      memcpy(outBlock[c], inBlock[c], blockLen * sizeof(*outBlock[c]));
   }

//   mManager->Process(0, outBlock, mNumChans, blockLen, mGain);

   return blockLen;
}

bool RealtimeEffectRenderer::ProcessFinalize()
{
   mLeader = nullptr;

   return true;
}
