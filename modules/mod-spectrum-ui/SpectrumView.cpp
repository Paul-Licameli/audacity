/**********************************************************************

Audacity: A Digital Audio Editor

SpectrumView.cpp

Paul Licameli split from WaveTrackView.cpp

**********************************************************************/

#include "SpectrumView.h"

#include "Sequence.h"
#include "Spectrum.h"

#include "SpectrumVRulerControls.h"
#include "tracks/playabletrack/wavetrack/ui/WaveTrackView.h"
#include "tracks/playabletrack/wavetrack/ui/WaveTrackViewConstants.h"
#include "AColor.h"
#include "Prefs.h"
#include "NumberScale.h"
#include "TrackArtist.h"
#include "TrackPanelDrawingContext.h"
#include "ViewInfo.h"
#include "WaveClip.h"
#include "WaveTrack.h"
#include "SpectrogramSettings.h"
#include "SpectralSelectHandle.h"

#include <wx/dcmemory.h>
#include <wx/graphics.h>

static const Identifier SpectrogramId = wxT("Spectrogram");

static WaveTrackSubView::Type sType{
   { SpectrogramId, XXO("&Spectrogram") }
};

static WaveTrackSubViewType::Registration reg{ sType };

SpectrumView::~SpectrumView() = default;

std::vector<UIHandlePtr> SpectrumView::DetailedHitTest(
   const TrackPanelMouseState &state,
   const AudacityProject *pProject, int currentTool, bool bMultiTool )
{
   const auto wt = std::static_pointer_cast< WaveTrack >( FindTrack() );

   return WaveTrackSubView::DoDetailedHitTest(
      state, pProject, currentTool, bMultiTool, wt
   ).second;
}

UIHandlePtr SpectrumView::SelectionHitTest(
    std::weak_ptr<UIHandle> &mSelectHandle,
    const TrackPanelMouseState &state, const AudacityProject *pProject)
{
   auto factory = [](
      const std::shared_ptr<TrackView> &pTrackView,
      bool oldUseSnap,
      const TrackList &trackList,
      const TrackPanelMouseState &st,
      const ViewInfo &viewInfo) -> UIHandlePtr {
         return std::make_shared<SpectralSelectHandle>(
            pTrackView, oldUseSnap, trackList, st, viewInfo );
   };
   return SelectHandle::HitTest( factory,
      mSelectHandle, state, pProject, shared_from_this() );
}

void SpectrumView::DoSetMinimized( bool minimized )
{
   auto wt = static_cast<WaveTrack*>( FindTrack().get() );

#ifdef EXPERIMENTAL_HALF_WAVE
   bool bHalfWave;
   gPrefs->Read(wxT("/GUI/CollapseToHalfWave"), &bHalfWave, false);
   if( bHalfWave && minimized)
   {
      // It is all right to set the top of scale to a huge number,
      // not knowing the track rate here -- because when retrieving the
      // value, then we pass in a sample rate and clamp it above to the
      // Nyquist frequency.
      constexpr auto max = std::numeric_limits<float>::max();
      const bool spectrumLinear =
         (SpectrogramSettings::Get(*wt).scaleType ==
            SpectrogramSettings::stLinear);
      // Zoom out full
      SpectrogramSettingsCache::Get(*wt)
         .SetBounds( spectrumLinear ? 0.0f : 1.0f, max );
   }
#endif

   TrackView::DoSetMinimized( minimized );
}

auto SpectrumView::SubViewType() const -> const Type &
{
   return sType;
}

std::shared_ptr<TrackVRulerControls> SpectrumView::DoGetVRulerControls()
{
   return std::make_shared<SpectrumVRulerControls>( shared_from_this() );
}

namespace
{

static inline float findValue
(const float *spectrum, float bin0, float bin1, unsigned nBins,
 bool autocorrelation, int gain, int range)
{
   float value;


#if 0
   // Averaging method
   if ((int)(bin1) == (int)(bin0)) {
      value = spectrum[(int)(bin0)];
   } else {
      float binwidth= bin1 - bin0;
      value = spectrum[(int)(bin0)] * (1.f - bin0 + (int)bin0);

      bin0 = 1 + (int)(bin0);
      while (bin0 < (int)(bin1)) {
         value += spectrum[(int)(bin0)];
         bin0 += 1.0;
      }
      // Do not reference past end of freq array.
      if ((int)(bin1) >= (int)nBins) {
         bin1 -= 1.0;
      }

      value += spectrum[(int)(bin1)] * (bin1 - (int)(bin1));
      value /= binwidth;
   }
#else
   // Maximum method, and no apportionment of any single bins over multiple pixel rows
   // See Bug971
   int index, limitIndex;
   if (autocorrelation) {
      // bin = 2 * nBins / (nBins - 1 - array_index);
      // Solve for index
      index = std::max(0.0f, std::min(float(nBins - 1),
         (nBins - 1) - (2 * nBins) / (std::max(1.0f, bin0))
      ));
      limitIndex = std::max(0.0f, std::min(float(nBins - 1),
         (nBins - 1) - (2 * nBins) / (std::max(1.0f, bin1))
      ));
   }
   else {
      index = std::min<int>(nBins - 1, (int)(floor(0.5 + bin0)));
      limitIndex = std::min<int>(nBins, (int)(floor(0.5 + bin1)));
   }
   value = spectrum[index];
   while (++index < limitIndex)
      value = std::max(value, spectrum[index]);
#endif
   if (!autocorrelation) {
      // Last step converts dB to a 0.0-1.0 range
      value = (value + range + gain) / (double)range;
   }
   value = std::min(1.0f, std::max(0.0f, value));
   return value;
}

// dashCount counts both dashes and the spaces between them.
inline AColor::ColorGradientChoice
ChooseColorSet( float bin0, float bin1, float selBinLo,
   float selBinCenter, float selBinHi, int dashCount, bool isSpectral )
{
   if (!isSpectral)
      return  AColor::ColorGradientTimeSelected;
   if ((selBinCenter >= 0) && (bin0 <= selBinCenter) &&
       (selBinCenter < bin1))
      return AColor::ColorGradientEdge;
   if ((0 == dashCount % 2) &&
       (((selBinLo >= 0) && (bin0 <= selBinLo) && ( selBinLo < bin1))  ||
        ((selBinHi >= 0) && (bin0 <= selBinHi) && ( selBinHi < bin1))))
      return AColor::ColorGradientEdge;
   if ((selBinLo < 0 || selBinLo < bin1) && (selBinHi < 0 || selBinHi > bin0))
      return  AColor::ColorGradientTimeAndFrequencySelected;
   
   return  AColor::ColorGradientTimeSelected;
}

class SpecCache {
public:

   // Make invalid cache
   SpecCache()
      : algorithm(-1)
      , pps(-1.0)
      , start(-1.0)
      , windowType(-1)
      , frequencyGain(-1)
      , dirty(-1)
   {
   }

   ~SpecCache()
   {
   }

   bool Matches(int dirty_, double pixelsPerSecond,
      const SpectrogramSettings &settings, double rate) const;

   // Calculate one column of the spectrum
   bool CalculateOneSpectrum
      (const SpectrogramSettings &settings,
       WaveTrackCache &waveTrackCache,
       const int xx, sampleCount numSamples,
       double offset, double rate, double pixelsPerSecond,
       int lowerBoundX, int upperBoundX,
       const std::vector<float> &gainFactors,
       float* __restrict scratch,
       float* __restrict out) const;

   // Grow the cache while preserving the (possibly now invalid!) contents
   void Grow(size_t len_, const SpectrogramSettings& settings,
               double pixelsPerSecond, double start_);

   // Calculate the dirty columns at the begin and end of the cache
   void Populate
      (const SpectrogramSettings &settings, WaveTrackCache &waveTrackCache,
       int copyBegin, int copyEnd, size_t numPixels,
       sampleCount numSamples,
       double offset, double rate, double pixelsPerSecond);

   size_t       len { 0 }; // counts pixels, not samples
   int          algorithm;
   double       pps;
   double       start;
   int          windowType;
   size_t       windowSize { 0 };
   unsigned     zeroPaddingFactor { 0 };
   int          frequencyGain;
   std::vector<float> freq;
   std::vector<sampleCount> where;

   int          dirty;
};

class SpecPxCache {
public:
   SpecPxCache(size_t cacheLen)
      : len{ cacheLen }
      , values{ len }
   {
      valid = false;
      scaleType = 0;
      range = gain = -1;
      minFreq = maxFreq = -1;
   }

   size_t  len;
   Floats values;
   bool         valid;

   int scaleType;
   int range;
   int gain;
   int minFreq;
   int maxFreq;
};

struct WaveClipSpectrumCache final : WaveClipListener
{
   WaveClipSpectrumCache();
   ~WaveClipSpectrumCache() override;

   // Cache of values to colour pixels of Spectrogram - used by TrackArtist
   std::unique_ptr<SpecPxCache> mSpecPxCache;
   std::unique_ptr<SpecCache> mSpecCache;
   int mDirty { 0 };

   static WaveClipSpectrumCache &Get( const WaveClip &clip );

   void MarkChanged() override; // NOFAIL-GUARANTEE
   void Invalidate() override; // NOFAIL-GUARANTEE

   /** Getting high-level data for screen display */
   bool GetSpectrogram(const WaveClip &clip, WaveTrackCache &cache,
                       const float *& spectrogram,
                       const sampleCount *& where,
                       size_t numPixels,
                       double t0, double pixelsPerSecond);
};

WaveClipSpectrumCache::WaveClipSpectrumCache()
: mSpecCache{ std::make_unique<SpecCache>() }
, mSpecPxCache{ std::make_unique<SpecPxCache>(1) }
{
}

WaveClipSpectrumCache::~WaveClipSpectrumCache()
{
}

static WaveClip::Caches::RegisteredFactory sKeyS{ []( WaveClip& ){
   return std::make_unique< WaveClipSpectrumCache >();
} };

WaveClipSpectrumCache &WaveClipSpectrumCache::Get( const WaveClip &clip )
{
   return const_cast< WaveClip& >( clip )
      .Caches::Get< WaveClipSpectrumCache >( sKeyS );
}

void WaveClipSpectrumCache::MarkChanged()
{
   ++mDirty;
}

void WaveClipSpectrumCache::Invalidate()
{
   // Invalidate the spectrum display cache
   mSpecCache = std::make_unique<SpecCache>();
}

namespace {

void ComputeSpectrogramGainFactors
   (size_t fftLen, double rate, int frequencyGain, std::vector<float> &gainFactors)
{
   if (frequencyGain > 0) {
      // Compute a frequency-dependent gain factor
      // scaled such that 1000 Hz gets a gain of 0dB

      // This is the reciprocal of the bin number of 1000 Hz:
      const double factor = ((double)rate / (double)fftLen) / 1000.0;

      auto half = fftLen / 2;
      gainFactors.reserve(half);
      // Don't take logarithm of zero!  Let bin 0 replicate the gain factor for bin 1.
      gainFactors.push_back(frequencyGain*log10(factor));
      for (decltype(half) x = 1; x < half; x++) {
         gainFactors.push_back(frequencyGain*log10(factor * x));
      }
   }
}

}

bool SpecCache::Matches
   (int dirty_, double pixelsPerSecond,
    const SpectrogramSettings &settings, double rate) const
{
   // Make a tolerant comparison of the pps values in this wise:
   // accumulated difference of times over the number of pixels is less than
   // a sample period.
   const double tstep = 1.0 / pixelsPerSecond;
   const bool ppsMatch =
      (fabs(tstep - 1.0 / pps) * len < (1.0 / rate));

   return
      ppsMatch &&
      dirty == dirty_ &&
      windowType == settings.windowType &&
      windowSize == settings.WindowSize() &&
      zeroPaddingFactor == settings.ZeroPaddingFactor() &&
      frequencyGain == settings.frequencyGain &&
      algorithm == settings.algorithm;
}

static void ComputeSpectrumUsingRealFFTf
   (float * __restrict buffer, const FFTParam *hFFT,
    const float * __restrict window, size_t len, float * __restrict out)
{
   size_t i;
   if(len > hFFT->Points * 2)
      len = hFFT->Points * 2;
   for(i = 0; i < len; i++)
      buffer[i] *= window[i];
   for( ; i < (hFFT->Points * 2); i++)
      buffer[i] = 0; // zero pad as needed
   RealFFTf(buffer, hFFT);
   // Handle the (real-only) DC
   float power = buffer[0] * buffer[0];
   if(power <= 0)
      out[0] = -160.0;
   else
      out[0] = 10.0 * log10f(power);
   for(i = 1; i < hFFT->Points; i++) {
      const int index = hFFT->BitReversed[i];
      const float re = buffer[index], im = buffer[index + 1];
      power = re * re + im * im;
      if(power <= 0)
         out[i] = -160.0;
      else
         out[i] = 10.0*log10f(power);
   }
}

bool SpecCache::CalculateOneSpectrum
   (const SpectrogramSettings &settings,
    WaveTrackCache &waveTrackCache,
    const int xx, const sampleCount numSamples,
    double offset, double rate, double pixelsPerSecond,
    int lowerBoundX, int upperBoundX,
    const std::vector<float> &gainFactors,
    float* __restrict scratch, float* __restrict out) const
{
   bool result = false;
   const bool reassignment =
      (settings.algorithm == SpectrogramSettings::algReassignment);
   const size_t windowSizeSetting = settings.WindowSize();

   sampleCount from;

   // xx may be for a column that is out of the visible bounds, but only
   // when we are calculating reassignment contributions that may cross into
   // the visible area.

   if (xx < 0)
      from = sampleCount(
         where[0].as_double() + xx * (rate / pixelsPerSecond)
      );
   else if (xx > (int)len)
      from = sampleCount(
         where[len].as_double() + (xx - len) * (rate / pixelsPerSecond)
      );
   else
      from = where[xx];

   const bool autocorrelation =
      settings.algorithm == SpectrogramSettings::algPitchEAC;
   const size_t zeroPaddingFactorSetting = settings.ZeroPaddingFactor();
   const size_t padding = (windowSizeSetting * (zeroPaddingFactorSetting - 1)) / 2;
   const size_t fftLen = windowSizeSetting * zeroPaddingFactorSetting;
   auto nBins = settings.NBins();

   if (from < 0 || from >= numSamples) {
      if (xx >= 0 && xx < (int)len) {
         // Pixel column is out of bounds of the clip!  Should not happen.
         float *const results = &out[nBins * xx];
         std::fill(results, results + nBins, 0.0f);
      }
   }
   else {


      // We can avoid copying memory when ComputeSpectrum is used below
      bool copy = !autocorrelation || (padding > 0) || reassignment;
      float *useBuffer = 0;
      float *adj = scratch + padding;

      {
         auto myLen = windowSizeSetting;
         // Take a window of the track centered at this sample.
         from -= windowSizeSetting >> 1;
         if (from < 0) {
            // Near the start of the clip, pad left with zeroes as needed.
            // from is at least -windowSize / 2
            for (auto ii = from; ii < 0; ++ii)
               *adj++ = 0;
            myLen += from.as_long_long(); // add a negative
            from = 0;
            copy = true;
         }

         if (from + myLen >= numSamples) {
            // Near the end of the clip, pad right with zeroes as needed.
            // newlen is bounded by myLen:
            auto newlen = ( numSamples - from ).as_size_t();
            for (decltype(myLen) ii = newlen; ii < myLen; ++ii)
               adj[ii] = 0;
            myLen = newlen;
            copy = true;
         }

         if (myLen > 0) {
            useBuffer = (float*)(waveTrackCache.GetFloats(
               sampleCount(
                  floor(0.5 + from.as_double() + offset * rate)
               ),
               myLen,
               // Don't throw in this drawing operation
               false)
            );

            if (copy) {
               if (useBuffer)
                  memcpy(adj, useBuffer, myLen * sizeof(float));
               else
                  memset(adj, 0, myLen * sizeof(float));
            }
         }
      }

      if (copy || !useBuffer)
         useBuffer = scratch;

      if (autocorrelation) {
         // not reassignment, xx is surely within bounds.
         wxASSERT(xx >= 0);
         float *const results = &out[nBins * xx];
         // This function does not mutate useBuffer
         ComputeSpectrum(useBuffer, windowSizeSetting, windowSizeSetting,
            rate, results,
            autocorrelation, settings.windowType);
      }
      else if (reassignment) {
         static const double epsilon = 1e-16;
         const auto hFFT = settings.hFFT.get();

         float *const scratch2 = scratch + fftLen;
         std::copy(scratch, scratch2, scratch2);

         float *const scratch3 = scratch + 2 * fftLen;
         std::copy(scratch, scratch2, scratch3);

         {
            const float *const window = settings.window.get();
            for (size_t ii = 0; ii < fftLen; ++ii)
               scratch[ii] *= window[ii];
            RealFFTf(scratch, hFFT);
         }

         {
            const float *const dWindow = settings.dWindow.get();
            for (size_t ii = 0; ii < fftLen; ++ii)
               scratch2[ii] *= dWindow[ii];
            RealFFTf(scratch2, hFFT);
         }

         {
            const float *const tWindow = settings.tWindow.get();
            for (size_t ii = 0; ii < fftLen; ++ii)
               scratch3[ii] *= tWindow[ii];
            RealFFTf(scratch3, hFFT);
         }

         for (size_t ii = 0; ii < hFFT->Points; ++ii) {
            const int index = hFFT->BitReversed[ii];
            const float
               denomRe = scratch[index],
               denomIm = ii == 0 ? 0 : scratch[index + 1];
            const double power = denomRe * denomRe + denomIm * denomIm;
            if (power < epsilon)
               // Avoid dividing by near-zero below
               continue;

            double freqCorrection;
            {
               const double multiplier = -(fftLen / (2.0f * M_PI));
               const float
                  numRe = scratch2[index],
                  numIm = ii == 0 ? 0 : scratch2[index + 1];
               // Find complex quotient --
               // Which means, multiply numerator by conjugate of denominator,
               // then divide by norm squared of denominator --
               // Then just take its imaginary part.
               const double
                  quotIm = (-numRe * denomIm + numIm * denomRe) / power;
               // With appropriate multiplier, that becomes the correction of
               // the frequency bin.
               freqCorrection = multiplier * quotIm;
            }

            const int bin = (int)((int)ii + freqCorrection + 0.5f);
            // Must check if correction takes bin out of bounds, above or below!
            // bin is signed!
            if (bin >= 0 && bin < (int)hFFT->Points) {
               double timeCorrection;
               {
                  const float
                     numRe = scratch3[index],
                     numIm = ii == 0 ? 0 : scratch3[index + 1];
                  // Find another complex quotient --
                  // Then just take its real part.
                  // The result has sample interval as unit.
                  timeCorrection =
                     (numRe * denomRe + numIm * denomIm) / power;
               }

               int correctedX = (floor(0.5 + xx + timeCorrection * pixelsPerSecond / rate));
               if (correctedX >= lowerBoundX && correctedX < upperBoundX)
               {
                  result = true;

                  // This is non-negative, because bin and correctedX are
                  auto ind = (int)nBins * correctedX + bin;
#ifdef _OPENMP
                  // This assignment can race if index reaches into another thread's bins.
                  // The probability of a race very low, so this carries little overhead,
                  // about 5% slower vs allowing it to race.
                  #pragma omp atomic update
#endif
                  out[ind] += power;
               }
            }
         }
      }
      else {
         // not reassignment, xx is surely within bounds.
         wxASSERT(xx >= 0);
         float *const results = &out[nBins * xx];

         // Do the FFT.  Note that useBuffer is multiplied by the window,
         // and the window is initialized with leading and trailing zeroes
         // when there is padding.  Therefore we did not need to reinitialize
         // the part of useBuffer in the padding zones.

         // This function mutates useBuffer
         ComputeSpectrumUsingRealFFTf
            (useBuffer, settings.hFFT.get(), settings.window.get(), fftLen, results);
         if (!gainFactors.empty()) {
            // Apply a frequency-dependent gain factor
            for (size_t ii = 0; ii < nBins; ++ii)
               results[ii] += gainFactors[ii];
         }
      }
   }

   return result;
}

void SpecCache::Grow(size_t len_, const SpectrogramSettings& settings,
                       double pixelsPerSecond, double start_)
{
   settings.CacheWindows();

   // len columns, and so many rows, column-major.
   // Don't take column literally -- this isn't pixel data yet, it's the
   // raw data to be mapped onto the display.
   freq.resize(len_ * settings.NBins());

   // Sample counts corresponding to the columns, and to one past the end.
   where.resize(len_ + 1);

   len = len_;
   algorithm = settings.algorithm;
   pps = pixelsPerSecond;
   start = start_;
   windowType = settings.windowType;
   windowSize = settings.WindowSize();
   zeroPaddingFactor = settings.ZeroPaddingFactor();
   frequencyGain = settings.frequencyGain;
}

void SpecCache::Populate
   (const SpectrogramSettings &settings, WaveTrackCache &waveTrackCache,
    int copyBegin, int copyEnd, size_t numPixels,
    sampleCount numSamples,
    double offset, double rate, double pixelsPerSecond)
{
   const int &frequencyGainSetting = settings.frequencyGain;
   const size_t windowSizeSetting = settings.WindowSize();
   const bool autocorrelation =
      settings.algorithm == SpectrogramSettings::algPitchEAC;
   const bool reassignment =
      settings.algorithm == SpectrogramSettings::algReassignment;
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   const size_t zeroPaddingFactorSetting = settings.ZeroPaddingFactor();
#else
   const size_t zeroPaddingFactorSetting = 1;
#endif

   // FFT length may be longer than the window of samples that affect results
   // because of zero padding done for increased frequency resolution
   const size_t fftLen = windowSizeSetting * zeroPaddingFactorSetting;
   const auto nBins = settings.NBins();

   const size_t bufferSize = fftLen;
   const size_t scratchSize = reassignment ? 3 * bufferSize : bufferSize;
   std::vector<float> scratch(scratchSize);

   std::vector<float> gainFactors;
   if (!autocorrelation)
      ComputeSpectrogramGainFactors(fftLen, rate, frequencyGainSetting, gainFactors);

   // Loop over the ranges before and after the copied portion and compute anew.
   // One of the ranges may be empty.
   for (int jj = 0; jj < 2; ++jj) {
      const int lowerBoundX = jj == 0 ? 0 : copyEnd;
      const int upperBoundX = jj == 0 ? copyBegin : numPixels;

#ifdef _OPENMP
      // Storage for mutable per-thread data.
      // private clause ensures one copy per thread
      struct ThreadLocalStorage {
         ThreadLocalStorage()  { }
         ~ThreadLocalStorage() { }

         void init(WaveTrackCache &waveTrackCache, size_t scratchSize) {
            if (!cache) {
               cache = std::make_unique<WaveTrackCache>(waveTrackCache.GetTrack());
               scratch.resize(scratchSize);
            }
         }
         std::unique_ptr<WaveTrackCache> cache;
         std::vector<float> scratch;
      } tls;

      #pragma omp parallel for private(tls)
#endif
      for (auto xx = lowerBoundX; xx < upperBoundX; ++xx)
      {
#ifdef _OPENMP
         tls.init(waveTrackCache, scratchSize);
         WaveTrackCache& cache = *tls.cache;
         float* buffer = &tls.scratch[0];
#else
         WaveTrackCache& cache = waveTrackCache;
         float* buffer = &scratch[0];
#endif
         CalculateOneSpectrum(
            settings, cache, xx, numSamples,
            offset, rate, pixelsPerSecond,
            lowerBoundX, upperBoundX,
            gainFactors, buffer, &freq[0]);
      }

      if (reassignment) {
         // Need to look beyond the edges of the range to accumulate more
         // time reassignments.
         // I'm not sure what's a good stopping criterion?
         auto xx = lowerBoundX;
         const double pixelsPerSample = pixelsPerSecond / rate;
         const int limit = std::min((int)(0.5 + fftLen * pixelsPerSample), 100);
         for (int ii = 0; ii < limit; ++ii)
         {
            const bool result =
               CalculateOneSpectrum(
                  settings, waveTrackCache, --xx, numSamples,
                  offset, rate, pixelsPerSecond,
                  lowerBoundX, upperBoundX,
                  gainFactors, &scratch[0], &freq[0]);
            if (!result)
               break;
         }

         xx = upperBoundX;
         for (int ii = 0; ii < limit; ++ii)
         {
            const bool result =
               CalculateOneSpectrum(
                  settings, waveTrackCache, xx++, numSamples,
                  offset, rate, pixelsPerSecond,
                  lowerBoundX, upperBoundX,
                  gainFactors, &scratch[0], &freq[0]);
            if (!result)
               break;
         }

         // Now Convert to dB terms.  Do this only after accumulating
         // power values, which may cross columns with the time correction.
#ifdef _OPENMP
         #pragma omp parallel for
#endif
         for (xx = lowerBoundX; xx < upperBoundX; ++xx) {
            float *const results = &freq[nBins * xx];
            for (size_t ii = 0; ii < nBins; ++ii) {
               float &power = results[ii];
               if (power <= 0)
                  power = -160.0;
               else
                  power = 10.0*log10f(power);
            }
            if (!gainFactors.empty()) {
               // Apply a frequency-dependent gain factor
               for (size_t ii = 0; ii < nBins; ++ii)
                  results[ii] += gainFactors[ii];
            }
         }
      }
   }
}

bool WaveClipSpectrumCache::GetSpectrogram(const WaveClip &clip,
                              WaveTrackCache &waveTrackCache,
                              const float *& spectrogram,
                              const sampleCount *& where,
                              size_t numPixels,
                              double t0, double pixelsPerSecond)
{
   const WaveTrack *const track = waveTrackCache.GetTrack().get();
   const auto &settings = SpectrogramSettings::Get(*track);
   const auto rate = clip.GetRate();

   bool match =
      mSpecCache &&
      mSpecCache->len > 0 &&
      mSpecCache->Matches
      (mDirty, pixelsPerSecond, settings, rate);

   if (match &&
       mSpecCache->start == t0 &&
       mSpecCache->len >= numPixels) {
      spectrogram = &mSpecCache->freq[0];
      where = &mSpecCache->where[0];

      return false;  //hit cache completely
   }

   // Caching is not implemented for reassignment, unless for
   // a complete hit, because of the complications of time reassignment
   if (settings.algorithm == SpectrogramSettings::algReassignment)
      match = false;

   // Free the cache when it won't cause a major stutter.
   // If the window size changed, we know there is nothing to be copied
   // If we zoomed out, or resized, we can give up memory. But not too much -
   // up to 2x extra is needed at the end of the clip to prevent stutter.
   if (mSpecCache->freq.capacity() > 2.1 * mSpecCache->freq.size() ||
       mSpecCache->windowSize*mSpecCache->zeroPaddingFactor <
       settings.WindowSize()*settings.ZeroPaddingFactor())
   {
      match = false;
      mSpecCache = std::make_unique<SpecCache>();
   }

   const double tstep = 1.0 / pixelsPerSecond;
   const double samplesPerPixel = rate * tstep;

   int oldX0 = 0;
   double correction = 0.0;

   int copyBegin = 0, copyEnd = 0;
   if (match) {
      findCorrection(mSpecCache->where, mSpecCache->len, numPixels,
         t0, rate, samplesPerPixel,
         oldX0, correction);
      // Remember our first pixel maps to oldX0 in the old cache,
      // possibly out of bounds.
      // For what range of pixels can data be copied?
      copyBegin = std::min((int)numPixels, std::max(0, -oldX0));
      copyEnd = std::min((int)numPixels, std::max(0,
         (int)mSpecCache->len - oldX0
      ));
   }

   // Resize the cache, keep the contents unchanged.
   mSpecCache->Grow(numPixels, settings, pixelsPerSecond, t0);
   auto nBins = settings.NBins();

   // Optimization: if the old cache is good and overlaps
   // with the current one, re-use as much of the cache as
   // possible
   if (copyEnd > copyBegin)
   {
      // memmove is required since dst/src overlap
      memmove(&mSpecCache->freq[nBins * copyBegin],
               &mSpecCache->freq[nBins * (copyBegin + oldX0)],
               nBins * (copyEnd - copyBegin) * sizeof(float));
   }

   // Reassignment accumulates, so it needs a zeroed buffer
   if (settings.algorithm == SpectrogramSettings::algReassignment)
   {
      // The cache could theoretically copy from the middle, resulting
      // in two regions to update. This won't happen in zoom, since
      // old cache doesn't match. It won't happen in resize, since the
      // spectrum view is pinned to left side of window.
      wxASSERT(
         (copyBegin >= 0 && copyEnd == (int)numPixels) || // copied the end
         (copyBegin == 0 && copyEnd <= (int)numPixels)    // copied the beginning
      );

      int zeroBegin = copyBegin > 0 ? 0 : copyEnd-copyBegin;
      int zeroEnd = copyBegin > 0 ? copyBegin : numPixels;

      memset(&mSpecCache->freq[nBins*zeroBegin], 0, nBins*(zeroEnd-zeroBegin)*sizeof(float));
   }

   // purposely offset the display 1/2 sample to the left (as compared
   // to waveform display) to properly center response of the FFT
   fillWhere(mSpecCache->where, numPixels, 0.5, correction,
      t0, rate, samplesPerPixel);

   mSpecCache->Populate
      (settings, waveTrackCache, copyBegin, copyEnd, numPixels,
       clip.GetSequence()->GetNumSamples(),
       clip.GetOffset(), rate, pixelsPerSecond);

   mSpecCache->dirty = mDirty;
   spectrogram = &mSpecCache->freq[0];
   where = &mSpecCache->where[0];

   return true;
}

void DrawClipSpectrum(TrackPanelDrawingContext &context,
                                   WaveTrackCache &waveTrackCache,
                                   const WaveClip *clip,
                                   const wxRect & rect,
                                   bool selected)
{
   auto &dc = context.dc;
   const auto artist = TrackArtist::Get( context );
   const auto &selectedRegion = *artist->pSelectedRegion;
   const auto &zoomInfo = *artist->pZoomInfo;

#ifdef PROFILE_WAVEFORM
   Profiler profiler;
#endif

   const WaveTrack *const track = waveTrackCache.GetTrack().get();
   const auto &settings = SpectrogramSettings::Get(*track);
   const bool autocorrelation = (settings.algorithm == SpectrogramSettings::algPitchEAC);

   enum { DASH_LENGTH = 10 /* pixels */ };

   const ClipParameters params{
      true, track, clip, rect, selectedRegion, zoomInfo };
   const wxRect &hiddenMid = params.hiddenMid;
   // The "hiddenMid" rect contains the part of the display actually
   // containing the waveform, as it appears without the fisheye.  If it's empty, we're done.
   if (hiddenMid.width <= 0) {
      return;
   }

   const double &t0 = params.t0;
   const double &tOffset = params.tOffset;
   const auto &ssel0 = params.ssel0;
   const auto &ssel1 = params.ssel1;
   const double &averagePixelsPerSample = params.averagePixelsPerSample;
   const double &rate = params.rate;
   const double &hiddenLeftOffset = params.hiddenLeftOffset;
   const double &leftOffset = params.leftOffset;
   const wxRect &mid = params.mid;

   double freqLo = SelectedRegion::UndefinedFrequency;
   double freqHi = SelectedRegion::UndefinedFrequency;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   freqLo = selectedRegion.f0();
   freqHi = selectedRegion.f1();
#endif

   const int &colorScheme = settings.colorScheme;
   const int &range = settings.range;
   const int &gain = settings.gain;

#ifdef EXPERIMENTAL_FIND_NOTES
   const bool &fftFindNotes = settings.fftFindNotes;
   const double &findNotesMinA = settings.findNotesMinA;
   const int &numberOfMaxima = settings.numberOfMaxima;
   const bool &findNotesQuantize = settings.findNotesQuantize;
#endif
#ifdef EXPERIMENTAL_FFT_Y_GRID
   const bool &fftYGrid = settings.fftYGrid;
#endif

   dc.SetPen(*wxTRANSPARENT_PEN);

   // We draw directly to a bit image in memory,
   // and then paint this directly to our offscreen
   // bitmap.  Note that this could be optimized even
   // more, but for now this is not bad.  -dmazzoni
   wxImage image((int)mid.width, (int)mid.height);
   if (!image.IsOk())
      return;
#ifdef EXPERIMENTAL_SPECTROGRAM_OVERLAY
   image.SetAlpha();
   unsigned char *alpha = image.GetAlpha();
#endif
   unsigned char *data = image.GetData();

   const auto half = settings.GetFFTLength() / 2;
   const double binUnit = rate / (2 * half);
   const float *freq = 0;
   const sampleCount *where = 0;
   bool updated;
   {
      const double pps = averagePixelsPerSample * rate;
      updated = WaveClipSpectrumCache::Get( *clip ).GetSpectrogram( *clip,
         waveTrackCache, freq, where,
         (size_t)hiddenMid.width,
         t0, pps);
   }
   auto nBins = settings.NBins();

   float minFreq, maxFreq;
   SpectrogramSettingsCache::Get(*track)
      .GetBounds(*track, minFreq, maxFreq);

   const SpectrogramSettings::ScaleType scaleType = settings.scaleType;

   // nearest frequency to each pixel row from number scale, for selecting
   // the desired fft bin(s) for display on that row
   float *bins = (float*)alloca(sizeof(*bins)*(hiddenMid.height + 1));
   {
      const NumberScale numberScale( settings.GetScale( minFreq, maxFreq ) );

      NumberScale::Iterator it = numberScale.begin(mid.height);
      float nextBin = std::max( 0.0f, std::min( float(nBins - 1),
         settings.findBin( *it, binUnit ) ) );

      int yy;
      for (yy = 0; yy < hiddenMid.height; ++yy) {
         bins[yy] = nextBin;
         nextBin = std::max( 0.0f, std::min( float(nBins - 1),
            settings.findBin( *++it, binUnit ) ) );
      }
      bins[yy] = nextBin;
   }

#ifdef EXPERIMENTAL_FFT_Y_GRID
   const float
      log2 = logf(2.0f),
      scale2 = (lmax - lmin) / log2,
      lmin2 = lmin / log2;

   ArrayOf<bool> yGrid{size_t(mid.height)};
   for (int yy = 0; yy < mid.height; ++yy) {
      float n = (float(yy) / mid.height*scale2 - lmin2) * 12;
      float n2 = (float(yy + 1) / mid.height*scale2 - lmin2) * 12;
      float f = float(minFreq) / (fftSkipPoints + 1)*powf(2.0f, n / 12.0f + lmin2);
      float f2 = float(minFreq) / (fftSkipPoints + 1)*powf(2.0f, n2 / 12.0f + lmin2);
      n = logf(f / 440) / log2 * 12;
      n2 = logf(f2 / 440) / log2 * 12;
      if (floor(n) < floor(n2))
         yGrid[yy] = true;
      else
         yGrid[yy] = false;
   }
#endif //EXPERIMENTAL_FFT_Y_GRID

   auto &clipCache = WaveClipSpectrumCache::Get( *clip );
   if (!updated && clipCache.mSpecPxCache->valid &&
      ((int)clipCache.mSpecPxCache->len == hiddenMid.height * hiddenMid.width)
      && scaleType == clipCache.mSpecPxCache->scaleType
      && gain == clipCache.mSpecPxCache->gain
      && range == clipCache.mSpecPxCache->range
      && minFreq == clipCache.mSpecPxCache->minFreq
      && maxFreq == clipCache.mSpecPxCache->maxFreq
#ifdef EXPERIMENTAL_FFT_Y_GRID
   && fftYGrid==fftYGridOld
#endif //EXPERIMENTAL_FFT_Y_GRID
#ifdef EXPERIMENTAL_FIND_NOTES
   && fftFindNotes == artist->fftFindNotesOld
   && findNotesMinA == artist->findNotesMinAOld
   && numberOfMaxima == artist->findNotesNOld
   && findNotesQuantize == artist->findNotesQuantizeOld
#endif
   ) {
      // Wave clip's spectrum cache is up to date,
      // and so is the spectrum pixel cache
   }
   else {
      // Update the spectrum pixel cache
      clipCache.mSpecPxCache = std::make_unique<SpecPxCache>(hiddenMid.width * hiddenMid.height);
      clipCache.mSpecPxCache->valid = true;
      clipCache.mSpecPxCache->scaleType = scaleType;
      clipCache.mSpecPxCache->gain = gain;
      clipCache.mSpecPxCache->range = range;
      clipCache.mSpecPxCache->minFreq = minFreq;
      clipCache.mSpecPxCache->maxFreq = maxFreq;
#ifdef EXPERIMENTAL_FIND_NOTES
      artist->fftFindNotesOld = fftFindNotes;
      artist->findNotesMinAOld = findNotesMinA;
      artist->findNotesNOld = numberOfMaxima;
      artist->findNotesQuantizeOld = findNotesQuantize;
#endif

#ifdef EXPERIMENTAL_FIND_NOTES
      float log2 = logf( 2.0f ),
         lmin = logf( minFreq ), lmax = logf( maxFreq ), scale = lmax - lmin,
         lmins = lmin,
         lmaxs = lmax
         ;
#endif //EXPERIMENTAL_FIND_NOTES

#ifdef EXPERIMENTAL_FIND_NOTES
      int maxima[128];
      float maxima0[128], maxima1[128];
      const float
         f2bin = half / (rate / 2.0f),
         bin2f = 1.0f / f2bin,
         minDistance = powf(2.0f, 2.0f / 12.0f),
         i0 = expf(lmin) / binUnit,
         i1 = expf(scale + lmin) / binUnit,
         minColor = 0.0f;
      const size_t maxTableSize = 1024;
      ArrayOf<int> indexes{ maxTableSize };
#endif //EXPERIMENTAL_FIND_NOTES

#ifdef _OPENMP
#pragma omp parallel for
#endif
      for (int xx = 0; xx < hiddenMid.width; ++xx) {
#ifdef EXPERIMENTAL_FIND_NOTES
         int maximas = 0;
         const int x0 = nBins * xx;
         if (fftFindNotes) {
            for (int i = maxTableSize - 1; i >= 0; i--)
               indexes[i] = -1;

            // Build a table of (most) values, put the index in it.
            for (int i = (int)(i0); i < (int)(i1); i++) {
               float freqi = freq[x0 + (int)(i)];
               int value = (int)((freqi + gain + range) / range*(maxTableSize - 1));
               if (value < 0)
                  value = 0;
               if (value >= maxTableSize)
                  value = maxTableSize - 1;
               indexes[value] = i;
            }
            // Build from the indices an array of maxima.
            for (int i = maxTableSize - 1; i >= 0; i--) {
               int index = indexes[i];
               if (index >= 0) {
                  float freqi = freq[x0 + index];
                  if (freqi < findNotesMinA)
                     break;

                  bool ok = true;
                  for (int m = 0; m < maximas; m++) {
                     // Avoid to store very close maxima.
                     float maxm = maxima[m];
                     if (maxm / index < minDistance && index / maxm < minDistance) {
                        ok = false;
                        break;
                     }
                  }
                  if (ok) {
                     maxima[maximas++] = index;
                     if (maximas >= numberOfMaxima)
                        break;
                  }
               }
            }

// The f2pix helper macro converts a frequency into a pixel coordinate.
#define f2pix(f) (logf(f)-lmins)/(lmaxs-lmins)*hiddenMid.height

            // Possibly quantize the maxima frequencies and create the pixel block limits.
            for (int i = 0; i < maximas; i++) {
               int index = maxima[i];
               float f = float(index)*bin2f;
               if (findNotesQuantize)
               {
                  f = expf((int)(log(f / 440) / log2 * 12 - 0.5) / 12.0f*log2) * 440;
                  maxima[i] = f*f2bin;
               }
               float f0 = expf((log(f / 440) / log2 * 24 - 1) / 24.0f*log2) * 440;
               maxima0[i] = f2pix(f0);
               float f1 = expf((log(f / 440) / log2 * 24 + 1) / 24.0f*log2) * 440;
               maxima1[i] = f2pix(f1);
            }
         }

         int it = 0;
         bool inMaximum = false;
#endif //EXPERIMENTAL_FIND_NOTES

         for (int yy = 0; yy < hiddenMid.height; ++yy) {
            const float bin     = bins[yy];
            const float nextBin = bins[yy+1];

            if (settings.scaleType != SpectrogramSettings::stLogarithmic) {
               const float value = findValue
                  (freq + nBins * xx, bin, nextBin, nBins, autocorrelation, gain, range);
               clipCache.mSpecPxCache->values[xx * hiddenMid.height + yy] = value;
            }
            else {
               float value;

#ifdef EXPERIMENTAL_FIND_NOTES
               if (fftFindNotes) {
                  if (it < maximas) {
                     float i0 = maxima0[it];
                     if (yy >= i0)
                        inMaximum = true;

                     if (inMaximum) {
                        float i1 = maxima1[it];
                        if (yy + 1 <= i1) {
                           value = findValue(freq + x0, bin, nextBin, nBins, autocorrelation, gain, range);
                           if (value < findNotesMinA)
                              value = minColor;
                        }
                        else {
                           it++;
                           inMaximum = false;
                           value = minColor;
                        }
                     }
                     else {
                        value = minColor;
                     }
                  }
                  else
                     value = minColor;
               }
               else
#endif //EXPERIMENTAL_FIND_NOTES
               {
                  value = findValue
                     (freq + nBins * xx, bin, nextBin, nBins, autocorrelation, gain, range);
               }
               clipCache.mSpecPxCache->values[xx * hiddenMid.height + yy] = value;
            } // logF
         } // each yy
      } // each xx
   } // updating cache

   float selBinLo = settings.findBin( freqLo, binUnit);
   float selBinHi = settings.findBin( freqHi, binUnit);
   float selBinCenter = (freqLo < 0 || freqHi < 0)
      ? -1
      : settings.findBin( sqrt(freqLo * freqHi), binUnit );

   const bool isSpectral = settings.SpectralSelectionEnabled();
   const bool hidden = (ZoomInfo::HIDDEN == zoomInfo.GetFisheyeState());
   const int begin = hidden
      ? 0
      : std::max(0, (int)(zoomInfo.GetFisheyeLeftBoundary(-leftOffset)));
   const int end = hidden
      ? 0
      : std::min(mid.width, (int)(zoomInfo.GetFisheyeRightBoundary(-leftOffset)));
   const size_t numPixels = std::max(0, end - begin);

   SpecCache specCache;

   // need explicit resize since specCache.where[] accessed before Populate()
   specCache.Grow(numPixels, settings, -1, t0);

   if (numPixels > 0) {
      for (int ii = begin; ii < end; ++ii) {
         const double time = zoomInfo.PositionToTime(ii, -leftOffset) - tOffset;
         specCache.where[ii - begin] = sampleCount(0.5 + rate * time);
      }
      specCache.Populate
         (settings, waveTrackCache,
          0, 0, numPixels,
          clip->GetNumSamples(),
          tOffset, rate,
          0 // FIXME: PRL -- make reassignment work with fisheye
       );
   }

   // build color gradient tables (not thread safe)
   if (!AColor::gradient_inited)
      AColor::PreComputeGradient();

   // left pixel column of the fisheye
   int fisheyeLeft = zoomInfo.GetFisheyeLeftBoundary(-leftOffset);

   // Bug 2389 - always draw at least one pixel of selection.
   int selectedX = zoomInfo.TimeToPosition(selectedRegion.t0(), -leftOffset);

#ifdef _OPENMP
#pragma omp parallel for
#endif
   for (int xx = 0; xx < mid.width; ++xx) {

      int correctedX = xx + leftOffset - hiddenLeftOffset;

      // in fisheye mode the time scale has changed, so the row values aren't cached
      // in the loop above, and must be fetched from fft cache
      float* uncached;
      if (!zoomInfo.InFisheye(xx, -leftOffset)) {
          uncached = 0;
      }
      else {
          int specIndex = (xx - fisheyeLeft) * nBins;
          wxASSERT(specIndex >= 0 && specIndex < (int)specCache.freq.size());
          uncached = &specCache.freq[specIndex];
      }

      // zoomInfo must be queried for each column since with fisheye enabled
      // time between columns is variable
      auto w0 = sampleCount(0.5 + rate *
                   (zoomInfo.PositionToTime(xx, -leftOffset) - tOffset));

      auto w1 = sampleCount(0.5 + rate *
                    (zoomInfo.PositionToTime(xx+1, -leftOffset) - tOffset));

      bool maybeSelected = ssel0 <= w0 && w1 < ssel1;
      maybeSelected = maybeSelected || (xx == selectedX);

      for (int yy = 0; yy < hiddenMid.height; ++yy) {
         const float bin     = bins[yy];
         const float nextBin = bins[yy+1];

         // For spectral selection, determine what colour
         // set to use.  We use a darker selection if
         // in both spectral range and time range.

         AColor::ColorGradientChoice selected = AColor::ColorGradientUnselected;

         // If we are in the time selected range, then we may use a different color set.
         if (maybeSelected)
            selected =
               ChooseColorSet(bin, nextBin, selBinLo, selBinCenter, selBinHi,
                  (xx + leftOffset - hiddenLeftOffset) / DASH_LENGTH, isSpectral);

         const float value = uncached
            ? findValue(uncached, bin, nextBin, nBins, autocorrelation, gain, range)
            : clipCache.mSpecPxCache->values[correctedX * hiddenMid.height + yy];

         unsigned char rv, gv, bv;
         GetColorGradient(value, selected, colorScheme, &rv, &gv, &bv);

#ifdef EXPERIMENTAL_FFT_Y_GRID
         if (fftYGrid && yGrid[yy]) {
            rv /= 1.1f;
            gv /= 1.1f;
            bv /= 1.1f;
         }
#endif //EXPERIMENTAL_FFT_Y_GRID

         int px = ((mid.height - 1 - yy) * mid.width + xx);
#ifdef EXPERIMENTAL_SPECTROGRAM_OVERLAY
         // More transparent the closer to zero intensity.
         alpha[px]= wxMin( 200, (value+0.3) * 500) ;
#endif
         px *=3;
         data[px++] = rv;
         data[px++] = gv;
         data[px] = bv;
      } // each yy
   } // each xx

   wxBitmap converted = wxBitmap(image);

   wxMemoryDC memDC;

   memDC.SelectObject(converted);

   dc.Blit(mid.x, mid.y, mid.width, mid.height, &memDC, 0, 0, wxCOPY, FALSE);

   // Draw clip edges, as also in waveform view, which improves the appearance
   // of split views
   {
      //increase virtual view size by px to hide edges that should not be visible
      auto clipRect = ClipParameters::GetClipRect(*clip, zoomInfo, rect.Inflate(1, 0), 1);
      if (!clipRect.IsEmpty())
         TrackArt::DrawClipEdges(dc, clipRect, selected);
   }
}

}

void SpectrumView::DoDraw(TrackPanelDrawingContext& context,
                                const WaveTrack* track,
                                const WaveClip* selectedClip,
                                const wxRect & rect )
{
   const auto artist = TrackArtist::Get( context );
   const auto &blankSelectedBrush = artist->blankSelectedBrush;
   const auto &blankBrush = artist->blankBrush;
   TrackArt::DrawBackgroundWithSelection(
      context, rect, track, blankSelectedBrush, blankBrush );

   WaveTrackCache cache(track->SharedPointer<const WaveTrack>());
   for (const auto &clip: track->GetClips())
      DrawClipSpectrum( context, cache, clip.get(), rect, clip.get() == selectedClip );

   DrawBoldBoundaries( context, track, rect );
}

void SpectrumView::Draw(
   TrackPanelDrawingContext &context, const wxRect &rect, unsigned iPass )
{
   WaveTrackSubView::Draw( context, rect, iPass );
   if ( iPass == TrackArtist::PassTracks ) {
      auto &dc = context.dc;
      // Update cache for locations, e.g. cutlines and merge points
      // Bug2588: do this for both channels, even if one is not drawn, so that
      // cut-line editing (which depends on the locations cache) works properly.
      // If both channels are visible, we will duplicate this effort, but that
      // matters little.
      for( auto channel:
          TrackList::Channels(static_cast<WaveTrack*>(FindTrack().get())) )
         channel->UpdateLocationsCache();

      const auto wt = std::static_pointer_cast<const WaveTrack>(
         FindTrack()->SubstitutePendingChangedTrack());

      const auto artist = TrackArtist::Get( context );
      
#if defined(__WXMAC__)
      wxAntialiasMode aamode = dc.GetGraphicsContext()->GetAntialiasMode();
      dc.GetGraphicsContext()->SetAntialiasMode(wxANTIALIAS_NONE);
#endif

      auto waveTrackView = GetWaveTrackView().lock();
      wxASSERT(waveTrackView.use_count());
      
      auto seletedClip = waveTrackView->GetSelectedClip().lock();
      DoDraw( context, wt.get(), seletedClip.get(), rect );
      
#if defined(__WXMAC__)
      dc.GetGraphicsContext()->SetAntialiasMode(aamode);
#endif
   }
}

static const WaveTrackSubViews::RegisteredFactory key{
   []( WaveTrackView &view ){
      return std::make_shared< SpectrumView >( view );
   }
};

// The following attaches the spectrogram settings item to the wave track popup
// menu.  It is appropriate only to spectrum view and so is kept in this
// source file with the rest of the spectrum view implementation.
#include "tracks/playabletrack/wavetrack/ui/WaveTrackControls.h"
#include "AudioIOBase.h"
#include "Menus.h"
#include "ProjectHistory.h"
#include "RefreshCode.h"
#include "prefs/PrefsDialog.h"
#include "SpectrumPrefs.h"
#include "widgets/AudacityMessageBox.h"
#include "widgets/PopupMenuTable.h"

namespace {
struct SpectrogramSettingsHandler : PopupMenuHandler {

   PlayableTrackControls::InitMenuData *mpData{};
   static SpectrogramSettingsHandler &Instance()
   {
      static SpectrogramSettingsHandler instance;
      return instance;
   }

   void OnSpectrogramSettings(wxCommandEvent &);

   void InitUserData(void *pUserData) override
   {
      mpData = static_cast< PlayableTrackControls::InitMenuData* >(pUserData);
   }

   void DestroyMenu() override
   {
      mpData = nullptr;
   }
};

void SpectrogramSettingsHandler::OnSpectrogramSettings(wxCommandEvent &)
{
   class ViewSettingsDialog final : public PrefsDialog
   {
   public:
      ViewSettingsDialog(wxWindow *parent, AudacityProject &project,
         const TranslatableString &title, PrefsPanel::Factories &factories,
         int page)
         : PrefsDialog(parent, &project, title, factories)
         , mPage(page)
      {
      }

      long GetPreferredPage() override
      {
         return mPage;
      }

      void SavePreferredPage() override
      {
      }

   private:
      const int mPage;
   };

   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO->IsBusy()){
      AudacityMessageBox(
         XO(
"To change Spectrogram Settings, stop any\n playing or recording first."),
         XO("Stop the Audio First"),
         wxOK | wxICON_EXCLAMATION | wxCENTRE);
      return;
   }

   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);

   PrefsPanel::Factories factories;
   // factories.push_back(WaveformPrefsFactory( pTrack ));
   factories.push_back(SpectrumPrefsFactory( pTrack ));
   const int page =
      // (pTrack->GetDisplay() == WaveTrackViewConstants::Spectrum) ? 1 :
      0;

   auto title = XO("%s:").Format( pTrack->GetName() );
   ViewSettingsDialog dialog(
      mpData->pParent, mpData->project, title, factories, page);

   if (0 != dialog.ShowModal()) {
      // Redraw
      AudacityProject *const project = &mpData->project;
      ProjectHistory::Get( *project ).ModifyState(true);
      //Bug 1725 Toolbar was left greyed out.
      //This solution is overkill, but does fix the problem and is what the
      //prefs dialog normally does.
      MenuCreator::RebuildAllMenuBars();
      mpData->result = RefreshCode::RefreshAll;
   }
}

PopupMenuTable::AttachedItem sAttachment{
   GetWaveTrackMenuTable(),
   { "SubViews/Extra" },
   std::make_unique<PopupMenuSection>( "SpectrogramSettings",
      // Conditionally add menu item for settings, if showing spectrum
      PopupMenuTable::Computed< WaveTrackPopupMenuTable >(
         []( WaveTrackPopupMenuTable &table ) -> Registry::BaseItemPtr {
            using Entry = PopupMenuTable::Entry;
            static const int OnSpectrogramSettingsID =
            GetWaveTrackMenuTable().ReserveId();

            const auto pTrack = &table.FindWaveTrack();
            const auto &view = WaveTrackView::Get( *pTrack );
            const auto displays = view.GetDisplays();
            bool hasSpectrum = (displays.end() != std::find(
               displays.begin(), displays.end(),
               WaveTrackSubView::Type{ { SpectrogramId, {} } }
            ) );
            if( hasSpectrum )
               // In future, we might move this to the context menu of the
               // Spectrum vertical ruler.
               // (But the latter won't be satisfactory without a means to
               // open that other context menu with keystrokes only, and that
               // would require some notion of a focused sub-view.)
               return std::make_unique<Entry>( "SpectrogramSettings",
                  Entry::Item,
                  OnSpectrogramSettingsID,
                  XXO("S&pectrogram Settings..."),
                  (wxCommandEventFunction)
                     (&SpectrogramSettingsHandler::OnSpectrogramSettings),
                  SpectrogramSettingsHandler::Instance(),
                  []( PopupMenuHandler &handler, wxMenu &menu, int id ){
                     // Bug 1253.  Shouldn't open preferences if audio is busy.
                     // We can't change them on the fly yet anyway.
                     auto gAudioIO = AudioIOBase::Get();
                     menu.Enable(id, !gAudioIO->IsBusy());
                  } );
            else
               return nullptr;
         } ) )
};
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
// Attach some related menu items
#include "tracks/ui/SelectHandle.h"
#include "CommonCommandFlags.h"
#include "Project.h"
#include "SpectrumAnalyst.h"
#include "commands/CommandContext.h"

namespace {
void DoNextPeakFrequency(AudacityProject &project, bool up)
{
   auto &tracks = TrackList::Get( project );
   auto &viewInfo = ViewInfo::Get( project );

   // Find the first selected wave track that is in a spectrogram view.
   const WaveTrack *pTrack {};
   for ( auto wt : tracks.Selected< const WaveTrack >() ) {
      const auto displays = WaveTrackView::Get( *wt ).GetDisplays();
      bool hasSpectrum = (displays.end() != std::find(
         displays.begin(), displays.end(),
         WaveTrackSubView::Type{ { SpectrogramId, {} } }
      ) );
      if ( hasSpectrum ) {
         pTrack = wt;
         break;
      }
   }

   if (pTrack) {
      SpectrumAnalyst analyst;
      SpectralSelectHandle::SnapCenterOnce(analyst, viewInfo, pTrack, up);
      ProjectHistory::Get( project ).ModifyState(false);
   }
}

struct Handler : CommandHandlerObject, ClientData::Base {

// Handler state:
double mLastF0{ SelectedRegion::UndefinedFrequency };
double mLastF1{ SelectedRegion::UndefinedFrequency };

void OnToggleSpectralSelection(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   const double f0 = selectedRegion.f0();
   const double f1 = selectedRegion.f1();
   const bool haveSpectralSelection =
   !(f0 == SelectedRegion::UndefinedFrequency &&
     f1 == SelectedRegion::UndefinedFrequency);
   if (haveSpectralSelection)
   {
      mLastF0 = f0;
      mLastF1 = f1;
      selectedRegion.setFrequencies
      (SelectedRegion::UndefinedFrequency, SelectedRegion::UndefinedFrequency);
   }
   else
      selectedRegion.setFrequencies(mLastF0, mLastF1);

   ProjectHistory::Get( project ).ModifyState(false);
}

void OnNextHigherPeakFrequency(const CommandContext &context)
{
   auto &project = context.project;
   DoNextPeakFrequency(project, true);
}

void OnNextLowerPeakFrequency(const CommandContext &context)
{
   auto &project = context.project;
   DoNextPeakFrequency(project, false);
}
};

// Handler is stateful.  Needs a factory registered with
// AudacityProject.
static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject&) {
      return std::make_unique< Handler >(); } };

static CommandHandlerObject &findCommandHandler(AudacityProject &project) {
   return project.AttachedObjects::Get< Handler >( key );
};

using namespace MenuTable;
#define FN(X) (& Handler :: X)

BaseItemSharedPtr SpectralSelectionMenu()
{
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( wxT("Spectral"), XXO("S&pectral"),
      Command( wxT("ToggleSpectralSelection"),
         XXO("To&ggle Spectral Selection"), FN(OnToggleSpectralSelection),
         TracksExistFlag(), wxT("Q") ),
      Command( wxT("NextHigherPeakFrequency"),
         XXO("Next &Higher Peak Frequency"), FN(OnNextHigherPeakFrequency),
         TracksExistFlag() ),
      Command( wxT("NextLowerPeakFrequency"),
         XXO("Next &Lower Peak Frequency"), FN(OnNextLowerPeakFrequency),
         TracksExistFlag() )
   ) ) };
   return menu;
}

#undef FN

AttachedItem sAttachment2{
   Placement{ wxT("Select/Basic"), { OrderingHint::After, wxT("Region") } },
   Shared( SpectralSelectionMenu() )
};

}
#endif // EXPERIMENTAL_SPECTRAL_EDITING
