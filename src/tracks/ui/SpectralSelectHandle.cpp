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
