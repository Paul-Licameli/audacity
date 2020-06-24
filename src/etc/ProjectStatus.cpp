/**********************************************************************

Audacity: A Digital Audio Editor

ProjectStatus.h

Paul Licameli

**********************************************************************/

#include "ProjectStatus.h"
#include "InconsistencyException.h"
#include "Internat.h"

wxDEFINE_EVENT(EVT_PROJECT_STATUS_UPDATE, wxCommandEvent);

ProjectStatus &ProjectStatus::Get( ProjectBase &project )
{
   return GetProjectFrame( project ).mStatus;
}

const ProjectStatus &ProjectStatus::Get( const ProjectBase &project )
{
   return Get( const_cast< ProjectBase & >( project ) );
}

ProjectStatus::ProjectStatus(
   ProjectBase &project, unsigned nFields, unsigned defaultField )
   : mProject{ project }
   , mLastStatusMessages( nFields )
   , mDefaultField{ defaultField }
{
}

ProjectStatus::~ProjectStatus() = default;

namespace
{
   ProjectStatus::StatusWidthFunctions &statusWidthFunctions()
   {
      static ProjectStatus::StatusWidthFunctions theFunctions;
      return theFunctions;
   }
}

ProjectStatus::RegisteredStatusWidthFunction::RegisteredStatusWidthFunction(
   StatusWidthFunction function )
{
   statusWidthFunctions().emplace_back( std::move( function ) );
}

auto ProjectStatus::GetStatusWidthFunctions() -> const StatusWidthFunctions &
{
   return statusWidthFunctions();
}

const TranslatableString &ProjectStatus::Get() const
{
   static TranslatableString empty;
   if ( mDefaultField >= mLastStatusMessages.size() )
      return empty;
   return Get( mDefaultField );
}

const TranslatableString &ProjectStatus::Get( unsigned field ) const
{
   return mLastStatusMessages[ field ];
}

void ProjectStatus::Set(const TranslatableString &msg )
{
   if ( mDefaultField >= mLastStatusMessages.size() )
      return;
   Set( msg, mDefaultField );
}

void ProjectStatus::Set(const TranslatableString &msg, unsigned field )
{
   auto &project = mProject;
   auto &lastMessage = mLastStatusMessages[ field ];
   // compare full translations not msgids!
   if ( msg.Translation() != lastMessage.Translation() ) {
      lastMessage = msg;

      // Be careful to null-check the window.  We might get to this function
      // during shut-down, but a timer hasn't been told to stop sending its
      // messages yet.
      auto pWindow = mProject.GetFrame();
      if ( !pWindow )
         return;
      auto &window = *pWindow;
      window.UpdateStatusWidths();
      window.SetStatusText( msg.Translation(), field );
      
      wxCommandEvent evt{ EVT_PROJECT_STATUS_UPDATE };
      evt.SetInt( field );
      window.GetEventHandler()->ProcessEvent( evt );
   }
}

ProjectWindowBase::ProjectWindowBase(wxWindowID id,
   const wxPoint & pos,
   const wxSize & size, ProjectBase &project,
   unsigned nFields, unsigned defaultField)
   : wxFrame(nullptr, id, _TS("Audacity"), pos, size)
   , mProject{ project }
   , mStatus{ mProject, nFields, defaultField }
{
   project.SetFrame( this );
};

ProjectWindowBase::~ProjectWindowBase()
{
}

void ProjectWindowBase::UpdateStatusWidths()
{
   auto nStatusBarFields = mStatus.mLastStatusMessages.size();
   auto nWidths = nStatusBarFields + 1;
   std::vector<int> widths( nWidths );
   const auto statusBar = GetStatusBar();
   const auto &functions = ProjectStatus::GetStatusWidthFunctions();
   // Start from 1 not 0
   // Specifying a first column always of width 0 was needed for reasons
   // I forget now
   for ( unsigned ii = 1; ii <= nStatusBarFields; ++ii ) {
      int &width = widths[ ii ];
      for ( const auto &function : functions ) {
         auto results = function( mProject, ii - 1 );
         for ( const auto &string : results.strings ) {
            int w;
            statusBar->GetTextExtent(string.Translation(), &w, nullptr);
            width = std::max<int>( width, w + results.extra );
         }
         width = std::max<int>( width, results.minimum );
      }
   }
   // The default status field is not fixed width
   if ( mStatus.mDefaultField < nStatusBarFields )
      widths[ mStatus.mDefaultField + 1 ] = -1;
   statusBar->SetStatusWidths( nWidths, widths.data() );
}

void ProjectWindowBase::SetStatusText(const wxString &text, int number)
{
   // Start from 1 not 0
   // Specifying a first column always of width 0 was needed for reasons
   // I forget now
   wxFrame::SetStatusText( text, number + 1 );
}

void ProjectBase::SetFrame( ProjectWindowBase *pFrame )
{
   mFrame = pFrame;
}

void ProjectBase::SetPanel( wxWindow *pPanel )
{
   mPanel = pPanel;
}

AUDACITY_DLL_API ProjectWindowBase &GetProjectFrame( ProjectBase &project )
{
   auto ptr = project.GetFrame();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

AUDACITY_DLL_API
   const ProjectWindowBase &GetProjectFrame( const ProjectBase &project )
{
   auto ptr = project.GetFrame();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

ProjectWindowBase *FindProjectFrame( ProjectBase *project )
{
   ProjectWindowBase *result = nullptr;
   if ( project )
      result = project->GetFrame();
   return result;
}

const ProjectWindowBase *FindProjectFrame( const ProjectBase *project )
{
   return FindProjectFrame( const_cast< ProjectBase* >( project ) );
}

AUDACITY_DLL_API wxWindow &GetProjectPanel( ProjectBase &project )
{
   auto ptr = project.GetPanel();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}

AUDACITY_DLL_API const wxWindow &GetProjectPanel(
   const ProjectBase &project )
{
   auto ptr = project.GetPanel();
   if ( !ptr )
      THROW_INCONSISTENCY_EXCEPTION;
   return *ptr;
}
