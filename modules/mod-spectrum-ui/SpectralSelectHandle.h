/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SpectralSelectHandle.h
 
 Paul Licameli split from SelectHandle.h
 
 **********************************************************************/

#ifndef __AUDACITY__SPECTRAL_SELECT_HANDLE__
#define __AUDACITY__SPECTRAL_SELECT_HANDLE__

#include "SelectHandle.h" // to inherit

class SpectrumAnalyst;
class WaveTrack;

class SpectralSelectHandle final : public SelectHandle
{
public:
   // This is needed to implement a command assignable to keystrokes
   static void SnapCenterOnce
      (SpectrumAnalyst &analyst,
       ViewInfo &viewInfo, const WaveTrack *pTrack, bool up);

   using SelectHandle::SelectHandle;
   ~SpectralSelectHandle() override;

   int ChooseBoundary
      (const ViewInfo &viewInfo,
       wxCoord xx, wxCoord yy, const TrackView *pTrackView, const wxRect &rect,
       bool mayDragWidth, bool onlyWithinSnapDistance,
       double *pPinValue = NULL) override;

   void SetTipAndCursorForBoundary(
      int boundary, bool,
      TranslatableString &tip, wxCursor *&pCursor) override;

   HitTestPreview Preview
      (const TrackPanelMouseState &state, AudacityProject *pProject)
      override;

   void ModifiedClick(
      const TrackPanelMouseEvent &event, AudacityProject *pProject,
      bool bShiftDown, bool bCtrlDown) override;

   bool UnmodifiedClick(
      const TrackPanelMouseEvent &event, AudacityProject *pProject)
      override;

   void DoDrag( AudacityProject &project,
      ViewInfo &viewInfo,
      TrackView &view, Track &clickedTrack, Track &track,
      wxCoord x, wxCoord y, bool controlDown) override;

   UIHandle::Result Release(
      const TrackPanelMouseEvent &evt, AudacityProject *pProject,
      wxWindow *pWindow) override;

private:
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

   //void ResetFreqSelectionPin
   //   (const ViewInfo &viewInfo, double hintFrequency, bool logF);

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
};

#endif
