/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.h

  Paul Licameli split from CommandManager.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_COMMAND_MANAGER__
#define __AUDACITY_PROJECT_COMMAND_MANAGER__

#include "commands/CommandManager.h"
#include "commands/CommandTargets.h"

class ProjectCommandManager final : public CommandManager {
public:
   using CommandManager::CommandManager;

   static CommandManager &Get( AudacityProject &project );
   static const CommandManager &Get( const AudacityProject &project );

   static void UpdateCheckmarksInAllProjects();
   static void RebuildAllMenuBars();

   std::unique_ptr<CommandOutputTargets> MakeTargets() override;
};

/// Displays messages from a command in a wxStatusBar
class AUDACITY_DLL_API StatusBarTarget final : public CommandMessageTarget
{
private:
   wxStatusBar &mStatus;
public:
   StatusBarTarget(wxStatusBar &sb)
      : mStatus(sb)
   {}
   void Update(const wxString &message) override;
};

#endif
