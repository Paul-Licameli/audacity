list( APPEND SOURCES
   Dither.cpp
   Dither.h
   FFT.cpp
   FFT.h
   InterpolateAudio.cpp
   InterpolateAudio.h
   Matrix.cpp
   Matrix.h
   RealFFTf.cpp
   RealFFTf.h
   SampleCount.cpp
   SampleCount.h
   SampleFormat.cpp
   SampleFormat.h
   Spectrum.cpp
   Spectrum.h
   float_cast.h
)
audacity_library( lib-math "${SOURCES}"
   "lib-preferences-interface;PRIVATE;wxBase"
   "" ""
)
