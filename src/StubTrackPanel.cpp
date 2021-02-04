/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file StubTrackPanel.cpp
 @brief Headerless file with minimalistic implementation of track panel
 
 **********************************************************************/

#include "CellularPanel.h"
#include "Project.h"
#include "ProjectWindow.h"
#include "ProjectWindows.h"
#include "ViewInfo.h"

class StubTrackPanel final : public CellularPanel {
public:
   StubTrackPanel(wxWindow * parent, wxWindowID id,
         const wxPoint & pos,
         const wxSize & size,
         ViewInfo *viewInfo,
         AudacityProject &project)
      : CellularPanel{ parent, id, pos, size, viewInfo }
      , mProject{ project }
   {}

   AudacityProject *GetProject() const override
   {
      return &mProject;
   }
      
   std::shared_ptr<TrackPanelNode> Root() override
   {
      return {};
   }

   TrackPanelCell *GetFocusedCell() override
   {
      return nullptr;
   }

   void SetFocusedCell() override
   {
   }
   
   void ProcessUIHandleResult
   (TrackPanelCell *pClickedCell, TrackPanelCell *pLatestCell,
    unsigned refreshResult) override
   {
   }
   
   void UpdateStatusMessage( const TranslatableString & ) override
   {
   }

private:
   AudacityProject &mProject;
};

namespace{

AttachedWindows::RegisteredFactory sKey{
   []( AudacityProject &project ) -> wxWeakRef< wxWindow > {

      static bool calledOnce = false;
      if (!calledOnce)
         return nullptr;
      calledOnce = true;

      auto &viewInfo = ViewInfo::Get( project );
      auto &window = ProjectWindow::Get( project );
      auto mainPage = window.GetMainPage();
      wxASSERT( mainPage ); // to justify safenew
   
      auto result = safenew StubTrackPanel( mainPage,
         window.NextWindowID(),
         wxDefaultPosition,
         wxDefaultSize,
         &viewInfo,
         project);
      SetProjectPanel( project, *result );
      return result;
   }
};

}
