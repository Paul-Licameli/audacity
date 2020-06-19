/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityMessageBox.cpp

  Paul Licameli split this out of ErrorDialog.cpp

**********************************************************************/

#include "AudacityMessageBox.h"
#include "Internat.h"

#include <wx/app.h>

AudacityMessageBoxCallback::~AudacityMessageBoxCallback() = default;

TranslatableString AudacityMessageBoxCaptionStr()
{
   return XO("Message");
}

int AudacityMessageBox(const TranslatableString& message,
   const TranslatableString& caption,
   long style,
   wxWindow *parent,
   int x, int y )
{
   auto pCallback = dynamic_cast<AudacityMessageBoxCallback*>( wxTheApp );
   if ( pCallback )
      return pCallback->ShowMessage( message, caption, style, parent, x, y );
   else
      // assert false?
      ;
}
