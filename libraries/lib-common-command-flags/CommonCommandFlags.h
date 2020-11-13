/**********************************************************************

Audacity: A Digital Audio Editor

CommonCommandFlags.h

Paul Licameli split from Menus.cpp

**********************************************************************/

#ifndef __AUDACITY_COMMON_COMMAND_FLAGS__
#define __AUDACITY_COMMON_COMMAND_FLAGS__

#include "CommandFlag.h"

COMMON_COMMAND_FLAGS_API
bool EditableTracksSelectedPred( const AudacityProject &project );

COMMON_COMMAND_FLAGS_API
bool AudioIOBusyPred( const AudacityProject &project );

COMMON_COMMAND_FLAGS_API
bool TimeSelectedPred( const AudacityProject &project );

COMMON_COMMAND_FLAGS_API
const CommandFlagOptions &cutCopyOptions();

extern COMMON_COMMAND_FLAGS_API const ReservedCommandFlag
   &AudioIONotBusyFlag(),
   &NoiseReductionTimeSelectedFlag(),
   &TimeSelectedFlag(), // This is equivalent to check if there is a valid selection, so it's used for Zoom to Selection too
   &WaveTracksSelectedFlag(),
   &TracksExistFlag(),
   &EditableTracksSelectedFlag(),
   &AnyTracksSelectedFlag(),
   &TrackPanelHasFocus();  //lll

extern COMMON_COMMAND_FLAGS_API const ReservedCommandFlag
   &AudioIOBusyFlag(), // lll
   &CaptureNotBusyFlag();

extern COMMON_COMMAND_FLAGS_API const ReservedCommandFlag
   &UndoAvailableFlag(),
   &RedoAvailableFlag(),
   &ZoomInAvailableFlag(),
   &ZoomOutAvailableFlag(),
   &PlayRegionLockedFlag(),  //msmeyer
   &PlayRegionNotLockedFlag(),  //msmeyer
   &WaveTracksExistFlag(),
   &NoteTracksSelectedFlag(),  //gsw
   &IsNotSyncLockedFlag(),  //awd
   &IsSyncLockedFlag(),  //awd
   &NotMinimizedFlag(), // prl
   &PausedFlag(), // jkc
   &NoAutoSelect() // jkc
;

#endif
