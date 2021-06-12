/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 ScrubState.h
 
 Paul Licameli split from AudioIO.cpp
 
 **********************************************************************/

#ifndef __AUDACITY_SCRUB_STATE__
#define __AUDACITY_SCRUB_STATE__

#include "PlaybackSchedule.h" // to inherit

#include "AudioIOBase.h" // for ScrubbingOptions

class ScrubbingPlaybackPolicy final : public PlaybackPolicy {
public:
   explicit ScrubbingPlaybackPolicy(const ScrubbingOptions &);
   ~ScrubbingPlaybackPolicy() override;

   double NormalizeTrackTime( PlaybackSchedule &schedule ) override;

   bool AllowSeek( PlaybackSchedule & ) override;

   std::chrono::milliseconds
      SleepInterval( PlaybackSchedule & ) override;

private:
   const ScrubbingOptions mOptions;
};

static constexpr unsigned ScrubPollInterval_ms = 50;

#endif
