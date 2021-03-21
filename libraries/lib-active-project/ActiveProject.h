/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 ActiveProject.h
 
 Paul Licameli split from Project.h
 
 **********************************************************************/

#ifndef __AUDACITY_ACTIVE_PROJECT__
#define __AUDACITY_ACTIVE_PROJECT__

#include <wx/event.h> // to declare custom event type

class AudacityProject;

// This event is emitted by the application object when there is a change
// in the activated project
wxDECLARE_EXPORTED_EVENT(ACTIVE_PROJECT_API,
                         EVT_PROJECT_ACTIVATION, wxCommandEvent);

ACTIVE_PROJECT_API AudacityProject *GetActiveProject();
// For use by ProjectManager only:
ACTIVE_PROJECT_API void SetActiveProject(AudacityProject * project);

#endif
