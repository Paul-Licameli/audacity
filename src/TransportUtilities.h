/**********************************************************************

  Audacity: A Digital Audio Editor

  @file TransportUtilities.h
  @brief Some UI related to starting and stopping play and record

  Paul Licameli split from ProjectAudioManager.h

**********************************************************************/

#ifndef __AUDACITY_TRANSPORT_UTILITIES__
#define __AUDACITY_TRANSPORT_UTILITIES__

#include "Audacity.h"
#include "Prefs.h"

struct AudioIOStartStreamOptions;
class CommandContext;
class SelectedRegion;
enum class PlayMode : int;

struct AUDACITY_DLL_API TransportUtilities
{
   static void PlayCurrentRegionAndWait(
      const CommandContext &context,
      bool looped = false,
      bool cutpreview = false);
   static void PlayPlayRegionAndWait(
      const CommandContext &context,
      const SelectedRegion &selectedRegion,
      const AudioIOStartStreamOptions &options,
      PlayMode mode);
   static void RecordAndWait(
      const CommandContext &context, bool altAppearance);

   static void DoStartPlaying(
      const CommandContext &context, bool looping = false);
   static bool DoStopPlaying(const CommandContext &context);

};

extern AUDACITY_DLL_API DoubleSetting RecordPreRollDuration;
extern AUDACITY_DLL_API DoubleSetting RecordCrossfadeDuration;

#endif
