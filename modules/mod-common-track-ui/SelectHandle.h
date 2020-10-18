/**********************************************************************

Audacity: A Digital Audio Editor

SelectHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_SELECT_HANDLE__
#define __AUDACITY_SELECT_HANDLE__

#include "UIHandle.h"
#include "SelectedRegion.h"
#include "Snap.h"

#include <vector>

class SelectionStateChanger;
class SnapManager;
class Track;
class TrackView;
class TrackList;
class ViewInfo;
class WaveTrack;
class wxMouseState;

enum SelectionBoundary {
   SBNone,
   SBLeft, SBRight,
};

class COMMON_TRACK_UI_API SelectHandle : public UIHandle
{
   SelectHandle(const SelectHandle&);

public:
   explicit SelectHandle
      (const std::shared_ptr<TrackView> &pTrackView, bool useSnap,
       const TrackList &trackList,
       const TrackPanelMouseState &st, const ViewInfo &viewInfo);

   //! Type of a function to manufacture a SelectHandle or subclass appropriate for the view
   using SelectHandleFactory = std::function< UIHandlePtr(
      const std::shared_ptr<TrackView> &pTrackView,
      bool oldUseSnap,
      const TrackList &trackList,
      const TrackPanelMouseState &st,
      const ViewInfo &viewInfo
   ) >;

   // This always hits, but details of the hit vary with mouse position and
   // key state.
   static UIHandlePtr HitTest( SelectHandleFactory factory,
       std::weak_ptr<UIHandle> &holder,
       const TrackPanelMouseState &state, const AudacityProject *pProject,
       const std::shared_ptr<TrackView> &pTrackView);

   SelectHandle &operator=(const SelectHandle&) = default;
   
   virtual ~SelectHandle();

   bool IsClicked() const;

   void SetUseSnap(bool use, AudacityProject *pProject);
   void Enter(bool forward, AudacityProject *pProject) override;

   bool HasSnap() const;
   bool HasEscape() const override;

   bool Escape(AudacityProject *pProject) override;

   virtual int ChooseBoundary
      (const ViewInfo &viewInfo,
       wxCoord xx, wxCoord yy, const TrackView *pTrackView, const wxRect &rect,
       bool mayDragWidth, bool onlyWithinSnapDistance,
       double *pPinValue = NULL);

   Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) final;

   virtual void ModifiedClick(
      const TrackPanelMouseEvent &event, AudacityProject *pProject,
      bool bShiftDown, bool bCtrlDown);

   /*!
   @return true if starting a new selection
    */
   virtual bool UnmodifiedClick(
      const TrackPanelMouseEvent &event, AudacityProject *pProject);

   Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) final;

   //! Further overridable part of the drag operation
   virtual void DoDrag( AudacityProject &project,
      ViewInfo &viewInfo,
      TrackView &view, Track &clickedTrack, Track &track,
      wxCoord x, wxCoord y, bool controlDown);

   //! Called within Preview()
   virtual void SetTipAndCursorForBoundary(
      int boundary, bool,
      TranslatableString &tip, wxCursor *&pCursor);

   HitTestPreview Preview
      (const TrackPanelMouseState &state, AudacityProject *pProject)
      override;

   Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent) override;

   Result Cancel(AudacityProject*) override;

   static UIHandle::Result NeedChangeHighlight
      (const SelectHandle &oldState,
       const SelectHandle &newState);

protected:
   std::weak_ptr<Track> FindTrack();

   void Connect(AudacityProject *pProject);

   void StartSelection(AudacityProject *pProject);
   void AdjustSelection
      (AudacityProject *pProject,
       ViewInfo &viewInfo, int mouseXCoordinate, int trackLeftEdge,
       Track *pTrack);
   void AssignSelection(ViewInfo &viewInfo, double selend, Track *pTrack);

protected:

   // TrackPanelDrawable implementation
   void Draw(
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass ) override;

   wxRect DrawingArea(
      TrackPanelDrawingContext &,
      const wxRect &rect, const wxRect &panelRect, unsigned iPass ) override;


   const std::weak_ptr<TrackView> mpView;
   wxRect mRect{};
   SelectedRegion mInitialSelection{};

   std::shared_ptr<SnapManager> mSnapManager;
   SnapResults mSnapStart, mSnapEnd;
   bool mUseSnap{ true };

   bool mSelStartValid{};
   double mSelStart{ 0.0 };

   int mSelectionBoundary{ 0 };

   int mMostRecentX{ -1 }, mMostRecentY{ -1 };

   bool mAutoScrolling{};

   std::shared_ptr<SelectionStateChanger> mSelectionStateChanger;

   class TimerHandler;
   friend TimerHandler;
   std::shared_ptr<TimerHandler> mTimerHandler;
};

#endif
