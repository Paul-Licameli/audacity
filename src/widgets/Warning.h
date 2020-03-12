/**********************************************************************

  Audacity: A Digital Audio Editor

  Warning.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_WARNING__
#define __AUDACITY_WARNING__

#include "audacity/Types.h"

#include <wx/defs.h>
class BoolSetting;
class wxString;
class wxWindow;

// "Don't show this warning again"
AUDACITY_DLL_API
const TranslatableLabel &DefaultWarningFooter();

/// Displays a warning dialog with a check box.  If the user checks
/// the box, that persists in the BoolSetting.
AUDACITY_DLL_API
int ShowWarningDialog(wxWindow *parent,
                      BoolSetting &setting,
                      const TranslatableString &message,
                      bool showCancelButton = false,
                      // This message appears by the checkbox:
                      const TranslatableLabel &footer = DefaultWarningFooter());

#endif // __AUDACITY_WARNING__
