/**********************************************************************

  Audacity: A Digital Audio Editor

  SimpleMono.cpp

  Dominic Mazzoni

*******************************************************************//**
\class EffectSimpleMono
\brief An abstract Effect class that simplifies the implementation of a basic
  monaural effect.  Inherit from it if your effect just modifies
  a single track in place and doesn't care how many samples
  it gets at a time.

  Your derived class only needs to implement
  GetSymbol, GetEffectAction, and ProcessSimpleMono.

*//*******************************************************************/



#include "SimpleMono.h"

#include "../WaveTrack.h"

#include <math.h>

bool EffectSimpleMono::Process()
{
   //Iterate over each track
   this->CopyInputTracks(); // Set up mOutputTracks.
   bool bGoodResult = true;

   mCurTrackNum = 0;
   for( auto pOutWaveTrack : mOutputTracks->Selected< WaveTrack >() )
   {
      //Get start and end times from track
      double trackStart = pOutWaveTrack->GetStartTime();
      double trackEnd = pOutWaveTrack->GetEndTime();

      //Set the current bounds to whichever left marker is
      //greater and whichever right marker is less:
      mCurT0 = mT0 < trackStart? trackStart: mT0;
      mCurT1 = mT1 > trackEnd? trackEnd: mT1;

      // Process only if the right marker is to the right of the left marker
      if (mCurT1 > mCurT0) {

         //Transform the marker timepoints to samples
         auto start = pOutWaveTrack->TimeToLongSamples(mCurT0);
         auto end = pOutWaveTrack->TimeToLongSamples(mCurT1);

         //Get the track rate and samples
         mCurRate = pOutWaveTrack->GetRate();
         mCurChannel = pOutWaveTrack->GetChannel();

         //NewTrackSimpleMono() will returns true by default
         //ProcessOne() processes a single track
         if (!NewTrackSimpleMono() || !ProcessOne(pOutWaveTrack, start, end))
         {
            bGoodResult = false;
            break;
         }
      }

      mCurTrackNum++;
   }

   this->ReplaceProcessedTracks(bGoodResult);
   return bGoodResult;
}


//ProcessOne() takes a track, transforms it to bunch of buffer-blocks,
//and executes ProcessSimpleMono on these blocks
bool EffectSimpleMono::ProcessOne(WaveTrack * track,
                                  sampleCount start, sampleCount end)
{
   bool bResult = InPlaceTransformBlocks( { track }, start, end, 0,
   [this]( sampleCount s, size_t block, float *const *buffers, size_t ){
      return ProcessSimpleMono(buffers[0], block);
   }, mCurTrackNum );

   //Return true because the effect processing succeeded.
   return true;
}

//null implementation of NewTrackSimpleMono
bool EffectSimpleMono::NewTrackSimpleMono()
{
   return true;
}
