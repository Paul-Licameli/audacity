/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SpectralSelectHandle.cpp
 
 Paul Licameli split from SelectHandle.cpp
 
 **********************************************************************/

#include "SpectralSelectHandle.h"
#include "SpectrogramSettings.h"
#include "NumberScale.h"
#include "SpectrumAnalyst.h"
#include "tracks/ui/TrackView.h"
#include "ViewInfo.h"
#include "WaveTrack.h"
#include "HitTestResult.h"
#include "ProjectHistory.h"
#include "TrackPanelAx.h"
#include "TrackPanelMouseEvent.h"
#include "../images/Cursors.h"

class SpectrumView;

// #define SPECTRAL_EDITING_ESC_KEY

enum {
   // Seems 4 is too small to work at the top.  Why?
   FREQ_SNAP_DISTANCE = 10,
};

enum SpectralSelectionBoundary {
   SBBottom = 1 + SBRight,
   SBTop, SBCenter, SBWidth,
};

namespace {
   template<typename T>
   inline void SetIfNotNull(T * pValue, const T Value)
   {
      if (pValue == NULL)
         return;
      *pValue = Value;
   }

   /// Converts a frequency to screen y position.
   wxInt64 FrequencyToPosition(const WaveTrack *wt,
      double frequency,
      wxInt64 trackTopEdge,
      int trackHeight)
   {
      const auto &settings = SpectrogramSettings::Get(*wt);
      float minFreq, maxFreq;
      SpectrogramSettingsCache::Get(*wt).GetBounds(*wt, minFreq, maxFreq);
      const NumberScale numberScale(settings.GetScale(minFreq, maxFreq));
      const float p = numberScale.ValueToPosition(frequency);
      return trackTopEdge + wxInt64((1.0 - p) * trackHeight);
   }

   /// Converts a position (mouse Y coordinate) to
   /// frequency, in Hz.
   double PositionToFrequency(const WaveTrack *wt,
      bool maySnap,
      wxInt64 mouseYCoordinate,
      wxInt64 trackTopEdge,
      int trackHeight)
   {
      const double rate = wt->GetRate();

      // Handle snapping
      if (maySnap &&
         mouseYCoordinate - trackTopEdge < FREQ_SNAP_DISTANCE)
         return rate;
      if (maySnap &&
         trackTopEdge + trackHeight - mouseYCoordinate < FREQ_SNAP_DISTANCE)
         return -1;

      const auto &settings = SpectrogramSettings::Get(*wt);
      float minFreq, maxFreq;
      SpectrogramSettingsCache::Get(*wt).GetBounds(*wt, minFreq, maxFreq);
      const NumberScale numberScale(settings.GetScale(minFreq, maxFreq));
      const double p = double(mouseYCoordinate - trackTopEdge) / trackHeight;
      return numberScale.PositionToValue(1.0 - p);
   }

   // Is the distance between A and B less than D?
   template < class A, class B, class DIST > bool within(A a, B b, DIST d)
   {
      return (a > b - d) && (a < b + d);
   }

   inline double findMaxRatio(double center, double rate)
   {
      const double minFrequency = 1.0;
      const double maxFrequency = (rate / 2.0);
      const double frequency =
         std::min(maxFrequency,
            std::max(minFrequency, center));
      return
         std::min(frequency / minFrequency, maxFrequency / frequency);
   }

   wxCursor *EnvelopeCursor()
   {
      // This one doubles as the center frequency cursor for spectral selection:
      static auto envelopeCursor =
         ::MakeCursor(wxCURSOR_ARROW, EnvCursorXpm, 16, 16);
      return &*envelopeCursor;
   }

   // This returns true if we're a spectral editing track.
   inline bool isSpectralSelectionView(const TrackView *pTrackView) {
      return
        pTrackView->FindTrack() &&
        pTrackView->FindTrack()->TypeSwitch< bool >(
           [&](const WaveTrack *wt) {
              const auto &settings = SpectrogramSettings::Get(*wt);
              return settings.SpectralSelectionEnabled();
           });
   }
}

void SpectralSelectHandle::SnapCenterOnce
   (SpectrumAnalyst &analyst,
    ViewInfo &viewInfo, const WaveTrack *pTrack, bool up)
{
   const auto &settings = SpectrogramSettings::Get(*pTrack);
   const auto windowSize = settings.GetFFTLength();
   const double rate = pTrack->GetRate();
   const double nyq = rate / 2.0;
   const double binFrequency = rate / windowSize;

   double f1 = viewInfo.selectedRegion.f1();
   double centerFrequency = viewInfo.selectedRegion.fc();
   if (centerFrequency <= 0) {
      centerFrequency = up ? binFrequency : nyq;
      f1 = centerFrequency * sqrt(2.0);
   }

   double ratio = f1 / centerFrequency;
   const int originalBin = floor(0.5 + centerFrequency / binFrequency);
   const int limitingBin = up ? floor(0.5 + nyq / binFrequency) : 1;

   // This is crude and wasteful, doing the FFT each time the command is called.
   // It would be better to cache the data, but then invalidation of the cache would
   // need doing in all places that change the time selection.
   StartSnappingFreqSelection(analyst, viewInfo, pTrack);
   double snappedFrequency = centerFrequency;
   int bin = originalBin;
   if (up) {
      while (snappedFrequency <= centerFrequency &&
         bin < limitingBin)
         snappedFrequency = analyst.FindPeak(++bin * binFrequency, NULL);
   }
   else {
      while (snappedFrequency >= centerFrequency &&
         bin > limitingBin)
         snappedFrequency = analyst.FindPeak(--bin * binFrequency, NULL);
   }

   // PRL:  added these two lines with the big TrackPanel refactor
   const double maxRatio = findMaxRatio(snappedFrequency, rate);
   ratio = std::min(ratio, maxRatio);

   viewInfo.selectedRegion.setFrequencies
      (snappedFrequency / ratio, snappedFrequency * ratio);
}

SpectralSelectHandle::~SpectralSelectHandle() = default;

int SpectralSelectHandle::ChooseBoundary
   (const ViewInfo &viewInfo,
    wxCoord xx, wxCoord yy, const TrackView *pTrackView, const wxRect &rect,
    bool mayDragWidth, bool onlyWithinSnapDistance,
    double *pPinValue)
{
   // Choose one of four boundaries to adjust, or the center frequency.
   // May choose frequencies only if in a spectrogram view and
   // within the time boundaries.
   // May choose no boundary if onlyWithinSnapDistance is true.
   // Otherwise choose the eligible boundary nearest the mouse click.
   const double selend = viewInfo.PositionToTime(xx, rect.x);
   const double t0 = viewInfo.selectedRegion.t0();
   const double t1 = viewInfo.selectedRegion.t1();

   const wxInt64 posS = viewInfo.TimeToPosition(selend);
   const wxInt64 pos0 = viewInfo.TimeToPosition(t0);
   wxInt64 pixelDist = std::abs(posS - pos0);

   const double f0 = viewInfo.selectedRegion.f0();
   const double f1 = viewInfo.selectedRegion.f1();
   const double fc = viewInfo.selectedRegion.fc();
   double ratio = 0;

   bool chooseTime = true;
   bool chooseBottom = true;
   bool chooseCenter = false;
   // Consider adjustment of frequencies only if mouse is
   // within the time boundaries
   if (!viewInfo.selectedRegion.isPoint() &&
      t0 <= selend && selend < t1 &&
      isSpectralSelectionView(pTrackView)) {
      // Spectral selection track is always wave
      auto pTrack = pTrackView->FindTrack();
      const WaveTrack *const wt =
        static_cast<const WaveTrack*>(pTrack.get());
      const wxInt64 bottomSel = (f0 >= 0)
         ? FrequencyToPosition(wt, f0, rect.y, rect.height)
         : rect.y + rect.height;
      const wxInt64 topSel = (f1 >= 0)
         ? FrequencyToPosition(wt, f1, rect.y, rect.height)
         : rect.y;
      wxInt64 signedBottomDist = (int)(yy - bottomSel);
      wxInt64 verticalDist = std::abs(signedBottomDist);
      if (bottomSel == topSel)
         // Top and bottom are too close to resolve on screen
         chooseBottom = (signedBottomDist >= 0);
      else {
         const wxInt64 topDist = std::abs((int)(yy - topSel));
         if (topDist < verticalDist)
            chooseBottom = false, verticalDist = topDist;
      }
      if (fc > 0
#ifdef SPECTRAL_EDITING_ESC_KEY
         && mayDragWidth
#endif
         ) {
         const wxInt64 centerSel =
            FrequencyToPosition(wt, fc, rect.y, rect.height);
         const wxInt64 centerDist = abs((int)(yy - centerSel));
         if (centerDist < verticalDist)
            chooseCenter = true, verticalDist = centerDist,
            ratio = f1 / fc;
      }
      if (verticalDist >= 0 &&
         verticalDist < pixelDist) {
         pixelDist = verticalDist;
         chooseTime = false;
      }
   }

   if (!chooseTime) {
      // PRL:  Seems I need a larger tolerance to make snapping work
      // at top of track, not sure why
      if (onlyWithinSnapDistance &&
         pixelDist >= FREQ_SNAP_DISTANCE) {
         SetIfNotNull(pPinValue, -1.0);
         return SBNone;
      }
      else if (chooseCenter) {
         SetIfNotNull(pPinValue, ratio);
         return SBCenter;
      }
      else if (mayDragWidth && fc > 0) {
         SetIfNotNull(pPinValue, fc);
         return SBWidth;
      }
      else if (chooseBottom) {
         SetIfNotNull(pPinValue, f1);
         return SBBottom;
      }
      else {
         SetIfNotNull(pPinValue, f0);
         return SBTop;
      }
   }

   return SelectHandle::ChooseBoundary(
      viewInfo, xx, yy, pTrackView, rect,
      mayDragWidth, onlyWithinSnapDistance, pPinValue);
}

void SpectralSelectHandle::SetTipAndCursorForBoundary(
   int boundary, bool shiftDown,
   TranslatableString &tip, wxCursor *&pCursor)
{
   bool frequencySnapping =
      !shiftDown || mFreqSelMode == FREQ_SEL_SNAPPING_CENTER;
   static auto bottomFrequencyCursor =
      ::MakeCursor(wxCURSOR_ARROW, BottomFrequencyCursorXpm, 16, 16);
   static auto topFrequencyCursor =
      ::MakeCursor(wxCURSOR_ARROW, TopFrequencyCursorXpm, 16, 16);
   static auto bandWidthCursor =
      ::MakeCursor(wxCURSOR_ARROW, BandWidthCursorXpm, 16, 16);

   switch (boundary) {
   case SBBottom:
      tip = XO("Click and drag to move bottom selection frequency.");
      pCursor = &*bottomFrequencyCursor;
      break;
   case SBTop:
      tip = XO("Click and drag to move top selection frequency.");
      pCursor = &*topFrequencyCursor;
      break;
   case SBCenter:
   {
#ifndef SPECTRAL_EDITING_ESC_KEY
      tip =
         frequencySnapping ?
         XO("Click and drag to move center selection frequency to a spectral peak.") :
         XO("Click and drag to move center selection frequency.");

#else
      shiftDown;

      tip =
         XO("Click and drag to move center selection frequency.");

#endif

      pCursor = EnvelopeCursor();
   }
   break;
   case SBWidth:
      tip = XO("Click and drag to adjust frequency bandwidth.");
      pCursor = &*bandWidthCursor;
      break;
   default:
      SelectHandle::SetTipAndCursorForBoundary(
            boundary, shiftDown, tip, pCursor);
   }
}

HitTestPreview SpectralSelectHandle::Preview
(const TrackPanelMouseState &st, AudacityProject *pProject)
{
#if 0
   if (!IsClicked()) {
      // This is a vestige of an idea in the prototype version.
      // Center would snap without mouse button down, click would pin the center
      // and drag width.
      if ((mFreqSelMode == FREQ_SEL_SNAPPING_CENTER) &&
         isSpectralSelectionView(pView)) {
         // Not shift-down, but center frequency snapping toggle is on
         tip = XO("Click and drag to set frequency bandwidth.");
         pCursor = &*envelopeCursor;
         return {};
      }
   }
#endif

   return SelectHandle::Preview(st, pProject);
}

void SpectralSelectHandle::ModifiedClick(
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
      case SBBottom:
      case SBTop:
      {
         mFreqSelTrack = pTrack->SharedPointer<const WaveTrack>();
         mFreqSelPin = value;
         mFreqSelMode =
            (boundary == SBBottom)
            ? FREQ_SEL_BOTTOM_FREE : FREQ_SEL_TOP_FREE;

         // Drag frequency only, not time:
         mSelStartValid = false;
         AdjustFreqSelection(
            static_cast<WaveTrack*>(pTrack),
            viewInfo, event.m_y, mRect.y, mRect.height);
         break;
      }
      case SBCenter:
      {
         const auto wt = static_cast<const WaveTrack*>(pTrack);
         HandleCenterFrequencyClick(viewInfo, true, wt, value);
         break;
      }
      case SBLeft:
      case SBRight:
      {
         // If drag starts, change time selection only
         // (also exit frequency snapping)
         mFreqSelMode = FREQ_SEL_INVALID;
         /* fallthrough */
      }
      default:
         return SelectHandle::ModifiedClick(
            evt, pProject, bShiftDown, bCtrlDown);
   }
}

bool SpectralSelectHandle::UnmodifiedClick(
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
         if (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
            isSpectralSelectionView(pView.get())) {
            // This code is no longer reachable, but it had a place in the
            // spectral selection prototype.  It used to be that you could be
            // in a center-frequency-snapping mode that was not a mouse drag
            // but responded to mouse movements.  Click exited that and dragged
            // width instead.  PRL.

            // Ignore whether we are inside the time selection.
            // Exit center-snapping, start dragging the width.
            mFreqSelMode = FREQ_SEL_PINNED_CENTER;
            mFreqSelTrack = pTrack->SharedPointer<const WaveTrack>();
            mFreqSelPin = viewInfo.selectedRegion.fc();
            // Do not adjust time boundaries
            mSelStartValid = false;
            AdjustFreqSelection(
               static_cast<WaveTrack*>(pTrack),
               viewInfo, event.m_y, mRect.y, mRect.height);
            // For persistence of the selection change:
            ProjectHistory::Get( *pProject ).ModifyState(false);
            mSelectionBoundary = SBWidth;
            //return RefreshNone;
            return false;
         }
         else
         {
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
            case SBBottom:
            case SBTop:
            case SBWidth:
               startNewSelection = false;
               // Disable time selection
               mSelStartValid = false;
               mFreqSelTrack = pTrack->SharedPointer<const WaveTrack>();
               mFreqSelPin = value;
               mFreqSelMode =
                  (boundary == SBWidth) ? FREQ_SEL_PINNED_CENTER :
                  (boundary == SBBottom) ? FREQ_SEL_BOTTOM_FREE :
                  FREQ_SEL_TOP_FREE;
               break;
            case SBCenter:
            {
               const auto wt = static_cast<const WaveTrack*>(pTrack);
               HandleCenterFrequencyClick(viewInfo, false, wt, value);
               startNewSelection = false;
               break;
            }
            case SBLeft:
            case SBRight:
               // Disable frequency selection
               mFreqSelMode = FREQ_SEL_INVALID;
               /* fallthrough */
            default:
               return SelectHandle::UnmodifiedClick(evt, pProject);
            }
         }
      } // bAdjustSelectionEdges
   }
   
   if (startNewSelection) {
      StartFreqSelection (viewInfo, event.m_y, mRect.y, mRect.height,
         pView.get());
      StartSelection(pProject);
   }

   return startNewSelection;
}

void SpectralSelectHandle::DoDrag( AudacityProject &project,
   ViewInfo &viewInfo, TrackView &view, Track &clickedTrack, Track &track,
   wxCoord x, wxCoord y, bool controlDown)
{
   SelectHandle::DoDrag(
      project, viewInfo, view, clickedTrack, track, x, y, controlDown);
#ifndef SPECTRAL_EDITING_ESC_KEY
   if (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
       !viewInfo.selectedRegion.isPoint())
      MoveSnappingFreqSelection
      (&project, viewInfo, y, mRect.y, mRect.height, &view);
   else
#endif
   if ( TrackList::Get( project ).Lock(mFreqSelTrack).get() == &track )
      AdjustFreqSelection(
         static_cast<const WaveTrack*>(&track),
         viewInfo, y, mRect.y, mRect.height);
}

UIHandle::Result SpectralSelectHandle::Release(
   const TrackPanelMouseEvent &evt, AudacityProject *pProject,
   wxWindow *pWindow)
{
   mFrequencySnapper.reset();
   return SelectHandle::Release(evt, pProject, pWindow);
}

void SpectralSelectHandle::StartFreqSelection(ViewInfo &viewInfo,
   int mouseYCoordinate, int trackTopEdge,
   int trackHeight, TrackView *pTrackView)
{
   mFreqSelTrack.reset();
   mFreqSelMode = FREQ_SEL_INVALID;
   mFreqSelPin = SelectedRegion::UndefinedFrequency;

   if (isSpectralSelectionView(pTrackView)) {
      // Spectral selection track is always wave
      auto shTrack = pTrackView->FindTrack()->SharedPointer<const WaveTrack>();
      mFreqSelTrack = shTrack;
      mFreqSelMode = FREQ_SEL_FREE;
      mFreqSelPin =
         PositionToFrequency(shTrack.get(), false, mouseYCoordinate,
         trackTopEdge, trackHeight);
      viewInfo.selectedRegion.setFrequencies(mFreqSelPin, mFreqSelPin);
   }
}

void SpectralSelectHandle::AdjustFreqSelection(
   const WaveTrack *wt, ViewInfo &viewInfo,
   int mouseYCoordinate, int trackTopEdge,
   int trackHeight)
{
   if (mFreqSelMode == FREQ_SEL_INVALID ||
       mFreqSelMode == FREQ_SEL_SNAPPING_CENTER)
      return;

   // Extension happens only when dragging in the same track in which we
   // started, and that is of a spectrogram display type.

   const double rate =  wt->GetRate();
   const double frequency =
      PositionToFrequency(wt, true, mouseYCoordinate,
         trackTopEdge, trackHeight);

   // Dragging center?
   if (mFreqSelMode == FREQ_SEL_DRAG_CENTER) {
      if (frequency == rate || frequency < 1.0)
         // snapped to top or bottom
         viewInfo.selectedRegion.setFrequencies(
            SelectedRegion::UndefinedFrequency,
            SelectedRegion::UndefinedFrequency);
      else {
         // mFreqSelPin holds the ratio of top to center
         const double maxRatio = findMaxRatio(frequency, rate);
         const double ratio = std::min(maxRatio, mFreqSelPin);
         viewInfo.selectedRegion.setFrequencies(
            frequency / ratio, frequency * ratio);
      }
   }
   else if (mFreqSelMode == FREQ_SEL_PINNED_CENTER) {
      if (mFreqSelPin >= 0) {
         // Change both upper and lower edges leaving centre where it is.
         if (frequency == rate || frequency < 1.0)
            // snapped to top or bottom
            viewInfo.selectedRegion.setFrequencies(
               SelectedRegion::UndefinedFrequency,
               SelectedRegion::UndefinedFrequency);
         else {
            // Given center and mouse position, find ratio of the larger to the
            // smaller, limit that to the frequency scale bounds, and adjust
            // top and bottom accordingly.
            const double maxRatio = findMaxRatio(mFreqSelPin, rate);
            double ratio = frequency / mFreqSelPin;
            if (ratio < 1.0)
               ratio = 1.0 / ratio;
            ratio = std::min(maxRatio, ratio);
            viewInfo.selectedRegion.setFrequencies(
               mFreqSelPin / ratio, mFreqSelPin * ratio);
         }
      }
   }
   else {
      // Dragging of upper or lower.
      const bool bottomDefined =
         !(mFreqSelMode == FREQ_SEL_TOP_FREE && mFreqSelPin < 0);
      const bool topDefined =
         !(mFreqSelMode == FREQ_SEL_BOTTOM_FREE && mFreqSelPin < 0);
      if (!bottomDefined || (topDefined && mFreqSelPin < frequency)) {
         // Adjust top
         if (frequency == rate)
            // snapped high; upper frequency is undefined
            viewInfo.selectedRegion.setF1(SelectedRegion::UndefinedFrequency);
         else
            viewInfo.selectedRegion.setF1(std::max(1.0, frequency));

         viewInfo.selectedRegion.setF0(mFreqSelPin);
      }
      else {
         // Adjust bottom
         if (frequency < 1.0)
            // snapped low; lower frequency is undefined
            viewInfo.selectedRegion.setF0(SelectedRegion::UndefinedFrequency);
         else
            viewInfo.selectedRegion.setF0(std::min(rate / 2.0, frequency));

         viewInfo.selectedRegion.setF1(mFreqSelPin);
      }
   }
}

void SpectralSelectHandle::HandleCenterFrequencyClick
(const ViewInfo &viewInfo, bool shiftDown, const WaveTrack *pTrack, double value)
{
   if (shiftDown) {
      // Disable time selection
      mSelStartValid = false;
      mFreqSelTrack = pTrack->SharedPointer<const WaveTrack>();
      mFreqSelPin = value;
      mFreqSelMode = FREQ_SEL_DRAG_CENTER;
   }
   else {
#ifndef SPECTRAL_EDITING_ESC_KEY
      // Start center snapping
      // Turn center snapping on (the only way to do this)
      mFreqSelMode = FREQ_SEL_SNAPPING_CENTER;
      // Disable time selection
      mSelStartValid = false;
      mFrequencySnapper = std::make_shared<SpectrumAnalyst>();
      StartSnappingFreqSelection(*mFrequencySnapper, viewInfo, pTrack);
#endif
   }
}

void SpectralSelectHandle::StartSnappingFreqSelection
   (SpectrumAnalyst &analyst,
    const ViewInfo &viewInfo, const WaveTrack *pTrack)
{
   static const size_t minLength = 8;

   const double rate = pTrack->GetRate();

   // Grab samples, just for this track, at these times
   std::vector<float> frequencySnappingData;
   const auto start =
      pTrack->TimeToLongSamples(viewInfo.selectedRegion.t0());
   const auto end =
      pTrack->TimeToLongSamples(viewInfo.selectedRegion.t1());
   const auto length =
      std::min(frequencySnappingData.max_size(),
         limitSampleBufferSize(10485760, // as in FreqWindow.cpp
            end - start));
   const auto effectiveLength = std::max(minLength, length);
   frequencySnappingData.resize(effectiveLength, 0.0f);
   pTrack->GetFloats(
      &frequencySnappingData[0],
      start, length, fillZero,
      // Don't try to cope with exceptions, just read zeroes instead.
      false);

   // Use same settings as are now used for spectrogram display,
   // except, shrink the window as needed so we get some answers

   const auto &settings = SpectrogramSettings::Get(*pTrack);
   auto windowSize = settings.GetFFTLength();

   while(windowSize > effectiveLength)
      windowSize >>= 1;
   const int windowType = settings.windowType;

   analyst.Calculate(
      SpectrumAnalyst::Spectrum, windowType, windowSize, rate,
      &frequencySnappingData[0], length);

   // We can now throw away the sample data but we keep the spectrum.
}

void SpectralSelectHandle::MoveSnappingFreqSelection
   (AudacityProject *pProject, ViewInfo &viewInfo, int mouseYCoordinate,
    int trackTopEdge,
    int trackHeight, TrackView *pTrackView)
{
   auto pTrack = pTrackView->FindTrack().get();
   if (pTrack &&
      pTrack->GetSelected() &&
      isSpectralSelectionView(pTrackView)) {
      // Spectral selection track is always wave
      WaveTrack *const wt = static_cast<WaveTrack*>(pTrack);
      // PRL:
      // What would happen if center snapping selection began in one spectrogram track,
      // then continues inside another?  We do not then recalculate
      // the spectrum (as was done in StartSnappingFreqSelection)
      // but snap according to the peaks in the old track.

      // But if we always supply the original clicked track here that doesn't matter.
      const double rate = wt->GetRate();
      const double frequency =
         PositionToFrequency(wt, false, mouseYCoordinate,
         trackTopEdge, trackHeight);
      const double snappedFrequency =
         mFrequencySnapper->FindPeak(frequency, NULL);
      const double maxRatio = findMaxRatio(snappedFrequency, rate);
      double ratio = 2.0; // An arbitrary octave on each side, at most
      {
         const double f0 = viewInfo.selectedRegion.f0();
         const double f1 = viewInfo.selectedRegion.f1();
         if (f1 >= f0 && f0 >= 0)
            // Preserve already chosen ratio instead
            ratio = sqrt(f1 / f0);
      }
      ratio = std::min(ratio, maxRatio);

      mFreqSelPin = snappedFrequency;
      viewInfo.selectedRegion.setFrequencies(
         snappedFrequency / ratio, snappedFrequency * ratio);

      // A change here would affect what AdjustFreqSelection() does
      // in the prototype version where you switch from moving center to
      // dragging width with a click.  No effect now.
      mFreqSelTrack = wt->SharedPointer<const WaveTrack>();

      // SelectNone();
      // SelectTrack(pTrack, true);
      TrackFocus::Get( *pProject ).Set(pTrack);
   }
}

#if 0
// unused
void SpectralSelectHandle::ResetFreqSelectionPin
   (const ViewInfo &viewInfo, double hintFrequency, bool logF)
{
   switch (mFreqSelMode) {
   case FREQ_SEL_INVALID:
   case FREQ_SEL_SNAPPING_CENTER:
      mFreqSelPin = -1.0;
      break;

   case FREQ_SEL_PINNED_CENTER:
      mFreqSelPin = viewInfo.selectedRegion.fc();
      break;

   case FREQ_SEL_DRAG_CENTER:
   {
      // Re-pin the width
      const double f0 = viewInfo.selectedRegion.f0();
      const double f1 = viewInfo.selectedRegion.f1();
      if (f0 >= 0 && f1 >= 0)
         mFreqSelPin = sqrt(f1 / f0);
      else
         mFreqSelPin = -1.0;
   }
   break;

   case FREQ_SEL_FREE:
      // Pin which?  Farther from the hint which is the presumed
      // mouse position.
   {
      // If this function finds use again, the following should be
      // generalized using NumberScale

      const double f0 = viewInfo.selectedRegion.f0();
      const double f1 = viewInfo.selectedRegion.f1();
      if (logF) {
         if (f1 < 0)
            mFreqSelPin = f0;
         else {
            const double logf1 = log(std::max(1.0, f1));
            const double logf0 = log(std::max(1.0, f0));
            const double logHint = log(std::max(1.0, hintFrequency));
            if (std::abs(logHint - logf1) < std::abs(logHint - logf0))
               mFreqSelPin = f0;
            else
               mFreqSelPin = f1;
         }
      }
      else {
         if (f1 < 0 ||
            std::abs(hintFrequency - f1) < std::abs(hintFrequency - f0))
            mFreqSelPin = f0;
         else
            mFreqSelPin = f1;
      }
   }
   break;

   case FREQ_SEL_TOP_FREE:
      mFreqSelPin = viewInfo.selectedRegion.f0();
      break;

   case FREQ_SEL_BOTTOM_FREE:
      mFreqSelPin = viewInfo.selectedRegion.f1();
      break;

   default:
      wxASSERT(false);
   }
}

#endif
