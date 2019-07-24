/**********************************************************************

  Audacity: A Digital Audio Editor

  SoundTouchEffect.h

  Dominic Mazzoni, Vaughan Johnson

  This abstract class contains all of the common code for an
  effect that uses SoundTouch to do its processing (ChangeTempo
  and ChangePitch).

**********************************************************************/

#include "../Audacity.h" // for USE_* macros

#if USE_SOUNDTOUCH

#ifndef __AUDACITY_EFFECT_SOUNDTOUCH__
#define __AUDACITY_EFFECT_SOUNDTOUCH__

#include "Effect.h"

// forward declaration of a class defined in SoundTouch.h
// which is not included here
namespace soundtouch { class SoundTouch; }


class TimeWarper;
class WaveTrack;

class EffectSoundTouch /* not final */ : public Effect
{
public:
   
   // Effect implementation

   void End() override;

   // EffectSoundTouch implementation

#ifdef USE_MIDI
   double mSemitones; // pitch change for NoteTracks
   EffectSoundTouch();
#endif
   ~EffectSoundTouch() override;

protected:
   // Effect implementation

   bool ProcessWithTimeWarper(
      const EffectContext &context, const TimeWarper &warper );

   std::unique_ptr<soundtouch::SoundTouch> mSoundTouch;
   double mCurT0;
   double mCurT1;

private:
   bool ProcessLabelTrack(LabelTrack *track, const TimeWarper &warper);
#ifdef USE_MIDI
   bool ProcessNoteTrack(NoteTrack *track, const TimeWarper &warper);
#endif
   bool ProcessOne( const EffectContext &context,
      WaveTrack * t, sampleCount start, sampleCount end,
      const TimeWarper &warper);
   bool ProcessStereo( const EffectContext &contect,
      WaveTrack* leftTrack, WaveTrack* rightTrack,
      sampleCount start, sampleCount end,
      const TimeWarper &warper);
   bool ProcessStereoResults( const EffectContext &context,
      const size_t outputCount,
      WaveTrack* outputLeftTrack,
      WaveTrack* outputRightTrack);

   int    mCurTrackNum;

   double m_maxNewLength;
};

#endif

#endif
