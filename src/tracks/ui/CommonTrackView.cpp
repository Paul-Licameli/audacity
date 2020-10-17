/**********************************************************************

Audacity: A Digital Audio Editor

CommonTrackView.cpp

Paul Licameli split from class TrackView

**********************************************************************/

#include "CommonTrackView.h"

#include "TimeShiftHandle.h"
#include "TrackControls.h"
#include "ZoomHandle.h"
#include "../ui/SelectHandle.h"
#include "../../AColor.h"
#include "../../ProjectSettings.h"
#include "../../Track.h"
#include "../../TrackArtist.h"
#include "../../TrackInfo.h"
#include "../../TrackPanelDrawingContext.h"
#include "../../TrackPanelMouseEvent.h"

#include <wx/dc.h>
#include <wx/graphics.h>

std::vector<UIHandlePtr> CommonTrackView::HitTest
(const TrackPanelMouseState &st,
 const AudacityProject *pProject)
{
   UIHandlePtr result;
   using namespace ToolCodes;
   std::vector<UIHandlePtr> results;
   const auto &settings = ProjectSettings::Get( *pProject );
   const auto currentTool = settings.GetTool();
   const bool isMultiTool = ( currentTool == multiTool );

   if ( !isMultiTool && currentTool == zoomTool ) {
      // Zoom tool is a non-selecting tool that takes precedence in all tracks
      // over all other tools, no matter what detail you point at.
      result = ZoomHandle::HitAnywhere(
         mZoomHandle );
      results.push_back(result);
      return results;
   }

   // In other tools, let subclasses determine detailed hits.
   results =
      DetailedHitTest( st, pProject, currentTool, isMultiTool );

   // There are still some general cases.

   // Sliding applies in more than one track type.
   if ( !isMultiTool && currentTool == slideTool ) {
      result = TimeShiftHandle::HitAnywhere(
         mTimeShiftHandle, FindTrack(), false);
      if (result)
         results.push_back(result);
   }

   // Let the multi-tool right-click handler apply only in default of all
   // other detailed hits.
   if ( isMultiTool ) {
      result = ZoomHandle::HitTest(mZoomHandle, st.state);
      if (result)
         results.push_back(result);
   }

   // Finally, default of all is adjustment of the selection box.
   if ( isMultiTool || currentTool == selectTool ) {
      result = SelectionHitTest( mSelectHandle, st, pProject );
      if (result)
         results.push_back(result);
   }

   return results;
}

UIHandlePtr CommonTrackView::SelectionHitTest(
    std::weak_ptr<UIHandle> &mSelectHandle,
    const TrackPanelMouseState &state, const AudacityProject *pProject)
{
   auto factory = [](
      const std::shared_ptr<TrackView> &pTrackView,
      bool oldUseSnap,
      const TrackList &trackList,
      const TrackPanelMouseState &st,
      const ViewInfo &viewInfo) -> UIHandlePtr {
         return std::make_shared<SelectHandle>(
            pTrackView, oldUseSnap, trackList, st, viewInfo );
   };
   return SelectHandle::HitTest( factory,
      mSelectHandle, state, pProject, shared_from_this() );
}

std::shared_ptr<TrackPanelCell> CommonTrackView::ContextMenuDelegate()
{
   return TrackControls::Get( *FindTrack() ).shared_from_this();
}

int CommonTrackView::GetMinimizedHeight() const
{
   auto height = TrackInfo::MinimumTrackHeight();
   const auto pTrack = FindTrack();
   auto channels = TrackList::Channels(pTrack->SubstituteOriginalTrack().get());
   auto nChannels = channels.size();
   auto begin = channels.begin();
   auto index =
      std::distance(begin, std::find(begin, channels.end(), pTrack.get()));
   return (height * (index + 1) / nChannels) - (height * index / nChannels);
}
