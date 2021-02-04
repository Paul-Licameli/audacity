/**********************************************************************

Audacity: A Digital Audio Editor

SelectHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "SelectHandle.h"

#include "tracks/ui/Scrubbing.h"
#include "tracks/ui/TrackView.h"

#include "AColor.h"
#include "HitTestResult.h"
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectHistory.h"
#include "ProjectSettings.h"
#include "ProjectWindow.h"
#include "ProjectWindows.h"
#include "RefreshCode.h"
#include "SelectUtilities.h"
#include "SelectionState.h"
#include "TrackArtist.h"
#include "TrackPanelAx.h"
#include "TrackPanelDrawingContext.h"
#include "TrackPanelMouseEvent.h"
#include "ViewInfo.h"
#include "WaveClip.h"
#include "WaveTrack.h"
#include "../images/Cursors.h"

// Only for definition of SonifyBeginModifyState:
//#include "NoteTrack.h"

enum {
   //This constant determines the size of the horizontal region (in pixels) around
   //the right and left selection bounds that can be used for horizontal selection adjusting
   //(or, vertical distance around top and bottom bounds in spectrograms,
   // for vertical selection adjusting)
   SELECTION_RESIZE_REGION = 3,
};

bool SelectHandle::IsClicked() const
{
   return mSelectionStateChanger.get() != NULL;
}

namespace
{
   template<typename T>
   inline void SetIfNotNull(T * pValue, const T Value)
   {
      if (pValue == NULL)
         return;
      *pValue = Value;
   }

   SelectionBoundary ChooseTimeBoundary
      (
      const double t0, const double t1,
      const ViewInfo &viewInfo,
      double selend, bool onlyWithinSnapDistance,
      wxInt64 *pPixelDist, double *pPinValue)
   {
      const wxInt64 posS = viewInfo.TimeToPosition(selend);
      const wxInt64 pos0 = viewInfo.TimeToPosition(t0);
      wxInt64 pixelDist = std::abs(posS - pos0);
      bool chooseLeft = true;

      if (t1<=t0)
         // Special case when selection is a point, and thus left
         // and right distances are the same
         chooseLeft = (selend < t0);
      else {
         const wxInt64 pos1 = viewInfo.TimeToPosition(t1);
         const wxInt64 rightDist = std::abs(posS - pos1);
         if (rightDist < pixelDist)
            chooseLeft = false, pixelDist = rightDist;
      }

      SetIfNotNull(pPixelDist, pixelDist);

      if (onlyWithinSnapDistance &&
         pixelDist >= SELECTION_RESIZE_REGION) {
         SetIfNotNull(pPinValue, -1.0);
         return SBNone;
      }
      else if (chooseLeft) {
         SetIfNotNull(pPinValue, t1);
         return SBLeft;
      }
      else {
         SetIfNotNull(pPinValue, t0);
         return SBRight;
      }
   }
}

int SelectHandle::ChooseBoundary
   (const ViewInfo &viewInfo,
    wxCoord xx, wxCoord yy, const TrackView *pTrackView, const wxRect &rect,
    bool mayDragWidth, bool onlyWithinSnapDistance,
    double *pPinValue)
{
   // May choose no boundary if onlyWithinSnapDistance is true.
   // Otherwise choose the eligible boundary nearest the mouse click.
   const double selend = viewInfo.PositionToTime(xx, rect.x);
   wxInt64 pixelDist = 0;
   const double t0 = viewInfo.selectedRegion.t0();
   const double t1 = viewInfo.selectedRegion.t1();

   return
      ChooseTimeBoundary(t0, t1, viewInfo, selend, onlyWithinSnapDistance,
      &pixelDist, pPinValue);
}

namespace {
   wxCursor *SelectCursor()
   {
      static auto selectCursor =
         ::MakeCursor(wxCURSOR_IBEAM, IBeamCursorXpm, 17, 16);
      return &*selectCursor;
   }
}

void SelectHandle::SetTipAndCursorForBoundary(
   int boundary, bool,
   TranslatableString &tip, wxCursor *&pCursor)
{
   static wxCursor adjustLeftSelectionCursor{ wxCURSOR_POINT_LEFT };
   static wxCursor adjustRightSelectionCursor{ wxCURSOR_POINT_RIGHT };

   switch (boundary) {
   case SBNone:
      pCursor = SelectCursor();
      break;
   case SBLeft:
      tip = XO("Click and drag to move left selection boundary.");
      pCursor = &adjustLeftSelectionCursor;
      break;
   case SBRight:
      tip = XO("Click and drag to move right selection boundary.");
      pCursor = &adjustRightSelectionCursor;
      break;
   default:
      wxASSERT(false);
   } // switch
   // Falls through the switch if there was no boundary found.
}

UIHandlePtr SelectHandle::HitTest( SelectHandleFactory factory,
 std::weak_ptr<UIHandle> &holder,
 const TrackPanelMouseState &st, const AudacityProject *pProject,
 const std::shared_ptr<TrackView> &pTrackView)
{
   // This handle is a little special because there may be some state to
   // preserve during movement before the click.
   auto old = std::dynamic_pointer_cast<SelectHandle>( holder.lock() );
   bool oldUseSnap = true;
   if (old) {
      // It should not have started listening to timer events
      if( old->mTimerHandler ) {
         wxASSERT(false);
         // Handle this eventuality anyway, don't leave a dangling back-pointer
         // in the attached event handler.
         old->mTimerHandler.reset();
      }
      oldUseSnap = old->mUseSnap;
   }

   const auto &viewInfo = ViewInfo::Get( *pProject );
   auto result = factory(
      pTrackView, oldUseSnap, TrackList::Get( *pProject ), st, viewInfo);

   result = AssignUIHandlePtr(holder, result);

   //Make sure we are within the selected track
   // Adjusting the selection edges can be turned off in
   // the preferences...
   auto pTrack = pTrackView->FindTrack();
   if (!pTrack->GetSelected() || !viewInfo.bAdjustSelectionEdges)
   {
      return result;
   }

   {
      const wxRect &rect = st.rect;
      wxInt64 leftSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t0(), rect.x);
      wxInt64 rightSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t1(), rect.x);
      // Something is wrong if right edge comes before left edge
      wxASSERT(!(rightSel < leftSel));
      static_cast<void>(leftSel); // Suppress unused variable warnings if not in debug-mode
      static_cast<void>(rightSel);
   }

   return result;
}

UIHandle::Result SelectHandle::NeedChangeHighlight
(const SelectHandle &oldState, const SelectHandle &newState)
{
   auto useSnap = oldState.mUseSnap;
   // This is guaranteed when constructing the NEW handle:
   wxASSERT( useSnap == newState.mUseSnap );
   if (!useSnap)
      return 0;

   auto &oldSnapState = oldState.mSnapStart;
   auto &newSnapState = newState.mSnapStart;
   if ( oldSnapState.Snapped() == newSnapState.Snapped() &&
        (!oldSnapState.Snapped() ||
         oldSnapState.outCoord == newSnapState.outCoord) )
      return 0;

   return RefreshCode::RefreshAll;
}

SelectHandle::SelectHandle
( const std::shared_ptr<TrackView> &pTrackView, bool useSnap,
  const TrackList &trackList,
  const TrackPanelMouseState &st, const ViewInfo &viewInfo )
   : mpView{ pTrackView }
   , mSnapManager{ std::make_shared<SnapManager>(
      *trackList.GetOwner(), trackList, viewInfo) }
{
   const wxMouseState &state = st.state;
   mRect = st.rect;

   auto time = std::max(0.0, viewInfo.PositionToTime(state.m_x, mRect.x));
   auto pTrack = pTrackView->FindTrack();
   mSnapStart = mSnapManager->Snap(pTrack.get(), time, false);
   if (mSnapStart.snappedPoint)
         mSnapStart.outCoord += mRect.x;
   else
      mSnapStart.outCoord = -1;

   mUseSnap = useSnap;
}

SelectHandle::~SelectHandle()
{
}

namespace {
   // Is the distance between A and B less than D?
   template < class A, class B, class DIST > bool within(A a, B b, DIST d)
   {
      return (a > b - d) && (a < b + d);
   }
}

void SelectHandle::Enter(bool, AudacityProject *project)
{
   SetUseSnap(true, project);
}

void SelectHandle::SetUseSnap(bool use, AudacityProject *project)
{
   mUseSnap = use;

   bool hasSnap = HasSnap();
   if (hasSnap)
      // Repaint to turn the snap lines on or off
      mChangeHighlight = RefreshCode::RefreshAll;

   if (IsClicked()) {
      // Readjust the moving selection end
      AssignSelection(
         ViewInfo::Get( *project ),
         mUseSnap ? mSnapEnd.outTime : mSnapEnd.timeSnappedTime,
         nullptr);
   }
}

bool SelectHandle::HasSnap() const
{
   return
      (IsClicked() ? mSnapEnd : mSnapStart).snappedPoint;
}

bool SelectHandle::HasEscape() const
{
   return HasSnap() && mUseSnap;
}

bool SelectHandle::Escape(AudacityProject *project)
{
   if (SelectHandle::HasEscape()) {
      SetUseSnap(false, project);
      return true;
   }
   return false;
}

UIHandle::Result SelectHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   /// This method gets called when we're handling selection
   /// and the mouse was just clicked.

   using namespace RefreshCode;

   const auto pView = mpView.lock();
   if ( !pView )
      return Cancelled;

   wxMouseEvent &event = evt.event;
   auto &trackList = TrackList::Get( *pProject );
   const auto sTrack = trackList.Lock( FindTrack() );
   const auto pTrack = sTrack.get();
   auto &viewInfo = ViewInfo::Get( *pProject );

   mMostRecentX = event.m_x;
   mMostRecentY = event.m_y;

   auto &selectionState = SelectionState::Get( *pProject );
   const auto &settings = ProjectSettings::Get( *pProject );
   if (event.LeftDClick() && !event.ShiftDown()) {
      // Deselect all other tracks and select this one.
      selectionState.SelectNone( trackList );

      selectionState.SelectTrack( *pTrack, true, true );

      // Default behavior: select whole track
      SelectionState::SelectTrackLength
         ( viewInfo, *pTrack, settings.IsSyncLocked() );

      // Special case: if we're over a clip in a WaveTrack,
      // select just that clip
      pTrack->TypeSwitch( [&] ( WaveTrack *wt ) {
         auto time = viewInfo.PositionToTime(event.m_x, mRect.x);
         WaveClip *const selectedClip = wt->GetClipAtTime(time);
         if (selectedClip) {
            viewInfo.selectedRegion.setTimes(
               selectedClip->GetOffset(), selectedClip->GetEndTime());
         }
      } );

      ProjectHistory::Get( *pProject ).ModifyState(false);

      // Do not start a drag
      return RefreshAll | Cancelled;
   }
   else if (!event.LeftDown())
      return Cancelled;

   mInitialSelection = viewInfo.selectedRegion;

   mSelectionStateChanger =
      std::make_shared< SelectionStateChanger >( selectionState, trackList );

   mSelectionBoundary = 0;

   bool bShiftDown = event.ShiftDown();
   bool bCtrlDown = event.ControlDown();

   mSelStart = mUseSnap ? mSnapStart.outTime : mSnapStart.timeSnappedTime;

   // I. Shift-click adjusts an existing selection
   if (bShiftDown || bCtrlDown) {
      if (bShiftDown)
         selectionState.ChangeSelectionOnShiftClick( trackList, *pTrack );
      if( bCtrlDown ){
         //Commented out bIsSelected toggles, as in Track Control Panel.
         //bool bIsSelected = pTrack->GetSelected();
         //Actual bIsSelected will always add.
         bool bIsSelected = false;
         // Don't toggle away the last selected track.
         if( !bIsSelected || trackList.SelectedLeaders().size() > 1 )
            selectionState.SelectTrack( *pTrack, !bIsSelected, true );
      }

      ModifiedClick(evt, pProject, bShiftDown, bCtrlDown);

      // For persistence of the selection change:
      ProjectHistory::Get( *pProject ).ModifyState(false);

      // Get timer events so we can auto-scroll
      Connect(pProject);

      // Full refresh since the label area may need to indicate
      // newly selected tracks.
      return RefreshAll;
   }

   // II. Unmodified click may start a NEW selection
   if( UnmodifiedClick(evt, pProject) ) {
      // If we didn't move a selection boundary, start a NEW selection
      selectionState.SelectNone( trackList );
      selectionState.SelectTrack( *pTrack, true, true );
      TrackFocus::Get( *pProject ).Set(pTrack);

      Connect(pProject);
      return RefreshAll;
   }
   else {
      Connect(pProject);
      return RefreshAll;
   }
}

void SelectHandle::ModifiedClick(
   const TrackPanelMouseEvent &evt, AudacityProject *pProject,
   bool bShiftDown, bool bCtrlDown)
{
   const auto pTrack = TrackList::Get( *pProject ).Lock( FindTrack() ).get();
   auto &viewInfo = ViewInfo::Get( *pProject );

   const auto pView = mpView.lock();

   wxMouseEvent &event = evt.event;
   const auto xx = viewInfo.TimeToPosition(mSelStart, mRect.x);

   double value;
   // Shift-click, choose closest boundary
   auto boundary =
      ChooseBoundary(viewInfo, xx, event.m_y,
         pView.get(), mRect, false, false, &value);
   mSelectionBoundary = boundary;
   switch (boundary) {
      case SBLeft:
      case SBRight:
      {
         mSelStartValid = true;
         mSelStart = value;
         mSnapStart = SnapResults{};
         AdjustSelection(pProject, viewInfo, event.m_x, mRect.x, pTrack);
         break;
      }
      default:
         wxASSERT(false);
   };
}

bool SelectHandle::UnmodifiedClick(
   const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const auto pTrack = TrackList::Get( *pProject ).Lock( FindTrack() ).get();
   auto &viewInfo = ViewInfo::Get( *pProject );

   const auto pView = mpView.lock();

   wxMouseEvent &event = evt.event;
   const auto xx = viewInfo.TimeToPosition(mSelStart, mRect.x);

   //Make sure you are within the selected track
   bool startNewSelection = true;
   if (pTrack && pTrack->GetSelected()) {
      // Adjusting selection edges can be turned off in the
      // preferences now
      if (viewInfo.bAdjustSelectionEdges) {
         // Not shift-down, choose boundary only within snapping
         double value;
         auto boundary =
            ChooseBoundary(viewInfo, xx, event.m_y,
               pView.get(), mRect, true, true, &value);
         mSelectionBoundary = boundary;
         switch (boundary) {
         case SBNone:
            // startNewSelection remains true
            break;
         case SBLeft:
         case SBRight:
            startNewSelection = false;
            mSelStartValid = true;
            mSelStart = value;
            mSnapStart = SnapResults{};
            break;
         default:
            wxASSERT(false);
         }
      } // bAdjustSelectionEdges
   }

   if (startNewSelection)
      StartSelection(pProject);

   return startNewSelection;
}

UIHandle::Result SelectHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;

   const auto pView = mpView.lock();
   if ( !pView )
      return Cancelled;

   auto &viewInfo = ViewInfo::Get( *pProject );
   const wxMouseEvent &event = evt.event;

   int x = mAutoScrolling ? mMostRecentX : event.m_x;
   int y = mAutoScrolling ? mMostRecentY : event.m_y;
   mMostRecentX = x;
   mMostRecentY = y;

   /// AS: If we're dragging to adjust a selection (or actually,
   ///  if the screen is scrolling while you're selecting), we
   ///  handle it here.

   // Fuhggeddaboudit if we're not dragging and not autoscrolling.
   if (!event.Dragging() && !mAutoScrolling)
      return RefreshNone;

   if (event.CmdDown()) {
      // Ctrl-drag has no meaning, fuhggeddaboudit
      // JKC YES it has meaning.
      //return RefreshNone;
   }

   // Also fuhggeddaboudit if not in a track.
   auto pTrack = TrackList::Get( *pProject ).Lock( FindTrack() );
   if (!pTrack)
      return RefreshNone;

   // JKC: Logic to prevent a selection smaller than 5 pixels to
   // prevent accidental dragging when selecting.
   // (if user really wants a tiny selection, they should zoom in).
   // Can someone make this value of '5' configurable in
   // preferences?
   enum { minimumSizedSelection = 5 }; //measured in pixels

   if (mSelStartValid) {
      wxInt64 SelStart = viewInfo.TimeToPosition(mSelStart, mRect.x); //cvt time to pixels.
      // Abandon this drag if selecting < 5 pixels.
      if (wxLongLong(SelStart - x).Abs() < minimumSizedSelection)
         return RefreshNone;
   }

   if (evt.pCell) {
      if ( auto clickedTrack =
          static_cast<CommonTrackPanelCell*>(evt.pCell.get())->FindTrack() ) {
         DoDrag(*pProject, viewInfo, *pView, *clickedTrack, *pTrack,
            x, y, event.ControlDown());
      }
   }

   return RefreshNone

      // If scrubbing does not use the helper poller thread, then
      // don't refresh at every mouse event, because it slows down seek-scrub.
      // Instead, let OnTimer do it, which is often enough.
      // And even if scrubbing does use the thread, then skipping refresh does not
      // bring that advantage, but it is probably still a good idea anyway.

      // | UpdateSelection

   ;
}

void SelectHandle::DoDrag( AudacityProject &project,
   ViewInfo &viewInfo, TrackView &, Track &clickedTrack, Track &track,
   wxCoord x, wxCoord y, bool controlDown)
{
   // Handle which tracks are selected
   Track *sTrack = &track;
   Track *eTrack = &clickedTrack;
   auto &trackList = TrackList::Get( project );
   if ( sTrack && eTrack && !controlDown ) {
      auto &selectionState = SelectionState::Get( project );
      selectionState.SelectRangeOfTracks( trackList, *sTrack, *eTrack );
   }

   AdjustSelection(&project, viewInfo, x, mRect.x, &clickedTrack);
}

HitTestPreview SelectHandle::Preview
(const TrackPanelMouseState &st, AudacityProject *pProject)
{
   if (!HasSnap() && !mUseSnap)
      // Moved out of snapping; revert to un-escaped state
      mUseSnap = true;

   const auto pView = mpView.lock();
   if ( !pView )
      return {};

   auto pTrack = FindTrack().lock();
   if (!pTrack)
      return {};

   TranslatableString tip;
   wxCursor *pCursor = SelectCursor();
   if ( IsClicked() )
      // Use same cursor as at the click
      SetTipAndCursorForBoundary
         (SelectionBoundary(mSelectionBoundary),
          st.state.ShiftDown(),
          tip, pCursor);
   else {
      // Choose one of many cursors for mouse-over

      auto &viewInfo = ViewInfo::Get( *pProject );

      auto &state = st.state;
      auto time = mUseSnap ? mSnapStart.outTime : mSnapStart.timeSnappedTime;
      auto xx = viewInfo.TimeToPosition(time, mRect.x);

      const bool bMultiToolMode =
         (ToolCodes::multiTool == ProjectSettings::Get( *pProject ).GetTool());

      //In Multi-tool mode, give multitool prompt if no-special-hit.
      if (bMultiToolMode) {
         // Look up the current key binding for Preferences.
         // (Don't assume it's the default!)
         auto keyStr =
            CommandManager::Get( *pProject ).GetKeyFromName(wxT("Preferences"))
            .Display( true );
         if (keyStr.empty())
            // No keyboard preference defined for opening Preferences dialog
            /* i18n-hint: These are the names of a menu and a command in that menu */
            keyStr = _("Edit, Preferences...");
         
         /* i18n-hint: %s is usually replaced by "Ctrl+P" for Windows/Linux, "Command+," for Mac */
         tip = XO("Multi-Tool Mode: %s for Mouse and Keyboard Preferences.")
            .Format( keyStr );
         // Later in this function we may point to some other string instead.
         if (!pTrack->GetSelected() ||
             !viewInfo.bAdjustSelectionEdges)
            ;
         else {
            const wxRect &rect = st.rect;
            const bool bShiftDown = state.ShiftDown();
            const bool bCtrlDown = state.ControlDown();
            const bool bModifierDown = bShiftDown || bCtrlDown;

            // If not shift-down and not snapping center, then
            // choose boundaries only in snapping tolerance,
            // and may choose center.
            auto boundary =
            ChooseBoundary(viewInfo, xx, state.m_y,
               pView.get(), rect, !bModifierDown, !bModifierDown);

            SetTipAndCursorForBoundary(boundary, bShiftDown, tip, pCursor);
         }
      }

      if (!pTrack->GetSelected() || !viewInfo.bAdjustSelectionEdges)
         ;
      else {
         const wxRect &rect = st.rect;
         const bool bShiftDown = state.ShiftDown();
         const bool bCtrlDown = state.ControlDown();
         const bool bModifierDown = bShiftDown || bCtrlDown;
         auto boundary = ChooseBoundary(
            viewInfo, xx, state.m_y,
               pView.get(), rect, !bModifierDown, !bModifierDown);
         SetTipAndCursorForBoundary(boundary, bShiftDown, tip, pCursor);
      }
   }
   if (tip.empty()) {
      tip = XO("Click and drag to select audio");
   }
   if (HasEscape() && mUseSnap) {
      tip.Join(
/* i18n-hint: "Snapping" means automatic alignment of selection edges to any nearby label or clip boundaries */
        XO("(snapping)"), wxT(" ")
      );
   }
   return { tip, pCursor };
}

UIHandle::Result SelectHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   ProjectHistory::Get( *pProject ).ModifyState(false);
   mSnapManager.reset();
   if (mSelectionStateChanger) {
      mSelectionStateChanger->Commit();
      mSelectionStateChanger.reset();
   }
   
   if (mUseSnap && (mSnapStart.outCoord != -1 || mSnapEnd.outCoord != -1))
      return RefreshAll;
   else
      return RefreshNone;
}

UIHandle::Result SelectHandle::Cancel(AudacityProject *pProject)
{
   mSelectionStateChanger.reset();
   ViewInfo::Get( *pProject ).selectedRegion = mInitialSelection;

   return RefreshCode::RefreshAll;
}

void SelectHandle::Draw(
   TrackPanelDrawingContext &context,
   const wxRect &rect, unsigned iPass )
{
   if ( iPass == TrackArtist::PassSnapping ) {
      auto &dc = context.dc;
      // Draw snap guidelines if we have any
      if ( mSnapManager ) {
         auto coord1 = (mUseSnap || IsClicked()) ? mSnapStart.outCoord : -1;
         auto coord2 = (!mUseSnap || !IsClicked()) ? -1 : mSnapEnd.outCoord;
         mSnapManager->Draw( &dc, coord1, coord2 );
      }
   }
}

wxRect SelectHandle::DrawingArea(
   TrackPanelDrawingContext &,
   const wxRect &rect, const wxRect &panelRect, unsigned iPass )
{
   if ( iPass == TrackArtist::PassSnapping )
      return MaximizeHeight( rect, panelRect );
   else
      return rect;
}

std::weak_ptr<Track> SelectHandle::FindTrack()
{
   auto pView = mpView.lock();
   if (!pView)
      return {};
   else
      return pView->FindTrack();
}

void SelectHandle::Connect(AudacityProject *pProject)
{
   mTimerHandler = std::make_shared<TimerHandler>( this, pProject );
}

class SelectHandle::TimerHandler : public wxEvtHandler
{
public:
   TimerHandler( SelectHandle *pParent, AudacityProject *pProject )
      : mParent{ pParent }
      , mConnectedProject{ pProject }
   {
      if (mConnectedProject)
         mConnectedProject->Bind(EVT_TRACK_PANEL_TIMER,
            &SelectHandle::TimerHandler::OnTimer,
            this);
   }

   // Receives timer event notifications, to implement auto-scroll
   void OnTimer(wxCommandEvent &event);

private:
   SelectHandle *mParent;
   AudacityProject *mConnectedProject;
};

void SelectHandle::TimerHandler::OnTimer(wxCommandEvent &event)
{
   event.Skip();

   // AS: If the user is dragging the mouse and there is a track that
   //  has captured the mouse, then scroll the screen, as necessary.

   ///  We check on each timer tick to see if we need to scroll.

   // DM: If we're "autoscrolling" (which means that we're scrolling
   //  because the user dragged from inside to outside the window,
   //  not because the user clicked in the scroll bar), then
   //  the selection code needs to be handled slightly differently.
   //  We set this flag ("mAutoScrolling") to tell the selecting
   //  code that we didn't get here as a result of a mouse event,
   //  and therefore it should ignore the event,
   //  and instead use the last known mouse position.  Setting
   //  this flag also causes the Mac to redraw immediately rather
   //  than waiting for the next update event; this makes scrolling
   //  smoother on MacOS 9.

   const auto project = mConnectedProject;
   const auto &trackPanel = GetProjectPanel( *project );
   auto &window = ProjectWindow::Get( *project );
   if (mParent->mMostRecentX >= mParent->mRect.x + mParent->mRect.width) {
      mParent->mAutoScrolling = true;
      window.TP_ScrollRight();
   }
   else if (mParent->mMostRecentX < mParent->mRect.x) {
      mParent->mAutoScrolling = true;
      window.TP_ScrollLeft();
   }
   else {
      // Bug1387:  enable autoscroll during drag, if the pointer is at either
      // extreme x coordinate of the screen, even if that is still within the
      // track area.

      int xx = mParent->mMostRecentX, yy = 0;
      trackPanel.ClientToScreen(&xx, &yy);
      if (xx == 0) {
         mParent->mAutoScrolling = true;
         window.TP_ScrollLeft();
      }
      else {
         int width, height;
         ::wxDisplaySize(&width, &height);
         if (xx == width - 1) {
            mParent->mAutoScrolling = true;
            window.TP_ScrollRight();
         }
      }
   }

   auto pTrack = mParent->FindTrack().lock(); // TrackList::Lock() ?
   if (mParent->mAutoScrolling && pTrack) {
      // AS: To keep the selection working properly as we scroll,
      //  we fake a mouse event (remember, this method is called
      //  from a timer tick).

      // AS: For some reason, GCC won't let us pass this directly.
      wxMouseEvent evt(wxEVT_MOTION);
      const auto size = trackPanel.GetSize();
      mParent->Drag(
         TrackPanelMouseEvent{
            evt, mParent->mRect, size,
            TrackView::Get( *pTrack ).shared_from_this() },
         project
      );
      mParent->mAutoScrolling = false;
      GetProjectPanel( *mConnectedProject ).Refresh(false);
   }
}

/// Reset our selection markers.
void SelectHandle::StartSelection( AudacityProject *pProject )
{
   auto &viewInfo = ViewInfo::Get( *pProject );
   mSelStartValid = true;

   viewInfo.selectedRegion.setTimes(mSelStart, mSelStart);

   // PRL:  commented out the Sonify stuff with the TrackPanel refactor.
   // It was no-op anyway.
   //SonifyBeginModifyState();
   ProjectHistory::Get( *pProject ).ModifyState(false);
   //SonifyEndModifyState();
}

/// Extend or contract the existing selection
void SelectHandle::AdjustSelection
(AudacityProject *pProject,
 ViewInfo &viewInfo, int mouseXCoordinate, int trackLeftEdge,
 Track *track)
{
   if (!mSelStartValid)
      return;

   double selend =
      std::max(0.0, viewInfo.PositionToTime(mouseXCoordinate, trackLeftEdge));
   double origSelend = selend;

   auto pTrack = Track::SharedPointer( track );
   if (!pTrack)
      pTrack = TrackList::Get( *pProject ).Lock( FindTrack() );

   if (pTrack && mSnapManager.get()) {
      bool rightEdge = (selend > mSelStart);
      mSnapEnd = mSnapManager->Snap(pTrack.get(), selend, rightEdge);
      if (mSnapEnd.Snapped()) {
         if (mUseSnap)
            selend = mSnapEnd.outTime;
         if (mSnapEnd.snappedPoint)
            mSnapEnd.outCoord += trackLeftEdge;
      }
      if (!mSnapEnd.snappedPoint)
         mSnapEnd.outCoord = -1;

      // Check if selection endpoints are too close together to snap (unless
      // using snap-to-time -- then we always accept the snap results)
      if (mSnapStart.outCoord >= 0 &&
          mSnapEnd.outCoord >= 0 &&
          std::abs(mSnapStart.outCoord - mSnapEnd.outCoord) < 3) {
         if(!mSnapEnd.snappedTime)
            selend = origSelend;
         mSnapEnd.outCoord = -1;
      }
   }
   AssignSelection(viewInfo, selend, pTrack.get());
}

void SelectHandle::AssignSelection
(ViewInfo &viewInfo, double selend, Track *pTrack)
{
   double sel0, sel1;
   if (mSelStart < selend) {
      sel0 = mSelStart;
      sel1 = selend;
   }
   else {
      sel1 = mSelStart;
      sel0 = selend;
   }

   viewInfo.selectedRegion.setTimes(sel0, sel1);
}
