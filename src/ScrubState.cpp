/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 ScrubState.cpp
 
 Paul Licameli split from AudioIO.cpp
 
 **********************************************************************/

#include "ScrubState.h"

ScrubbingPlaybackPolicy::ScrubbingPlaybackPolicy(
   const ScrubbingOptions &options)
   : mOptions{ options }
{}

ScrubbingPlaybackPolicy::~ScrubbingPlaybackPolicy() = default;

double ScrubbingPlaybackPolicy::NormalizeTrackTime( PlaybackSchedule &schedule )
{
   return schedule.GetTrackTime();
}
