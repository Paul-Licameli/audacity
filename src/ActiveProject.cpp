/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 ActiveProject.cpp
 
 Paul Licameli split from Project.cpp
 
 **********************************************************************/

#include "ActiveProject.h"
#include "KeyboardCapture.h"
#include "Project.h"

#include <wx/app.h>
#include <wx/frame.h>

//This is a pointer to the currently-active project.
static AudacityProject *gActiveProject;

wxDEFINE_EVENT(EVT_PROJECT_ACTIVATION, wxCommandEvent);

AUDACITY_DLL_API AudacityProject *GetActiveProject()
{
   return gActiveProject;
}

void SetActiveProject(AudacityProject * project)
{
   if ( gActiveProject != project ) {
      gActiveProject = project;
      KeyboardCapture::Capture( nullptr );
      wxTheApp->QueueEvent( safenew wxCommandEvent{ EVT_PROJECT_ACTIVATION } );
   }
   wxTheApp->SetTopWindow( FindProjectFrame( project ) );
}
