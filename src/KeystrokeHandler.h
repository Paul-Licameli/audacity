/**********************************************************************

  Audacity: A Digital Audio Editor

  @file KeystrokeHandler.h

  Paul Licameli split from CommandManager.h

**********************************************************************/
#ifndef __AUDACITY_KEYSTROKE_HANDLER__
#define __AUDACITY_KEYSTROKE_HANDLER__

#include "commands/Keyboard.h"

class AudacityProject;
class wxKeyEvent;

AUDACITY_DLL_API bool FilterKeyEvent(
   AudacityProject *project, const wxKeyEvent & evt, bool permit);

// Construct NormalizedKeyString from a key event
AUDACITY_DLL_API
NormalizedKeyString KeyEventToKeyString(const wxKeyEvent & keyEvent);

#endif
