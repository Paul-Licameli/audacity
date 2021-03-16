/**********************************************************************

Audacity: A Digital Audio Editor

EnvelopeHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/


#include "EnvelopeHandle.h"

#include "TrackView.h"

#include "../../Envelope.h"
#include "../../EnvelopeEditor.h"
#include "../../HitTestResult.h"
#include "../../ProjectAudioIO.h"
#include "../../ProjectHistory.h"
#include "../../RefreshCode.h"
#include "../../TrackArt.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../ViewInfo.h"
#include "../../../images/Cursors.h"

EnvelopeHandle::EnvelopeHandle( Data data )
   : mData{ std::move(data) }
{
}

const Envelope *EnvelopeHandle::GetEnvelope() const
{
   if (mData.mEnvelopeEditors.empty())
      return nullptr;
   else
      return &mData.mEnvelopeEditors[0]->GetEnvelope();
}

void EnvelopeHandle::Enter(bool, AudacityProject *)
{
#ifdef EXPERIMENTAL_TRACK_PANEL_HIGHLIGHTING
   mChangeHighlight = RefreshCode::RefreshCell;
#endif
}

EnvelopeHandle::~EnvelopeHandle()
{}

UIHandlePtr EnvelopeHandle::HitAnywhere(
   std::weak_ptr<EnvelopeHandle> & WXUNUSED(holder),
   Data data)
{
   auto result = std::make_shared<EnvelopeHandle>( std::move(data) );
   return result;
}

UIHandlePtr EnvelopeHandle::HitEnvelope(
   std::weak_ptr<EnvelopeHandle> &holder,
   const wxMouseState &state,
   const wxRect &rect, const AudacityProject *pProject,
   Data data)
{
   wxASSERT(!data.mEnvelopeEditors.empty());
   auto &envelope = data.mEnvelopeEditors[0]->GetEnvelope();
   const auto &viewInfo = ViewInfo::Get( *pProject );

   const double envValue =
      envelope.GetValue(viewInfo.PositionToTime(state.m_x, rect.x));

   // Get y position of envelope point.
   int yValue = GetWaveYPos(envValue,
      data.mLower, data.mUpper,
      rect.height, data.mLog, true, data.mdBRange, false) + rect.y;

   // Get y position of center line
   int ctr = GetWaveYPos(0.0,
      data.mLower, data.mUpper,
      rect.height, data.mLog, true, data.mdBRange, false) + rect.y;

   // Get y distance of mouse from center line (in pixels).
   int yMouse = abs(ctr - state.m_y);
   // Get y distance of envelope from center line (in pixels)
   yValue = abs(ctr - yValue);

   // JKC: It happens that the envelope is actually drawn offset from its
   // 'true' position (it is 3 pixels wide).  yMisalign is really a fudge
   // factor to allow us to hit it exactly, but I wouldn't dream of
   // calling it yFudgeFactor :)
   const int yMisalign = 2;
   // Perhaps yTolerance should be put into preferences?
   const int yTolerance = 5; // how far from envelope we may be and count as a hit.
   int distance;

   // For amplification using the envelope we introduced the idea of contours.
   // The contours have the same shape as the envelope, which may be partially off-screen.
   // The contours are closer in to the center line.
   int ContourSpacing = (int)(rect.height / (2 * (data.mUpper - data.mLower)));
   const int MaxContours = 2;

   // Adding ContourSpacing/2 selects a region either side of the contour.
   int yDisplace = yValue - yMisalign - yMouse + ContourSpacing / 2;
   if (yDisplace > (MaxContours * ContourSpacing))
      return {};
   // Subtracting the ContourSpacing/2 we added earlier ensures distance is centred on the contour.
   distance = abs((yDisplace % ContourSpacing) - ContourSpacing / 2);
   if (distance >= yTolerance)
      return {};

   return HitAnywhere(holder, std::move(data));
}

UIHandle::Result EnvelopeHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   const bool unsafe = ProjectAudioIO::Get( *pProject ).IsAudioActive();
   if ( unsafe )
      return Cancelled;

   const wxMouseEvent &event = evt.event;
   const auto &viewInfo = ViewInfo::Get( *pProject );
   const auto pView = std::static_pointer_cast<TrackView>(evt.pCell);
   const auto pTrack = pView ? pView->FindTrack().get() : nullptr;

   unsigned result = Cancelled;
   if (mData.mEnvelopeEditors.empty())
      return result;

   mRect = evt.rect;

   const bool needUpdate = ForwardEventToEnvelopes(event, viewInfo);
   return needUpdate ? RefreshCell : RefreshNone;
}

UIHandle::Result EnvelopeHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   const wxMouseEvent &event = evt.event;
   const auto &viewInfo = ViewInfo::Get( *pProject );
   const bool unsafe = ProjectAudioIO::Get( *pProject ).IsAudioActive();
   if (unsafe) {
      this->Cancel(pProject);
      return RefreshCell | Cancelled;
   }

   const bool needUpdate = ForwardEventToEnvelopes(event, viewInfo);
   return needUpdate ? RefreshCell : RefreshNone;
}

HitTestPreview EnvelopeHandle::Preview
(const TrackPanelMouseState &, AudacityProject *pProject)
{
   const bool unsafe = ProjectAudioIO::Get( *pProject ).IsAudioActive();
   static auto disabledCursor =
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16);
   static auto envelopeCursor =
      ::MakeCursor(wxCURSOR_ARROW, EnvCursorXpm, 16, 16);

   return {
      mData.mMessage,
      (unsafe
       ? &*disabledCursor
       : &*envelopeCursor)
   };
}

UIHandle::Result EnvelopeHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *)
{
   const wxMouseEvent &event = evt.event;
   const auto &viewInfo = ViewInfo::Get( *pProject );
   const bool unsafe = ProjectAudioIO::Get( *pProject ).IsAudioActive();
   if (unsafe)
      return this->Cancel(pProject);

   const bool needUpdate = ForwardEventToEnvelopes(event, viewInfo);

   ProjectHistory::Get( *pProject ).PushState(
      /* i18n-hint: (verb) Audacity has just adjusted the envelope .*/
      XO("Adjusted envelope."),
      /* i18n-hint: The envelope is a curve that controls the audio loudness.*/
      XO("Envelope")
   );

   mData.mEnvelopeEditors.clear();

   using namespace RefreshCode;
   return needUpdate ? RefreshCell : RefreshNone;
}

UIHandle::Result EnvelopeHandle::Cancel(AudacityProject *pProject)
{
   ProjectHistory::Get( *pProject ).RollbackState();
   mData.mEnvelopeEditors.clear();
   return RefreshCode::RefreshCell;
}

bool EnvelopeHandle::ForwardEventToEnvelopes
   (const wxMouseEvent &event, const ViewInfo &viewInfo)
{
   /// The Envelope class actually handles things at the mouse
   /// event level, so we have to forward the events over.  Envelope
   /// will then tell us whether or not we need to redraw.

   // AS: I'm not sure why we can't let the Envelope take care of
   //  redrawing itself.  ?
   bool needUpdate = false;
   for (const auto &pEditor : mData.mEnvelopeEditors) {
      needUpdate =
         pEditor->MouseEvent(
            event, mRect, viewInfo, mData.mLog,
            mData.mdBRange, mData.mLower, mData.mUpper)
         || needUpdate;
   }

   return needUpdate;
}
