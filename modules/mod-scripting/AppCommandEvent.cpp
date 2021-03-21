/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file AppCommandEvent.cpp
\brief Implements AppCommandEvent.

*//****************************************************************//**

\class AppCommandEvent
\brief An event 'envelope' for sending Command objects through the wxwidgets
event loop.

   This allows commands to be communicated from the script thread to the main
   thread.

*//*******************************************************************/

#include "AppCommandEvent.h"

wxDEFINE_EVENT(wxEVT_APP_COMMAND_RECEIVED, AppCommandEvent);
IMPLEMENT_DYNAMIC_CLASS(AppCommandEvent, wxEvent)

AppCommandEvent::AppCommandEvent(wxEventType commandType, int id)
: wxEvent{id, commandType}
{ }

// Copy constructor
AppCommandEvent::AppCommandEvent(const AppCommandEvent &event)
   : wxEvent{event}
   , mCommand(event.mCommand)
{
}

AppCommandEvent::~AppCommandEvent()
{
}

// Clone is required by wxwidgets; implemented via copy constructor
wxEvent *AppCommandEvent::Clone() const
{
   return safenew AppCommandEvent(*this);
}

/// Store a pointer to a command object
void AppCommandEvent::SetCommand(const OldStyleCommandPointer &cmd)
{
   wxASSERT(!mCommand);
   mCommand = cmd;
}

OldStyleCommandPointer AppCommandEvent::GetCommand()
{
   return mCommand;
}
