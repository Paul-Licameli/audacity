/**********************************************************************

Audacity: A Digital Audio Editor

TimeTrackView.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "TimeTrackView.h"
#include "TimeTrack.h"

#include "TimeTrackControls.h"

#include "TimeTrackVRulerControls.h"
#include "AColor.h"
#include "AllThemeResources.h"
#include "Envelope.h"
#include "EnvelopeEditor.h"
#include "HitTestResult.h"
#include "Theme.h"
#include "TrackArtist.h"
#include "TrackPanelDrawingContext.h"
#include "TrackPanelMouseEvent.h"
#include "ViewInfo.h"
#include "widgets/Ruler.h"

#include "EnvelopeHandle.h"

#include <wx/dc.h>

using Doubles = ArrayOf<double>;

TimeTrackView::TimeTrackView(
   const std::shared_ptr<Track> &pTrack, const ZoomInfo &zoomInfo )
   : CommonTrackView{ pTrack }
{
   mRuler = std::make_unique<Ruler>();
   mRuler->SetUseZoomInfo(0, &zoomInfo);
   mRuler->SetLabelEdges(false);
   mRuler->SetFormat(Ruler::TimeFormat);
}

TimeTrackView::~TimeTrackView()
{
}

namespace {
   void GetTimeTrackData
      (const AudacityProject &project, const TimeTrack &tt,
       double &dBRange, bool &dB, float &zoomMin, float &zoomMax)
   {
      const auto &viewInfo = ViewInfo::Get( project );
      dBRange = viewInfo.dBr;
      dB = tt.GetDisplayLog();
      zoomMin = tt.GetRangeLower(), zoomMax = tt.GetRangeUpper();
      if (dB) {
         // MB: silly way to undo the work of GetWaveYPos while still getting a logarithmic scale
         zoomMin = LINEAR_TO_DB(std::max(1.0e-7, double(zoomMin))) / dBRange + 1.0;
         zoomMax = LINEAR_TO_DB(std::max(1.0e-7, double(zoomMax))) / dBRange + 1.0;
      }
   }

   static UIHandlePtr EnvelopeHitTest
   (std::weak_ptr<EnvelopeHandle> &holder,
    const wxMouseState &state, const wxRect &rect,
    const AudacityProject *pProject, const std::shared_ptr<TimeTrack> &tt)
   {
      const auto envelope = tt->GetEnvelope();
      if (!envelope)
         return {};
      EnvelopeHandle::Data data;
      GetTimeTrackData( *pProject, *tt,
         data.mdBRange, data.mLog, data.mLower, data.mUpper);
      data.mEnvelopeEditors.push_back(
         std::make_unique< EnvelopeEditor >( *envelope, false )
      );
      data.mMessage =
         XO("Click and drag to warp playback time");

      return EnvelopeHandle::HitEnvelope(holder, state, rect, pProject,
         std::move(data));
   }
}

std::vector<UIHandlePtr> TimeTrackView::DetailedHitTest
(const TrackPanelMouseState &st,
 const AudacityProject *pProject, int, bool)
{
   std::vector<UIHandlePtr> results;
   auto result = EnvelopeHitTest
      ( mEnvelopeHandle, st.state, st.rect, pProject,
        std::static_pointer_cast< TimeTrack >( FindTrack() ) );
   if (result)
      results.push_back(result);
   return results;
}

using DoGetTimeTrackView = DoGetView::Override< TimeTrack >;
DEFINE_ATTACHED_VIRTUAL_OVERRIDE(DoGetTimeTrackView) {
   return [](TimeTrack &track) {
      return std::make_shared<TimeTrackView>(
         track.SharedPointer(), track.GetZoomInfo() );
   };
}

using DoGetTimeTrackVRulerControls =
   DoGetVRulerControls::Override< TimeTrackView >;
DEFINE_ATTACHED_VIRTUAL_OVERRIDE(DoGetTimeTrackVRulerControls) {
   return [](TimeTrackView &view) {
      return std::make_shared<TimeTrackVRulerControls>( view.shared_from_this() );
   };
}

namespace {
void DrawHorzRulerAndCurve
( TrackPanelDrawingContext &context, const wxRect & r,
  const TimeTrack &track, Ruler &ruler )
{
   auto &dc = context.dc;
   const auto artist = TrackArtist::Get( context );
   const auto &zoomInfo = artist->zoomInfo;
   
   bool highlight = false;
#ifdef EXPERIMENTAL_TRACK_PANEL_HIGHLIGHTING
   auto target = dynamic_cast<EnvelopeHandle*>(context.target.get());
   highlight = target && target->GetEnvelope() == this->GetEnvelope();
#endif
   
   double min = zoomInfo.PositionToTime(0);
   double max = zoomInfo.PositionToTime(r.width);
   if (min > max)
   {
      wxASSERT(false);
      min = max;
   }
   
   AColor::UseThemeColour( &dc, clrUnselected );
   dc.DrawRectangle(r);
   
   //copy this rectangle away for future use.
   wxRect mid = r;
   
   // Draw the Ruler
   ruler.SetBounds(r.x, r.y, r.x + r.width - 1, r.y + r.height - 1);
   ruler.SetRange(min, max);
   ruler.SetFlip(true); // tick marks at top of track.
   ruler.Invalidate();  // otherwise does not redraw.
   ruler.SetTickColour( theTheme.Colour( clrTrackPanelText ));
   ruler.Draw(dc, track.GetEnvelope());
   
   Doubles envValues{ size_t(mid.width) };
   Envelope::GetValues( *track.GetEnvelope(),
    0, 0, envValues.get(), mid.width, 0, zoomInfo );
   
   wxPen &pen = highlight ? AColor::uglyPen : AColor::envelopePen;
   dc.SetPen( pen );
   
   auto rangeLower = track.GetRangeLower(), rangeUpper = track.GetRangeUpper();
   double logLower = log(std::max(1.0e-7, rangeLower)),
      logUpper = log(std::max(1.0e-7, rangeUpper));

   for (int x = 0; x < mid.width; x++)
   {
      double y;
      if ( track.GetDisplayLog() )
         y = (double)mid.height * (logUpper - log(envValues[x])) / (logUpper - logLower);
      else
         y = (double)mid.height * (rangeUpper - envValues[x]) / (rangeUpper - rangeLower);
      int thisy = r.y + (int)y;
      AColor::Line(dc, mid.x + x, thisy - 1, mid.x + x, thisy+2);
   }
}

void DrawTimeTrack(TrackPanelDrawingContext &context,
                                const TimeTrack &track, Ruler &ruler,
                                const wxRect & rect)
{
   // Ruler and curve...
   DrawHorzRulerAndCurve( context, rect, track, ruler );

   // ... then the control points
   wxRect envRect = rect;
   envRect.height -= 2;
   double lower = track.GetRangeLower(), upper = track.GetRangeUpper();
   const auto artist = TrackArtist::Get( context );
   const auto dbRange = artist->mdBrange;
   if(track.GetDisplayLog()) {
      // MB: silly way to undo the work of GetWaveYPos while still getting a logarithmic scale
      lower = LINEAR_TO_DB(std::max(1.0e-7, lower)) / dbRange + 1.0;
      upper = LINEAR_TO_DB(std::max(1.0e-7, upper)) / dbRange + 1.0;
   }
   EnvelopeEditor::DrawPoints( *track.GetEnvelope(),
        context.dc, artist->zoomInfo, envRect,
        track.GetDisplayLog(), dbRange, lower, upper, false );
}
}

void TimeTrackView::Draw(
   TrackPanelDrawingContext &context,
   const wxRect &rect, unsigned iPass )
{
   CommonTrackView::Draw( context, rect, iPass );
   if ( iPass == TrackArtist::PassTracks ) {
      const auto tt = std::static_pointer_cast<const TimeTrack>(
         FindTrack()->SubstitutePendingChangedTrack());
      DrawTimeTrack( context, *tt, GetRuler(), rect );
   }
}

#include "ModuleConstants.h"
DEFINE_MODULE_ENTRIES
