do_directory( TYPE STATIC
   PRIVATE
      wxBase
      libsoxr

   PUBLIC
      src/utility

   PRECOMPILE
      Diags.h
      FFT.h
      Prefs.h

   SOURCES
      Biquad.cpp
      Biquad.h
      Dither.cpp
      Dither.h
      EBUR128.cpp
      EBUR128.h
      FFT.cpp
      FFT.h
      InterpolateAudio.cpp
      InterpolateAudio.h
      Matrix.cpp
      Matrix.h
      RealFFTf.cpp
      RealFFTf.h
      RealFFTf48x.cpp
      RealFFTf48x.h
      Resample.cpp
      Resample.h
      SampleFormat.cpp
      SampleFormat.h
      SpecPowerMeter.cpp
      SpecPowerMeter.h
      Spectrum.cpp
      Spectrum.h
      SseMathFuncs.cpp
      SseMathFuncs.h
      float_cast.h
)
