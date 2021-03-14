/**********************************************************************

  Audacity: A Digital Audio Editor

  @file RecordUtilties.h

  Paul Licameli split from ProjectAudioManager.h and TransportUtilities.h

**********************************************************************/

#ifndef __AUDACITY_RECORD_UTILITIES__
#define __AUDACITY_RECORD_UTILITIES__

#include "Prefs.h"

#include <memory>

class AudacityProject;
struct AudioIOStartStreamOptions;
struct TransportTracks;
class WaveTrack;
using WaveTrackArray = std::vector < std::shared_ptr < WaveTrack > >;

namespace RecordUtilities {

bool UseDuplex();

constexpr int RATE_NOT_SELECTED{ -1 };

// Find suitable tracks to record into, or return an empty array.
AUDACITY_DLL_API WaveTrackArray ChooseExistingRecordingTracks(
   AudacityProject &proj, bool selectedOnly,
   double targetRate = RATE_NOT_SELECTED);

bool DoRecord(AudacityProject &project,
   const TransportTracks &tracks, //!< If captureTracks is empty, then tracks are created
   double t0, double t1,
   bool altAppearance,
   const AudioIOStartStreamOptions &options);

void OnRecord(AudacityProject &project, bool altAppearance);

struct PropertiesOfSelected
{
   bool allSameRate{ false };
   int rateOfSelected{ RATE_NOT_SELECTED };
   int numberOfSelected{ 0 };
};

AUDACITY_DLL_API
PropertiesOfSelected GetPropertiesOfSelected(const AudacityProject &proj);

extern AUDACITY_DLL_API DoubleSetting RecordPreRollDuration;
extern AUDACITY_DLL_API DoubleSetting RecordCrossfadeDuration;

}

#endif
