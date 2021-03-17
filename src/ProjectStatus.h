/**********************************************************************

Audacity: A Digital Audio Editor

ProjectStatus.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_PROJECT_STATUS__
#define __AUDACITY_PROJECT_STATUS__
#endif

#include <utility>
#include <vector>
#include "Project.h"
#include "Prefs.h"

class AudacityProject;
class wxWindow;

enum StatusBarField : int {
   stateStatusBarField = 1,
   mainStatusBarField = 2,
   rateStatusBarField = 3,
   
   nStatusBarFields = 3
};

struct AUDACITY_DLL_API ProjectStatusEvent final : wxEvent{
   explicit ProjectStatusEvent( StatusBarField field );
   ~ProjectStatusEvent() override;
   wxEvent *Clone() const override;
   StatusBarField mField;
};

// Type of event emitted by the project when its status message is set
// GetInt() identifies the intended field of the status bar
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_PROJECT_STATUS_UPDATE, ProjectStatusEvent);

class AUDACITY_DLL_API ProjectStatus final
   : public AttachedProjectObject
   , public PrefsListener
{
public:
   static ProjectStatus &Get( AudacityProject &project );
   static const ProjectStatus &Get( const AudacityProject &project );

   explicit ProjectStatus( AudacityProject &project );
   ProjectStatus( const ProjectStatus & ) = delete;
   ProjectStatus &operator= ( const ProjectStatus & ) = delete;
   ~ProjectStatus() override;

   // Type of a function to report translatable strings, and also report an extra
   // margin, to request that the corresponding field of the status bar should
   // be wide enough to contain any of those strings plus the margin.
   using StatusWidthResult = std::pair< std::vector<TranslatableString>, unsigned >;
   using StatusWidthFunction = std::function<
      StatusWidthResult( const AudacityProject &, StatusBarField )
   >;
   using StatusWidthFunctions = std::vector< StatusWidthFunction >;

   // Typically a static instance of this struct is used.
   struct AUDACITY_DLL_API RegisteredStatusWidthFunction
   {
      explicit
      RegisteredStatusWidthFunction( const StatusWidthFunction &function );
   };

   static const StatusWidthFunctions &GetStatusWidthFunctions();

   const TranslatableString &Get( StatusBarField field = mainStatusBarField ) const;
   void Set(const TranslatableString &msg,
      StatusBarField field = mainStatusBarField);

   // PrefsListener implementation
   void UpdatePrefs() override;

private:
   AudacityProject &mProject;
   TranslatableString mLastStatusMessages[ nStatusBarFields ];
};
