/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   File License: wxWidgets

   Dan Horgan

******************************************************************//**

\file CommandBuilder.cpp
\brief Contains definitions for CommandBuilder class.

\class CommandBuilder
\brief A type of factory for Commands of various sorts.

CommandBuilder has the task of deciding what command is meant by a given
command string, and producing a suitable command object. For now, it doesn't
actually do any processing - it just passes everything on to the BatchCommand
system by constructing BatchCommandEval objects.

*//*******************************************************************/


#include "CommandBuilder.h"

#include "CommandDirectory.h"
#include "Command.h"
#include "CommandContext.h"
#include "CommandTargets.h"
#include "../Shuttle.h"

CommandBuilder::CommandBuilder(
   AudacityProject *project, const wxString &cmdString)
   : mValid(false)
{
   BuildCommand(project, cmdString);
}

CommandBuilder::CommandBuilder(AudacityProject *project,
   const wxString &cmdName, const wxString &params)
   : mValid(false)
{
   BuildCommand(project, cmdName, params);
}

CommandBuilder::~CommandBuilder()
{
}

bool CommandBuilder::WasValid()
{
   return mValid;
}

OldStyleCommandPointer CommandBuilder::GetCommand()
{
   wxASSERT(mValid);
   wxASSERT(mCommand);
   auto result = mCommand;
   mCommand.reset();
   return result;
}

wxString CommandBuilder::GetResponse()
{
   if (!mValid && !mError.empty()) {
      return mError + L"\n";
   }
   return mResponse->GetResponse() + L"\n";
}

void CommandBuilder::Failure(const wxString &msg)
{
   mError = msg;
   mValid = false;
}

void CommandBuilder::Success(const OldStyleCommandPointer &cmd)
{
   mCommand = cmd;
   mValid = true;
}

void CommandBuilder::BuildCommand(AudacityProject *project,
                                  const wxString &cmdName,
                                  const wxString &cmdParamsArg)
{
   // Stage 1: create a Command object of the right type

   mResponse = std::make_shared< ResponseTarget >();
   auto output
      = std::make_unique<CommandOutputTargets>(std::make_unique<NullProgressTarget>(),
                                mResponse,
                                mResponse);

#ifdef OLD_BATCH_SYSTEM
   OldStyleCommandType *factory = CommandDirectory::Get()->LookUp(cmdName);

   if (factory == NULL)
   {
      // Fall back to hoping the Batch Command system can handle it
#endif
      OldStyleCommandType *type = CommandDirectory::Get()->LookUp(L"BatchCommand");
      wxASSERT(type != NULL);
      mCommand = type->Create(project, nullptr);
      mCommand->SetParameter(L"CommandName", cmdName);
      mCommand->SetParameter(L"ParamString", cmdParamsArg);
      auto aCommand = std::make_shared<ApplyAndSendResponse>(mCommand, output);
      Success(aCommand);
      return;
#ifdef OLD_BATCH_SYSTEM
   }

   CommandSignature &signature = factory->GetSignature();
   mCommand = factory->Create(nullptr);
   //mCommand->SetOutput( std::move(output) );
   // Stage 2: set the parameters

   ShuttleCli shuttle;
   shuttle.mParams = cmdParamsArg;
   shuttle.mbStoreInClient = true;

   ParamValueMap::const_iterator iter;
   ParamValueMap params = signature.GetDefaults();

   // Iterate through the parameters defined by the command
   for (iter = params.begin(); iter != params.end(); ++iter)
   {
      wxString paramString;
      // IF there is a match in the args actually used
      if (shuttle.TransferString(iter->first, paramString, L""))
      {
         // Then set that parameter.
         if (!mCommand->SetParameter(iter->first, paramString))
         {
            Failure();
            return;
         }
      }
   }

   // Check for unrecognised parameters

   wxString cmdParams(cmdParamsArg);

   while (!cmdParams.empty())
   {
      cmdParams.Trim(true);
      cmdParams.Trim(false);
      int splitAt = cmdParams.Find(L'=');
      if (splitAt < 0 && !cmdParams.empty())
      {
         Failure(L"Parameter string is missing '='");
         return;
      }
      wxString paramName = cmdParams.Left(splitAt);
      if (params.find(paramName) == params.end())
      {
         Failure(L"Unrecognized parameter: '" + paramName + L"'");
         return;
      }
      // Handling of quoted strings is quite limited.
      // You start and end with a " or a '.
      // There is no escaping in the string.
      cmdParams = cmdParams.Mid(splitAt+1);
      if( cmdParams.empty() )
         splitAt =-1;
      else if( cmdParams[0] == '\"' ){
         cmdParams = cmdParams.Mid(1);
         splitAt = cmdParams.Find(L'\"')+1;
      }
      else if( cmdParams[0] == '\'' ){
         cmdParams = cmdParams.Mid(1);
         splitAt = cmdParams.Find(L'\'')+1;
      }
      else
         splitAt = cmdParams.Find(L' ')+1;
      if (splitAt < 1)
      {
         splitAt = cmdParams.length();
      }
      cmdParams = cmdParams.Mid(splitAt);
   }
   auto aCommand = std::make_shared<ApplyAndSendResponse>(mCommand, output);
   Success(aCommand);
#endif
}

void CommandBuilder::BuildCommand(
   AudacityProject *project, const wxString &cmdStringArg)
{
   wxString cmdString(cmdStringArg);

   // Find the command name terminator...  If there is more than one word and
   // no terminator, the command is badly formed
   cmdString.Trim(true); cmdString.Trim(false);
   int splitAt = cmdString.Find(L':');
   if (splitAt < 0 && cmdString.Find(L' ') >= 0) {
      Failure(L"Syntax error!\nCommand is missing ':'");
      return;
   }

   wxString cmdName = cmdString.Left(splitAt);
   wxString cmdParams = cmdString.Mid(splitAt+1);
   if( splitAt < 0 )
      cmdParams = "";

   cmdName.Trim(true);
   cmdParams.Trim(false);

   BuildCommand(project, cmdName, cmdParams);
}
