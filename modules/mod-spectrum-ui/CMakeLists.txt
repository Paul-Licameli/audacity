list( APPEND SOURCES
   SpectralSelectHandle.cpp
   SpectralSelectHandle.h
   SpectrumPrefs.cpp
   SpectrumPrefs.h
   SpectrumVRulerControls.cpp
   SpectrumVRulerControls.h
   SpectrumVZoomHandle.cpp
   SpectrumVZoomHandle.h
   SpectrumView.cpp
   SpectrumView.h
   SpectrogramSettings.cpp
   SpectrogramSettings.h
)
audacity_module( mod-spectrum-ui "${SOURCES}" "Audacity"
   "" "" )
