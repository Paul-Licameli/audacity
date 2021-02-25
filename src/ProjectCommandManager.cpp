/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectCommandManager.cpp

  Paul Licameli split from CommandManager.cpp

**********************************************************************/

#include "ProjectCommandManager.h"
#include "Project.h"

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
