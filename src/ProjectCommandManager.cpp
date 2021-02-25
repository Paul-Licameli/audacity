/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.cpp

  Paul Licameli split from CommandManager.cpp

\class InteractiveOutputTargets
\brief InteractiveOutputTargets is an output target that pops up a
dialog, if necessary.

\class MessageDialogTarget
\brief MessageDialogTarget is a CommandOutputTarget that sends its status
to the LongMessageDialog.

\class LongMessageDialog
\brief LongMessageDialog is a dialog with a Text Window in it to
capture the more lengthy output from some commands.

**********************************************************************/

#include "ProjectCommandManager.h"
#include "DefaultCommandOutputTargets.h"
#include "Project.h"
#include "ShuttleGui.h"
#include "commands/CommandTargets.h"
#include "wxPanelWrapper.h"

#include <wx/app.h>
#include <wx/statusbr.h>

///
static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject&) {
      return std::make_unique<ProjectCommandManager>();
   }
};

CommandManager &ProjectCommandManager::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< CommandManager >( key );
}

const CommandManager &ProjectCommandManager::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

void ProjectCommandManager::UpdateCheckmarksInAllProjects()
{
   for (auto pProject : AllProjects{}) {
      auto &project = *pProject;
      ProjectCommandManager::Get(project).UpdateCheckmarks(project);
   }
}

std::unique_ptr<CommandOutputTargets> ProjectCommandManager::MakeTargets()
{
   return DefaultCommandOutputTargets();
}

void StatusBarTarget::Update(const wxString &message)
{
   mStatus.SetStatusText(message, 0);
}
