/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2018 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file ScriptCommandRelay.cpp
\brief Contains definitions for ScriptCommandRelay

*//****************************************************************//**

\class ScriptCommandRelay
\brief ScriptCommandRelay is just a way to move some of the scripting-specific
code out of ModuleManager.

*//*******************************************************************/

#include "ScriptCommandRelay.h"

#include "CommandTargets.h"
#include "CommandBuilder.h"
#include "AppCommandEvent.h"
#include "../Project.h"
#include <wx/app.h>
#include <wx/string.h>
#include <thread>

/// This is the function which actually obeys one command.
static int ExecCommand(wxString *pIn, wxString *pOut, bool fromMain)
{
   {
      CommandBuilder builder(::GetActiveProject(), *pIn);
      if (builder.WasValid())
      {
         OldStyleCommandPointer cmd = builder.GetCommand();

         AppCommandEvent ev;
         ev.SetCommand(cmd);

         if (fromMain)
         {
            // Use SafelyProcessEvent, which stops exceptions, because this is
            // expected to be reached from within the XLisp runtime
            wxTheApp->SafelyProcessEvent(ev);
         }
         else
         {
            // Send the event to the main thread
            wxTheApp->AddPendingEvent(ev);
         }
      }

      // Wait for and retrieve the response
      *pOut = builder.GetResponse();
   }

   return 0;
}

/// Executes a command in the worker (script) thread
static int ExecFromWorker(wxString *pIn, wxString *pOut)
{
   return ExecCommand(pIn, pOut, false);
}

/// Executes a command on the main (GUI) thread.
static int ExecFromMain(wxString *pIn, wxString *pOut)
{
   return ExecCommand(pIn, pOut, true);
}

/// Starts the script server
void ScriptCommandRelay::StartScriptServer(tpRegScriptServerFunc scriptFn)
{
   wxASSERT(scriptFn != NULL);

   auto server = [](tpRegScriptServerFunc function)
   {
      while (true)
      {
         function(ExecFromWorker);
      }
   };

   std::thread(server, scriptFn).detach();
}


void * ScriptCommandRelay::ExecForLisp( char * pIn )
{
   wxString Str1(pIn);
   wxString Str2;

   ExecFromMain(&Str1, &Str2);

   return nyq_reformat_aud_do_response(Str2);
}
