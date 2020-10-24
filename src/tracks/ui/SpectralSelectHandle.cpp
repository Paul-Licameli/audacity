/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SpectralSelectHandle.cpp
 
 Paul Licameli split from SelectHandle.cpp
 
 **********************************************************************/

#include "SpectralSelectHandle.h"
#include "../../prefs/SpectrogramSettings.h"
#include "../../SpectrumAnalyst.h"
#include "../../ViewInfo.h"
#include "../../WaveTrack.h"
#include "../../HitTestResult.h"
#include "../../../images/Cursors.h"

namespace {
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
