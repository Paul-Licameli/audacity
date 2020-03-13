/**********************************************************************

Audacity: A Digital Audio Editor

SpectrogramSettings.cpp

Paul Licameli

*******************************************************************//**

\class SpectrogramSettings
\brief Spectrogram settings, either for one track or as defaults.

*//*******************************************************************/


#include "SpectrogramSettings.h"

#include "../AColor.h"
#include "../NumberScale.h"

#include <algorithm>

#include "../FFT.h"
#include "../Prefs.h"

#include <cmath>

#include "../widgets/AudacityMessageBox.h"

SpectrogramSettings::Globals::Globals()
{
   LoadPrefs();
}

void SpectrogramSettings::Globals::SavePrefs()
{
#ifdef SPECTRAL_SELECTION_GLOBAL_SWITCH
   gPrefs->Write(L"/Spectrum/EnableSpectralSelection", spectralSelection);
#endif
}

void SpectrogramSettings::Globals::LoadPrefs()
{
#ifdef SPECTRAL_SELECTION_GLOBAL_SWITCH
   spectralSelection
      = (gPrefs->Read(L"/Spectrum/EnableSpectralSelection", 1L) != 0);
#endif
}

SpectrogramSettings::Globals
&SpectrogramSettings::Globals::Get()
{
   static Globals instance;
   return instance;
}

SpectrogramSettings::SpectrogramSettings()
{
   LoadPrefs();
}

SpectrogramSettings::SpectrogramSettings(const SpectrogramSettings &other)
   : minFreq(other.minFreq)
   , maxFreq(other.maxFreq)
   , range(other.range)
   , gain(other.gain)
   , frequencyGain(other.frequencyGain)
   , windowType(other.windowType)
   , windowSize(other.windowSize)
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   , zeroPaddingFactor(other.zeroPaddingFactor)
#endif
   , colorScheme(other.colorScheme)
   , scaleType(other.scaleType)
#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   , spectralSelection(other.spectralSelection)
#endif
   , algorithm(other.algorithm)
#ifdef EXPERIMENTAL_FFT_Y_GRID
   , fftYGrid(other.fftYGrid)
#endif
#ifdef EXPERIMENTAL_FIND_NOTES
   , fftFindNotes(other.fftFindNotes)
   , findNotesMinA(other.findNotesMinA)
   , numberOfMaxima(other.numberOfMaxima)
   , findNotesQuantize(other.findNotesQuantize)
#endif

   // Do not copy these!
   , hFFT{}
   , window{}
   , tWindow{}
   , dWindow{}
{
}

SpectrogramSettings &SpectrogramSettings::operator= (const SpectrogramSettings &other)
{
   if (this != &other) {
      minFreq = other.minFreq;
      maxFreq = other.maxFreq;
      range = other.range;
      gain = other.gain;
      frequencyGain = other.frequencyGain;
      windowType = other.windowType;
      windowSize = other.windowSize;
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
      zeroPaddingFactor = other.zeroPaddingFactor;
#endif
      colorScheme = other.colorScheme;
      scaleType = other.scaleType;
#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
      spectralSelection = other.spectralSelection;
#endif
      algorithm = other.algorithm;
#ifdef EXPERIMENTAL_FFT_Y_GRID
      fftYGrid = other.fftYGrid;
#endif
#ifdef EXPERIMENTAL_FIND_NOTES
      fftFindNotes = other.fftFindNotes;
      findNotesMinA = other.findNotesMinA;
      numberOfMaxima = other.numberOfMaxima;
      findNotesQuantize = other.findNotesQuantize;
#endif

      // Invalidate the caches
      DestroyWindows();
   }
   return *this;
}

SpectrogramSettings& SpectrogramSettings::defaults()
{
   static SpectrogramSettings instance;
   return instance;
}

//static
const std::vector< EnumValueSymbol > &SpectrogramSettings::GetScaleNames()
{
   static const std::vector< EnumValueSymbol > result{
      // Keep in correspondence with enum SpectrogramSettings::ScaleType:
      XO("Linear") ,
      XO("Logarithmic") ,
      /* i18n-hint: The name of a frequency scale in psychoacoustics */
      XO("Mel") ,
      /* i18n-hint: The name of a frequency scale in psychoacoustics, named for Heinrich Barkhausen */
      XO("Bark") ,
      /* i18n-hint: The name of a frequency scale in psychoacoustics, abbreviates Equivalent Rectangular Bandwidth */
      XO("ERB") ,
      /* i18n-hint: Time units, that is Period = 1 / Frequency */
      XO("Period") ,
   };
   return result;
}

//static
const EnumValueSymbols &SpectrogramSettings::GetColorSchemeNames()
{
   static const EnumValueSymbols result{
      // Keep in correspondence with enum SpectrogramSettings::ColorScheme:
      /* i18n-hint: New color scheme for spectrograms */
      { L"SpecColorNew",     XC("Color (default)",   "spectrum prefs") },
      /* i18n-hint: Classic color scheme(from theme) for spectrograms */
      { L"SpecColorTheme",   XC("Color (classic)",   "spectrum prefs") },
      /* i18n-hint: Grayscale color scheme for spectrograms */
      { L"SpecGrayscale",    XC("Grayscale",         "spectrum prefs") },
      /* i18n-hint: Inverse grayscale color scheme for spectrograms */
      { L"SpecInvGrayscale", XC("Inverse grayscale", "spectrum prefs") },
   };

   wxASSERT(csNumColorScheme == result.size());
   static_assert(csNumColorScheme == AColor::colorSchemes, "Broken correspondence");

   return result;
}


void SpectrogramSettings::ColorSchemeEnumSetting::Migrate(wxString &value)
{
   // Migrate old grayscale option to Color scheme choice
   bool isGrayscale = (gPrefs->Read(L"/Spectrum/Grayscale", 0L) != 0);
   if (isGrayscale && !gPrefs->Read(L"/Spectrum/ColorScheme", &value)) {
      value = GetColorSchemeNames().at(csInvGrayscale).Internal();
      Write(value);
      gPrefs->Flush();
   }
}

SpectrogramSettings::ColorSchemeEnumSetting SpectrogramSettings::colorSchemeSetting{
   L"/Spectrum/ColorScheme",
   GetColorSchemeNames(),
   csColorNew, // default to Color(New)
   { csColorNew, csColorTheme, csGrayscale, csInvGrayscale },
};


//static
const TranslatableStrings &SpectrogramSettings::GetAlgorithmNames()
{
   static const TranslatableStrings results{
      // Keep in correspondence with enum SpectrogramSettings::Algorithm:
      XO("Frequencies") ,
      /* i18n-hint: the Reassignment algorithm for spectrograms */
      XO("Reassignment") ,
      /* i18n-hint: EAC abbreviates "Enhanced Autocorrelation" */
      XO("Pitch (EAC)") ,
   };
   return results;
}

bool SpectrogramSettings::Validate(bool quiet)
{
   if (!quiet &&
      maxFreq < 100) {
      AudacityMessageBox( XO("Maximum frequency must be 100 Hz or above") );
      return false;
   }
   else
      maxFreq = std::max(100, maxFreq);

   if (!quiet &&
      minFreq < 0) {
      AudacityMessageBox( XO("Minimum frequency must be at least 0 Hz") );
      return false;
   }
   else
      minFreq = std::max(0, minFreq);

   if (!quiet &&
      maxFreq <= minFreq) {
      AudacityMessageBox( XO(
"Minimum frequency must be less than maximum frequency") );
      return false;
   }
   else
      maxFreq = std::max(1 + minFreq, maxFreq);

   if (!quiet &&
      range <= 0) {
      AudacityMessageBox( XO("The range must be at least 1 dB") );
      return false;
   }
   else
      range = std::max(1, range);

   if (!quiet &&
      frequencyGain < 0) {
      AudacityMessageBox( XO("The frequency gain cannot be negative") );
      return false;
   }
   else if (!quiet &&
      frequencyGain > 60) {
      AudacityMessageBox( XO(
"The frequency gain must be no more than 60 dB/dec") );
      return false;
   }
   else
      frequencyGain =
         std::max(0, std::min(60, frequencyGain));

   // The rest are controlled by drop-down menus so they can't go wrong
   // in the Preferences dialog, but we also come here after reading fom saved
   // preference files, which could be or from future versions.  Validate quietly.
   windowType =
      std::max(0, std::min(NumWindowFuncs() - 1, windowType));
   scaleType =
      ScaleType(std::max(0,
         std::min((int)(SpectrogramSettings::stNumScaleTypes) - 1,
            (int)(scaleType))));
   colorScheme = ColorScheme(
      std::max(0, std::min<int>(csNumColorScheme-1, colorScheme))
   );
   algorithm = Algorithm(
      std::max(0, std::min((int)(algNumAlgorithms) - 1, (int)(algorithm)))
   );
   ConvertToEnumeratedWindowSizes();
   ConvertToActualWindowSizes();

   return true;
}

void SpectrogramSettings::LoadPrefs()
{
   minFreq = gPrefs->Read(L"/Spectrum/MinFreq", 0L);

   maxFreq = gPrefs->Read(L"/Spectrum/MaxFreq", 8000L);

   range = gPrefs->Read(L"/Spectrum/Range", 80L);
   gain = gPrefs->Read(L"/Spectrum/Gain", 20L);
   frequencyGain = gPrefs->Read(L"/Spectrum/FrequencyGain", 0L);

   windowSize = gPrefs->Read(L"/Spectrum/FFTSize", 1024);

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   zeroPaddingFactor = gPrefs->Read(L"/Spectrum/ZeroPaddingFactor", 1);
#endif

   gPrefs->Read(L"/Spectrum/WindowType", &windowType, eWinFuncHann);

   colorScheme = colorSchemeSetting.ReadEnum();

   scaleType = ScaleType(gPrefs->Read(L"/Spectrum/ScaleType", 0L));

#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   spectralSelection = (gPrefs->Read(L"/Spectrum/EnableSpectralSelection", 1L) != 0);
#endif

   algorithm = Algorithm(gPrefs->Read(L"/Spectrum/Algorithm", 0L));

#ifdef EXPERIMENTAL_FFT_Y_GRID
   fftYGrid = (gPrefs->Read(L"/Spectrum/FFTYGrid", 0L) != 0);
#endif //EXPERIMENTAL_FFT_Y_GRID

#ifdef EXPERIMENTAL_FIND_NOTES
   fftFindNotes = (gPrefs->Read(L"/Spectrum/FFTFindNotes", 0L) != 0);
   findNotesMinA = gPrefs->Read(L"/Spectrum/FindNotesMinA", -30.0);
   numberOfMaxima = gPrefs->Read(L"/Spectrum/FindNotesN", 5L);
   findNotesQuantize = (gPrefs->Read(L"/Spectrum/FindNotesQuantize", 0L) != 0);
#endif //EXPERIMENTAL_FIND_NOTES

   // Enforce legal values
   Validate(true);

   InvalidateCaches();
}

void SpectrogramSettings::SavePrefs()
{
   gPrefs->Write(L"/Spectrum/MinFreq", minFreq);
   gPrefs->Write(L"/Spectrum/MaxFreq", maxFreq);

   // Nothing wrote these.  They only varied from the linear scale bounds in-session. -- PRL
   // gPrefs->Write(L"/SpectrumLog/MaxFreq", logMinFreq);
   // gPrefs->Write(L"/SpectrumLog/MinFreq", logMaxFreq);

   gPrefs->Write(L"/Spectrum/Range", range);
   gPrefs->Write(L"/Spectrum/Gain", gain);
   gPrefs->Write(L"/Spectrum/FrequencyGain", frequencyGain);

   gPrefs->Write(L"/Spectrum/FFTSize", windowSize);

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   gPrefs->Write(L"/Spectrum/ZeroPaddingFactor", zeroPaddingFactor);
#endif

   gPrefs->Write(L"/Spectrum/WindowType", windowType);

   colorSchemeSetting.WriteEnum(static_cast<ColorScheme>(colorScheme));

   gPrefs->Write(L"/Spectrum/ScaleType", (int) scaleType);

#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   gPrefs->Write(L"/Spectrum/EnableSpectralSelection", spectralSelection);
#endif

   gPrefs->Write(L"/Spectrum/Algorithm", (int) algorithm);

#ifdef EXPERIMENTAL_FFT_Y_GRID
   gPrefs->Write(L"/Spectrum/FFTYGrid", fftYGrid);
#endif //EXPERIMENTAL_FFT_Y_GRID

#ifdef EXPERIMENTAL_FIND_NOTES
   gPrefs->Write(L"/Spectrum/FFTFindNotes", fftFindNotes);
   gPrefs->Write(L"/Spectrum/FindNotesMinA", findNotesMinA);
   gPrefs->Write(L"/Spectrum/FindNotesN", numberOfMaxima);
   gPrefs->Write(L"/Spectrum/FindNotesQuantize", findNotesQuantize);
#endif //EXPERIMENTAL_FIND_NOTES
}

// This is a temporary hack until SpectrogramSettings gets fully integrated
void SpectrogramSettings::UpdatePrefs()
{
   if (minFreq == defaults().minFreq) {
      gPrefs->Read(L"/Spectrum/MinFreq", &minFreq, 0L);
   }

   if (maxFreq == defaults().maxFreq) {
      gPrefs->Read(L"/Spectrum/MaxFreq", &maxFreq, 8000L);
   }

   if (range == defaults().range) {
      gPrefs->Read(L"/Spectrum/Range", &range, 80L);
   }

   if (gain == defaults().gain) {
      gPrefs->Read(L"/Spectrum/Gain", &gain, 20L);
   }

   if (frequencyGain == defaults().frequencyGain) {
      gPrefs->Read(L"/Spectrum/FrequencyGain", &frequencyGain, 0L);
   }

   if (windowSize == defaults().windowSize) {
      gPrefs->Read(L"/Spectrum/FFTSize", &windowSize, 1024);
   }

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   if (zeroPaddingFactor == defaults().zeroPaddingFactor) {
      gPrefs->Read(L"/Spectrum/ZeroPaddingFactor", &zeroPaddingFactor, 1);
   }
#endif

   if (windowType == defaults().windowType) {
      gPrefs->Read(L"/Spectrum/WindowType", &windowType, eWinFuncHann);
   }

   if (colorScheme == defaults().colorScheme) {
      colorScheme = colorSchemeSetting.ReadEnum();
   }

   if (scaleType == defaults().scaleType) {
      int temp;
      gPrefs->Read(L"/Spectrum/ScaleType", &temp, 0L);
      scaleType = ScaleType(temp);
   }

#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   if (spectralSelection == defaults().spectralSelection) {
      int temp;
      gPrefs->Read(L"/Spectrum/EnableSpectralSelection", &temp, 1L);
      spectralSelection = (temp != 0);
   }
#endif

   if (algorithm == defaults().algorithm) {
      int temp;
      gPrefs->Read(L"/Spectrum/Algorithm", &temp, 0L);
      algorithm = Algorithm(temp);
   }

#ifdef EXPERIMENTAL_FFT_Y_GRID
   if (fftYGrid == defaults().fftYGrid) {
      int temp;
      gPrefs->Read(L"/Spectrum/FFTYGrid", &temp, 0L);
      fftYGrid = (temp != 0);
   }
#endif //EXPERIMENTAL_FFT_Y_GRID

#ifdef EXPERIMENTAL_FIND_NOTES
   if (fftFindNotes == defaults().fftFindNotes) {
      int temp;
      gPrefs->Read(L"/Spectrum/FFTFindNotes", &temp, 0L);
      fftFindNotes = (temp != 0);
   }

   if (findNotesMinA == defaults().findNotesMinA) {
      gPrefs->Read(L"/Spectrum/FindNotesMinA", &findNotesMinA, -30.0);
   }

   if (numberOfMaxima == defaults().numberOfMaxima) {
      numberOfMaxima = gPrefs->Read(L"/Spectrum/FindNotesN", &numberOfMaxima, 5L);
   }

   if (findNotesQuantize == defaults().findNotesQuantize) {
      int temp;
      gPrefs->Read(L"/Spectrum/FindNotesQuantize", &temp, 0L);
      findNotesQuantize = (temp != 0);
   }
#endif //EXPERIMENTAL_FIND_NOTES

   // Enforce legal values
   Validate(true);
}

void SpectrogramSettings::InvalidateCaches()
{
   DestroyWindows();
}

SpectrogramSettings::~SpectrogramSettings()
{
   DestroyWindows();
}

void SpectrogramSettings::DestroyWindows()
{
   hFFT.reset();
   window.reset();
   dWindow.reset();
   tWindow.reset();
}


namespace
{
   enum { WINDOW, TWINDOW, DWINDOW };
   void RecreateWindow(
      Floats &window, int which, size_t fftLen,
      size_t padding, int windowType, size_t windowSize, double &scale)
   {
      // Create the requested window function
      window = Floats{ fftLen };
      size_t ii;

      const bool extra = padding > 0;
      wxASSERT(windowSize % 2 == 0);
      if (extra)
         // For windows that do not go to 0 at the edges, this improves symmetry
         ++windowSize;
      const size_t endOfWindow = padding + windowSize;
      // Left and right padding
      for (ii = 0; ii < padding; ++ii) {
         window[ii] = 0.0;
         window[fftLen - ii - 1] = 0.0;
      }
      // Default rectangular window in the middle
      for (; ii < endOfWindow; ++ii)
         window[ii] = 1.0;
      // Overwrite middle as needed
      switch (which) {
      case WINDOW:
         NewWindowFunc(windowType, windowSize, extra, window.get() + padding);
         break;
      case TWINDOW:
         NewWindowFunc(windowType, windowSize, extra, window.get() + padding);
         {
            for (int jj = padding, multiplier = -(int)windowSize / 2; jj < (int)endOfWindow; ++jj, ++multiplier)
               window[jj] *= multiplier;
         }
         break;
      case DWINDOW:
         DerivativeOfWindowFunc(windowType, windowSize, extra, window.get() + padding);
         break;
      default:
         wxASSERT(false);
      }
      // Scale the window function to give 0dB spectrum for 0dB sine tone
      if (which == WINDOW) {
         scale = 0.0;
         for (ii = padding; ii < endOfWindow; ++ii)
            scale += window[ii];
         if (scale > 0)
            scale = 2.0 / scale;
      }
      for (ii = padding; ii < endOfWindow; ++ii)
         window[ii] *= scale;
   }
}

void SpectrogramSettings::CacheWindows() const
{
   if (hFFT == NULL || window == NULL) {

      double scale;
      auto factor = ZeroPaddingFactor();
      const auto fftLen = WindowSize() * factor;
      const auto padding = (WindowSize() * (factor - 1)) / 2;

      hFFT = GetFFT(fftLen);
      RecreateWindow(window, WINDOW, fftLen, padding, windowType, windowSize, scale);
      if (algorithm == algReassignment) {
         RecreateWindow(tWindow, TWINDOW, fftLen, padding, windowType, windowSize, scale);
         RecreateWindow(dWindow, DWINDOW, fftLen, padding, windowType, windowSize, scale);
      }
   }
}

void SpectrogramSettings::ConvertToEnumeratedWindowSizes()
{
   unsigned size;
   int logarithm;

   logarithm = -LogMinWindowSize;
   size = unsigned(windowSize);
   while (size > 1)
      size >>= 1, ++logarithm;
   windowSize = std::max(0, std::min(NumWindowSizes - 1, logarithm));

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   // Choices for zero padding begin at 1
   logarithm = 0;
   size = unsigned(zeroPaddingFactor);
   while (zeroPaddingFactor > 1)
      zeroPaddingFactor >>= 1, ++logarithm;
   zeroPaddingFactor = std::max(0,
      std::min(LogMaxWindowSize - (windowSize + LogMinWindowSize),
         logarithm
   ));
#endif
}

void SpectrogramSettings::ConvertToActualWindowSizes()
{
   windowSize = 1 << (windowSize + LogMinWindowSize);
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   zeroPaddingFactor = 1 << zeroPaddingFactor;
#endif
}

float SpectrogramSettings::findBin( float frequency, float binUnit ) const
{
   float linearBin = frequency / binUnit;
   if (linearBin < 0)
      return -1;
   else
      return linearBin;
}

size_t SpectrogramSettings::GetFFTLength() const
{
#ifndef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   return windowSize;
#else
   return windowSize * ((algorithm != algPitchEAC) ? zeroPaddingFactor : 1);
#endif
}

size_t SpectrogramSettings::NBins() const
{
   // Omit the Nyquist frequency bin
   return GetFFTLength() / 2;
}

NumberScale SpectrogramSettings::GetScale( float minFreqIn, float maxFreqIn ) const
{
   NumberScaleType type = nstLinear;

   // Don't assume the correspondence of the enums will remain direct in the future.
   // Do this switch.
   switch (scaleType) {
   default:
      wxASSERT(false);
   case stLinear:
      type = nstLinear; break;
   case stLogarithmic:
      type = nstLogarithmic; break;
   case stMel:
      type = nstMel; break;
   case stBark:
      type = nstBark; break;
   case stErb:
      type = nstErb; break;
   case stPeriod:
      type = nstPeriod; break;
   }

   return NumberScale(type, minFreqIn, maxFreqIn);
}

bool SpectrogramSettings::SpectralSelectionEnabled() const
{
#ifdef SPECTRAL_SELECTION_GLOBAL_SWITCH
   return Globals::Get().spectralSelection;
#else
   return spectralSelection;
#endif
}
