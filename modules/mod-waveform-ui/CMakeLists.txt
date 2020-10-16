list( APPEND SOURCES
   SampleHandle.cpp
   SampleHandle.h
   WaveformPrefs.cpp
   WaveformPrefs.h
   WaveformSettings.cpp
   WaveformSettings.h
   WaveformVRulerControls.cpp
   WaveformVRulerControls.h
   WaveformVZoomHandle.cpp
   WaveformVZoomHandle.h
   WaveformView.cpp
   WaveformView.h
)
audacity_module( mod-waveform-ui "${SOURCES}" "mod-wave-track-ui-interface"
   "" "" )
