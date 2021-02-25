/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.h

  Paul Licameli split from CommandManager.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_COMMAND_MANAGER__
#define __AUDACITY_PROJECT_COMMAND_MANAGER__

#include "commands/CommandManager.h"

class ProjectCommandManager final : public CommandManager {
public:
   using CommandManager::CommandManager;

   static CommandManager &Get( AudacityProject &project );
   static const CommandManager &Get( const AudacityProject &project );

   static void UpdateCheckmarksInAllProjects();
};

#endif
