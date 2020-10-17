list( APPEND SOURCES
   CutlineHandle.cpp
   CutlineHandle.h
   WaveTrackAffordanceControls.cpp
   WaveTrackAffordanceControls.h
   WaveTrackControls.cpp
   WaveTrackControls.h
   WaveTrackLocation.cpp
   WaveTrackLocation.h
   WaveTrackMenuItems.cpp
   WaveTrackShifter.cpp
   WaveTrackSliderHandles.cpp
   WaveTrackSliderHandles.h
   WaveTrackVRulerControls.cpp
   WaveTrackVRulerControls.h
   WaveTrackVZoomHandle.cpp
   WaveTrackVZoomHandle.h
   WaveTrackView.cpp
   WaveTrackView.h
   WaveTrackViewConstants.cpp
   WaveTrackViewConstants.h
)
audacity_module( mod-wave-track-ui "${SOURCES}" "mod-playable-track-ui-interface"
   "" "" )
