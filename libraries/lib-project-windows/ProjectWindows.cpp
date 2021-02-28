/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectWindows.cpp

  Paul Licameli split from Project.cpp

**********************************************************************/

#include "ProjectWindows.h"
#include "BasicUI.h"
#include "Project.h"

#include <wx/frame.h>

namespace {
struct ProjectWindows final : AttachedProjectObject
{
   static ProjectWindows &Get( AudacityProject &project );
   static const ProjectWindows &Get( const AudacityProject &project );
   explicit ProjectWindows(AudacityProject &project)
      : mAttachedWindows{project}
   {}

   wxWeakRef< CellularPanel > mPanel{};
   wxWeakRef< wxFrame > mFrame{};

   AttachedWindows mAttachedWindows;
 };

const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject &project) {
      return std::make_unique<ProjectWindows>(project);
   }
};

ProjectWindows &ProjectWindows::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< ProjectWindows >( key );
}

const ProjectWindows &ProjectWindows::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}
}

PROJECT_WINDOWS_API CellularPanel &GetProjectPanel( AudacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mPanel;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

PROJECT_WINDOWS_API const CellularPanel &GetProjectPanel(
   const AudacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mPanel;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

PROJECT_WINDOWS_API void SetProjectPanel(
   AudacityProject &project, CellularPanel &panel )
{
   ProjectWindows::Get(project).mPanel = &panel;
}

PROJECT_WINDOWS_API wxFrame &GetProjectFrame( AudacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mFrame;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

PROJECT_WINDOWS_API const wxFrame &GetProjectFrame( const AudacityProject &project )
{
   auto ptr = ProjectWindows::Get(project).mFrame;
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

wxFrame *FindProjectFrame( AudacityProject *project ) {
   if (!project)
      return nullptr;
   return ProjectWindows::Get(*project).mFrame;
}

const wxFrame *FindProjectFrame( const AudacityProject *project ) {
   if (!project)
      return nullptr;
   return ProjectWindows::Get(*project).mFrame;
}

static WindowPlacementFactory &theFactory()
{
   static WindowPlacementFactory sFactory;
   return sFactory;
}

WindowPlacementFactory InstallProjectFramePlacementFactory(
   WindowPlacementFactory newFactory)
{
   auto &factory = theFactory();
   auto result = std::move(factory);
   factory = std::move(newFactory);
   return factory;
}

std::unique_ptr<const BasicUI::WindowPlacement>
ProjectFramePlacement( AudacityProject *project )
{
   if (!project || !theFactory())
      return std::make_unique<BasicUI::WindowPlacement>();
   return theFactory()(*project);
}

void SetProjectFrame(AudacityProject &project, wxFrame &frame )
{
   ProjectWindows::Get(project).mFrame = &frame;
}

AttachedWindows &GetAttachedWindows(AudacityProject &project)
{
   return ProjectWindows::Get(project).mAttachedWindows;
}
