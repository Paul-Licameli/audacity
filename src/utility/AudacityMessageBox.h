/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityMessageBox.h

  Paul Licameli split this out of ErrorDialog.h

**********************************************************************/

#ifndef __AUDACITY_MESSAGE_BOX__
#define __AUDACITY_MESSAGE_BOX__

class wxWindow;
#include "Internat.h"

extern TranslatableString AudacityMessageBoxCaptionStr();

// Do not use wxMessageBox!!  Its default window title does not translate!
int AudacityMessageBox(
   const TranslatableString& message,
   const TranslatableString& caption = AudacityMessageBoxCaptionStr(),
   long style = -1, // wxOK | wxCENTRE,
   wxWindow *parent = nullptr,
   int x = -1, int y = -1);

// Callback interface to be implemented by the application object; it implements
// details of display of a message to the user (perhaps with a dialog, perhaps
// just on the console).  The indirection exists so that AudacityMessageBox has
// link dependency only on wxBase.
class AudacityMessageBoxCallback
{
public:
   virtual ~AudacityMessageBoxCallback();
   virtual int ShowMessage(
      const TranslatableString &message,
      const TranslatableString &caption,
      long style = -1, // wxOK | wxCENTRE
      wxWindow *parent = NULL,
      int x = wxDefaultCoord, int y = wxDefaultCoord ) = 0;
};

#endif
