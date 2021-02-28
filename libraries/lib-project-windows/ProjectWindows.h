/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectWindows.h

  Paul Licameli split from Project.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_WINDOWS__
#define __AUDACITY_PROJECT_WINDOWS__

#include "ClientData.h"
#include "CellularPanel.h"

#include <functional>

class AudacityProject;
class wxFrame;

namespace BasicUI { class WindowPlacement; }

///\brief Get the main sub-window of the project frame that displays track data
// (as a wxWindow only, when you do not need to use the subclass TrackPanel)
PROJECT_WINDOWS_API CellularPanel &GetProjectPanel( AudacityProject &project );
PROJECT_WINDOWS_API const CellularPanel &GetProjectPanel(
   const AudacityProject &project );

PROJECT_WINDOWS_API void SetProjectPanel(
   AudacityProject &project, CellularPanel &panel );

///\brief Get the top-level window associated with the project (as a wxFrame
/// only, when you do not need to use the subclass ProjectWindow)
PROJECT_WINDOWS_API wxFrame &GetProjectFrame( AudacityProject &project );
PROJECT_WINDOWS_API const wxFrame &GetProjectFrame( const AudacityProject &project );

///\brief Get a pointer to the window associated with a project, or null if
/// the given pointer is null, or the window was not yet set.
wxFrame *FindProjectFrame( AudacityProject *project );

///\brief Get a pointer to the window associated with a project, or null if
/// the given pointer is null, or the window was not yet set.
const wxFrame *FindProjectFrame( const AudacityProject *project );

PROJECT_WINDOWS_API void SetProjectFrame(
   AudacityProject &project, wxFrame &frame );

//! Type of function that makes a WindowPlacement for dialogs, with project frame as parent
using WindowPlacementFactory = std::function<
   std::unique_ptr<const BasicUI::WindowPlacement>(
      AudacityProject &project)
>;

//! Install the WindowPlacementFactory used by ProjectFramePlacement
/*! @return the previously installed factory, if any */
PROJECT_WINDOWS_API WindowPlacementFactory
InstallProjectFramePlacementFactory(WindowPlacementFactory newFactory);

//! Make a WindowPlacement object suitable for `project` (which may be null)
/*! @post return value is not null */
PROJECT_WINDOWS_API std::unique_ptr<const BasicUI::WindowPlacement>
ProjectFramePlacement( AudacityProject *project );

// Container of pointers to various windows associated with the project, which
// is not responsible for destroying them -- wxWidgets handles that instead
class AttachedWindows : public ClientData::Site<
   AttachedWindows, wxWindow, ClientData::SkipCopying, ClientData::BarePtr
>
{
public:
   explicit AttachedWindows(AudacityProject &project)
      : mProject{project} {}
   operator AudacityProject & () const { return mProject; }
private:
   AudacityProject &mProject;
};

PROJECT_WINDOWS_API
AttachedWindows &GetAttachedWindows(AudacityProject &project);

#endif
