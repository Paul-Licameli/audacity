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
#include "Menus.h"
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

void ProjectCommandManager::RebuildAllMenuBars()
{
   for( auto p : AllProjects{} ) {
      MenuManager::Get(*p)
         .RebuildMenuBar(*p, ProjectCommandManager::Get(*p));
#if defined(__WXGTK__)
      // Workaround for:
      //
      //   http://bugzilla.audacityteam.org/show_bug.cgi?id=458
      //
      // This workaround should be removed when Audacity updates to wxWidgets 3.x which has a fix.
      auto &window = GetProjectFrame( *p );
      wxRect r = window.GetRect();
      window.SetSize(wxSize(1,1));
      window.SetSize(r.GetSize());
#endif
   }
}

void StatusBarTarget::Update(const wxString &message)
{
   mStatus.SetStatusText(message, 0);
}
