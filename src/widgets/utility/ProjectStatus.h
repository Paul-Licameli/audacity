/**********************************************************************

Audacity: A Digital Audio Editor

ProjectStatus.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_PROJECT_STATUS__
#define __AUDACITY_PROJECT_STATUS__

#include "Audacity.h"

#include <utility>
#include <vector>
#include <wx/event.h> // to declare custom event type
#include <wx/frame.h> // to inherit

class ProjectBase;
class ProjectWindowBase;
class TranslatableString;

// Type of event emitted by the project window when one of the status messages
// has changed.  GetInt() identifies the intended field of the status bar
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_PROJECT_STATUS_UPDATE, wxCommandEvent);

///\brief Stores (per instance) message strings to be displayed in status bar
/// fields of the main project frame, and (globally) an array of callbacks to
/// compute widths of the fields
class ProjectStatus final
{
public:
   static ProjectStatus &Get( ProjectBase &project );
   static const ProjectStatus &Get( const ProjectBase &project );

   // The default field will also be variable-width
   explicit ProjectStatus(
      ProjectBase &project, unsigned nFields, unsigned defaultField );
   ProjectStatus( const ProjectStatus & ) = delete;
   ProjectStatus &operator= ( const ProjectStatus & ) = delete;
   ~ProjectStatus();

   // Type of a function to report translatable strings, and also report an
   // extra margin, to request that the corresponding field of the status bar
   // should be wide enough to contain any of those strings plus the margin.
   // May also report a minimum width.
   struct StatusWidthResult {
      std::vector<TranslatableString> strings;
      unsigned extra = 0;
      unsigned minimum = 0;
   };
   using StatusWidthFunction = std::function<
      StatusWidthResult( const ProjectBase &, unsigned /* field selector */ )
   >;
   using StatusWidthFunctions = std::vector< StatusWidthFunction >;

   // Typically a static instance of this struct is used.
   struct RegisteredStatusWidthFunction
   {
      explicit
      RegisteredStatusWidthFunction( StatusWidthFunction function );
   };

   static const StatusWidthFunctions &GetStatusWidthFunctions();

   // Get the default field
   const TranslatableString &Get() const;
   // Get specified field
   const TranslatableString &Get( unsigned field ) const;

   // Set default field
   void Set(const TranslatableString &msg);
   // Set specified field
   void Set(const TranslatableString &msg, unsigned field);

private:
   friend ProjectWindowBase;

   ProjectBase &mProject;
   std::vector<TranslatableString> mLastStatusMessages;
   unsigned mDefaultField;
};

///\brief A top-level window associated with a project.  It also has a
/// @ref ProjectStatus
class ProjectWindowBase /* not final */ : public wxFrame
{
public:
   // The default field will also be variable-width
   explicit ProjectWindowBase(
      wxWindowID id,
      const wxPoint & pos, const wxSize &size,
      ProjectBase &project, unsigned nFields, unsigned defaultField );

   ~ProjectWindowBase() override;

   ProjectBase &GetProject() { return mProject; }
   const ProjectBase &GetProject() const { return mProject; }

   void UpdateStatusWidths();

   // Calling this directly will not send an event for change of messages.
   void SetStatusText(const wxString &text, int number) override;

protected:
   friend ProjectStatus;

   ProjectBase &mProject;
   ProjectStatus mStatus;
};

///\brief Base object to hold the application's project data.  It can associate
/// with a @ref ProjectWindowBase and one other special window.  Those are set
/// after construction.
class ProjectBase
{
 public:
   ProjectWindowBase *GetFrame() { return mFrame; }
   const ProjectWindowBase *GetFrame() const { return mFrame; }
   void SetFrame( ProjectWindowBase *pFrame );
 
   wxWindow *GetPanel() { return mPanel; }
   const wxWindow *GetPanel() const { return mPanel; }
   void SetPanel( wxWindow *pPanel );

 private:
   wxWeakRef< ProjectWindowBase > mFrame{};
   wxWeakRef< wxWindow > mPanel{};
};

class AudacityProject; // Forward declaration only

///\brief Given a window, discover the @ref ProjectBase object, if any,
/// associated with its top-level parent.  Then static_cast the result to a
/// pointer to the template parameter type.
template< typename Subclass = AudacityProject >
inline Subclass *FindProjectFromWindow( wxWindow *pWindow )
{
   while ( pWindow && pWindow->GetParent() )
      pWindow = pWindow->GetParent();
   auto pProjectWindow = dynamic_cast< ProjectWindowBase* >( pWindow );
   auto result = pProjectWindow ? &pProjectWindow->GetProject() : nullptr;
   return static_cast< Subclass* >( result );
}

///\brief Given a window, discover the @ref ProjectBase object, if any,
/// associated with its top-level parent.  Then static_cast the result to a
/// pointer to the template parameter type.
template< typename Subclass = AudacityProject >
inline const Subclass *FindProjectFromWindow( const wxWindow *pWindow )
{
   return
      FindProjectFromWindow< Subclass >( const_cast< wxWindow* >( pWindow ) );
}

///\brief Get the top-level window associated with the project, or throw an
/// @ref InconsistencyException if it was not set
AUDACITY_DLL_API ProjectWindowBase &GetProjectFrame( ProjectBase &project );
///\brief Get the top-level window associated with the project, or throw an
/// @ref InconsistencyException if it was not set
AUDACITY_DLL_API
   const ProjectWindowBase &GetProjectFrame( const ProjectBase &project );

///\brief Get a pointer to the window associated with a project,
// if the given pointer is not null and the project's frame was set, or null
ProjectWindowBase *FindProjectFrame( ProjectBase *project );
///\brief Get a pointer to the window associated with a project,
// if the given pointer is not null and the project's frame was set, or null
const ProjectWindowBase *FindProjectFrame( const ProjectBase *project );

///\brief Get the extra sub-window associated with the project, or throw an
/// @ref InconsistencyException if it was not set
AUDACITY_DLL_API wxWindow &GetProjectPanel( ProjectBase &project );
///\brief Get the extra sub-window associated with the project, or throw an
/// @ref InconsistencyException if it was not set
AUDACITY_DLL_API const wxWindow &GetProjectPanel(
   const ProjectBase &project );

#endif
