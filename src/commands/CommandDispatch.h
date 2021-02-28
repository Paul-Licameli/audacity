/**********************************************************************

  Audacity: A Digital Audio Editor

  @file CommandDispatch.h
  @brief Interpret text as a command or effect name

  Paul Licameli split from MacroCommands.h

**********************************************************************/

#ifndef __AUDACITY_COMMAND_DISPATCH
#define __AUDACITY_COMMAND_DISPATCH

#include <functional>
#include "CommandFlag.h"
#include "CommandID.h"

class CommandContext;
class CommandManager;

AUDACITY_DLL_API bool HandleTextualCommand( CommandManager &commandManager,
   const CommandID & Str,
   const CommandContext & context, CommandFlag flags, bool alwaysEnabled);

//! Type of function that extends command dispatching; returns true if command succeeded
using TextualCommandHandler =
   std::function< bool(const CommandID &, const CommandContext &) >;

//! Statically constructed instance extends command dispatching
/*! If no command of the given ID is known, then the first registered function that accepts it will be used */
struct AUDACITY_DLL_API RegisteredTextualCommandHandler {
   RegisteredTextualCommandHandler( TextualCommandHandler handler );
   ~RegisteredTextualCommandHandler();
};

#endif
