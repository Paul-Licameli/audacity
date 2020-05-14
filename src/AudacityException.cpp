/**********************************************************************

  Audacity: A Digital Audio Editor

  AudacityException

  Paul Licameli

********************************************************************//**

\class AudacityException
\brief root of a hierarchy of classes that are thrown and caught
 by Audacity.

\class MessageBoxException
\brief an AudacityException that pops up a single message box even if
there were multiple exceptions of the same kind before it actually 
got to show.

*//********************************************************************/

#include "Audacity.h"
#include "AudacityException.h"

#include "widgets/AudacityMessageBox.h"

#include <atomic>

AudacityException::~AudacityException()
{
}

static std::atomic< unsigned > sOutstandingMessages { 0 };

MessageBoxException::MessageBoxException( const TranslatableString &caption_ )
   : caption{ caption_ }
{
   if (!caption.empty())
      sOutstandingMessages.fetch_add( 1, std::memory_order_release );
   else
      // invalidate me
      moved = true;
}

// The class needs a copy constructor to be throwable
// (or will need it, by C++14 rules).  But the copy
// needs to act like a move constructor.  There must be unique responsibility
// for each exception thrown to decrease the global count when it is handled.
MessageBoxException::MessageBoxException( const MessageBoxException& that )
{
   caption = that.caption;
   moved = that.moved;
   that.moved = true;
}

MessageBoxException::~MessageBoxException()
{
   if (!moved)
      // If exceptions are used properly, you should never reach this,
      // because moved should become true earlier in the object's lifetime.
      sOutstandingMessages.fetch_sub( 1, std::memory_order_release );
}

SimpleMessageBoxException::~SimpleMessageBoxException()
{
}

TranslatableString SimpleMessageBoxException::ErrorMessage() const
{
   return message;
}

// This is meant to be invoked in the main thread via wxEvtHandler::CallAfter
void MessageBoxException::DelayedHandlerAction()
{
   if (!moved) {
      // This test prevents accumulation of multiple messages between idle
      // times of the main even loop.  Only the last queued exception
      // displays its message.  We assume that multiple messages have a
      // common cause such as exhaustion of disk space so that the others
      // give the user no useful added information.
      if ( sOutstandingMessages.fetch_sub( 1, std::memory_order_acquire ) == 1 )
         ::AudacityMessageBox(
            ErrorMessage(),
            (caption.empty() ? AudacityMessageBoxCaptionStr() : caption),
            wxICON_ERROR
         );
      moved = true;
   }
}
