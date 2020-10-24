/**********************************************************************

Audacity: A Digital Audio Editor

SelectHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_SELECT_HANDLE__
#define __AUDACITY_SELECT_HANDLE__

#include "../../UIHandle.h"
#include "../../SelectedRegion.h"
#include "../../Snap.h"

#include <vector>

class SelectionStateChanger;
class SnapManager;
class SpectrumAnalyst;
class Track;
class TrackView;
class TrackList;
class ViewInfo;
class WaveTrack;
class wxMouseState;

enum SelectionBoundary {
   SBNone,
   SBLeft, SBRight,
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   SBBottom, SBTop, SBCenter, SBWidth,
#endif
};

class AUDACITY_DLL_API SelectHandle : public UIHandle
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

   Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) override;

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

private:
   std::weak_ptr<Track> FindTrack();

   void Connect(AudacityProject *pProject);

   void StartSelection(AudacityProject *pProject);
   void AdjustSelection
      (AudacityProject *pProject,
       ViewInfo &viewInfo, int mouseXCoordinate, int trackLeftEdge,
       Track *pTrack);
   void AssignSelection(ViewInfo &viewInfo, double selend, Track *pTrack);

protected:
   void StartFreqSelection
      (ViewInfo &viewInfo, int mouseYCoordinate, int trackTopEdge,
      int trackHeight, TrackView *pTrackView);
   void AdjustFreqSelection
      (const WaveTrack *wt,
       ViewInfo &viewInfo, int mouseYCoordinate, int trackTopEdge,
       int trackHeight);

   void HandleCenterFrequencyClick
      (const ViewInfo &viewInfo, bool shiftDown,
       const WaveTrack *pTrack, double value);
   static void StartSnappingFreqSelection
      (SpectrumAnalyst &analyst,
       const ViewInfo &viewInfo, const WaveTrack *pTrack);
   void MoveSnappingFreqSelection
      (AudacityProject *pProject, ViewInfo &viewInfo, int mouseYCoordinate,
       int trackTopEdge,
       int trackHeight, TrackView *pTrackView);

   // TrackPanelDrawable implementation
   void Draw(
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass ) override;

   wxRect DrawingArea(
      TrackPanelDrawingContext &,
      const wxRect &rect, const wxRect &panelRect, unsigned iPass ) override;

   //void ResetFreqSelectionPin
   //   (const ViewInfo &viewInfo, double hintFrequency, bool logF);


   std::weak_ptr<TrackView> mpView;
   wxRect mRect{};
   SelectedRegion mInitialSelection{};

   std::shared_ptr<SnapManager> mSnapManager;
   SnapResults mSnapStart, mSnapEnd;
   bool mUseSnap{ true };

   bool mSelStartValid{};
   double mSelStart{ 0.0 };

   int mSelectionBoundary{ 0 };

   enum eFreqSelMode {
      FREQ_SEL_INVALID,

      FREQ_SEL_SNAPPING_CENTER,
      FREQ_SEL_PINNED_CENTER,
      FREQ_SEL_DRAG_CENTER,

      FREQ_SEL_FREE,
      FREQ_SEL_TOP_FREE,
      FREQ_SEL_BOTTOM_FREE,
   }  mFreqSelMode{ FREQ_SEL_INVALID };
   std::weak_ptr<const WaveTrack> mFreqSelTrack;
   // Following holds:
   // the center for FREQ_SEL_PINNED_CENTER,
   // the ratio of top to center (== center to bottom) for FREQ_SEL_DRAG_CENTER,
   // a frequency boundary for FREQ_SEL_FREE, FREQ_SEL_TOP_FREE, or
   // FREQ_SEL_BOTTOM_FREE,
   // and is ignored otherwise.
   double mFreqSelPin{ -1.0 };
   std::shared_ptr<SpectrumAnalyst> mFrequencySnapper;

   int mMostRecentX{ -1 }, mMostRecentY{ -1 };

   bool mAutoScrolling{};

   std::shared_ptr<SelectionStateChanger> mSelectionStateChanger;

   class TimerHandler;
   friend TimerHandler;
   std::shared_ptr<TimerHandler> mTimerHandler;
};

#endif
