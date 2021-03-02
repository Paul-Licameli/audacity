/**********************************************************************

  Audacity: A Digital Audio Editor

  @file CommandDispatch.cpp
  @brief implements function HandleTextualCommand

  Paul Licameli split from MacroCommands.cpp

**********************************************************************/

#include "CommandDispatch.h"

#include "CommandManager.h"
#include "PluginManager.h"

using TextualCommandHandlers = std::vector<TextualCommandHandler>;

static TextualCommandHandlers& handlers()
{
   static TextualCommandHandlers theHandlers;
   return theHandlers;
}

RegisteredTextualCommandHandler::RegisteredTextualCommandHandler(
   TextualCommandHandler handler )
{
   handlers().emplace_back( std::move( handler ) );
}

RegisteredTextualCommandHandler::~RegisteredTextualCommandHandler()
{
   handlers().pop_back();
}

bool HandleTextualCommand( CommandManager &commandManager,
   const CommandID & Str,
   const CommandContext & context, CommandFlag flags, bool alwaysEnabled)
{
   switch ( commandManager.HandleTextualCommand(
      Str, context, flags, alwaysEnabled) ) {
   case CommandManager::CommandSuccess:
      return true;
   case CommandManager::CommandFailure:
      return false;
   case CommandManager::CommandNotFound:
   default:
      break;
   }

   for (auto &handler: handlers()) {
      if (handler && handler( Str, context ))
         return true;
   }

   return false;
}
