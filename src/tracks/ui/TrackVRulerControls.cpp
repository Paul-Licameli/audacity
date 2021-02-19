/**********************************************************************

Audacity: A Digital Audio Editor

TrackVRulerControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/


#include "TrackVRulerControls.h"

#include "TrackView.h"

#include "AColor.h"
#include "../../Track.h"
#include "../../TrackArtist.h"
#include "../../TrackPanelDrawingContext.h"
#include "../../ViewInfo.h"

#include <wx/cursor.h>
#include <wx/dc.h>
#include <wx/translation.h>

TrackVRulerControls::TrackVRulerControls(
   const std::shared_ptr<TrackView> &pTrackView )
  : mwTrackView{ pTrackView }
{
}

TrackVRulerControls::~TrackVRulerControls()
{
}

DEFINE_ATTACHED_VIRTUAL(DoGetVRulerControls) {
   return [](auto &trackView) {
      // Default returns just a stub implementation
      return std::make_shared<TrackVRulerControls>(trackView.shared_from_this());
   };
}

static AttachedTrackViewCells::RegisteredFactory
sKey{ [](auto&){return nullptr;} };

TrackVRulerControls &TrackVRulerControls::Get( TrackView &trackView )
{
   if (auto result = trackView.AttachedTrackViewCells::Find(sKey))
      return *static_cast<TrackVRulerControls *>(result);

   // Create on demand
   auto sresult = DoGetVRulerControls::Call(trackView);
   auto result = sresult.get();
   wxASSERT(result);
   trackView.AttachedTrackViewCells::Assign(sKey, std::move(sresult));
   return *result;
}

const TrackVRulerControls &TrackVRulerControls::Get( const TrackView &trackView )
{
   return *DoGetVRulerControls::Call(const_cast<TrackView&>(trackView));
}

void TrackVRulerControls::UpdateRuler( const wxRect & )
{
}

DEFINE_ATTACHED_VIRTUAL(DoGetAffordanceControls) {
   return [](auto &trackView) {
      return nullptr;
   };
}
static AttachedTrackViewCells::RegisteredFactory
sKey0{ [](auto&){return nullptr;} };

TrackAffordanceControls &TrackAffordanceControls::Get( TrackView &trackView )
{
   if (auto result = trackView.AttachedTrackViewCells::Find(sKey0))
      return *static_cast<TrackAffordanceControls *>(result);

   // Create on demand
   auto sresult = DoGetAffordanceControls::Call(trackView);
   auto result = sresult.get();
   wxASSERT(result);
   trackView.AttachedTrackViewCells::Assign(sKey, std::move(sresult));
   return *result;
}

std::shared_ptr<Track> TrackVRulerControls::DoFindTrack()
{
   const auto pView = mwTrackView.lock();
   if ( pView )
      return pView->FindTrack();
   return {};
}

std::vector<UIHandlePtr> TrackVRulerControls::HitTest
(const TrackPanelMouseState &, const AudacityProject *)
{
   return std::vector<UIHandlePtr>{};
}

void TrackVRulerControls::DrawZooming
   ( TrackPanelDrawingContext &context, const wxRect &rect_,
     int zoomStart, int zoomEnd )
{
   // Draw a dashed rectangle, its right side disappearing in the black right
   // border of the track area, which is not part of this cell but right of it.
   auto &dc = context.dc;

   dc.SetBrush(*wxTRANSPARENT_BRUSH);
   dc.SetPen(*wxBLACK_DASHED_PEN);

   wxRect rect {
      rect_.x,
      std::min( zoomStart, zoomEnd),
      rect_.width,
      1 + abs( zoomEnd - zoomStart)
   };

   dc.DrawRectangle(rect);
}

wxRect TrackVRulerControls::ZoomingArea(
   const wxRect &rect, const wxRect &panelRect )
{
   // TODO: Don't use the constant kRightMargin, but somehow discover the
   // neighboring track rectangle
   return {
      // Left edge of the rectangle disappears in the vertical line at
      // left edge of the ruler
      rect.x,
      rect.y,
      // Extend the dashed rectangle right up to the track border
      (panelRect.width - kRightMargin + kBorderThickness) - rect.x,
      rect.height
   };
}

void TrackVRulerControls::Draw(
   TrackPanelDrawingContext &context,
   const wxRect &rect_, unsigned iPass )
{
   // Common initial part of drawing for all subtypes
   if ( iPass == TrackArtist::PassMargins ) {
      auto rect = rect_;
      --rect.width;
      
      auto dc = &context.dc;
      
      
      // Paint the background
      auto pTrack = FindTrack();
      AColor::MediumTrackInfo(dc, pTrack && pTrack->GetSelected() );
      dc->DrawRectangle( rect );
      
      // Stroke the left border
      dc->SetPen(*wxBLACK_PEN);
      {
         const auto left = rect.GetLeft();
         AColor::Line( *dc, left, rect.GetTop(), left, rect.GetBottom() );
      }
   }
}

wxRect TrackVRulerControls::DrawingArea(
   TrackPanelDrawingContext &,
   const wxRect &rect, const wxRect &, unsigned iPass )
{
   // Common area change for all subclasses when drawing the controls
   // A bevel extends below one pixel outside of the hit-test area
   if ( iPass == TrackArtist::PassControls )
      return { rect.x, rect.y, rect.width, rect.height + 1 };
   else
      return rect;
}
