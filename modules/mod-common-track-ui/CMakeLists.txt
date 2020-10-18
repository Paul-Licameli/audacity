list( APPEND SOURCES
   AffordanceHandle.cpp
   AffordanceHandle.h
   ButtonHandle.cpp
   ButtonHandle.h
   CommonTrackControls.cpp
   CommonTrackControls.h
   CommonTrackView.cpp
   CommonTrackView.h
   EnvelopeEditor.cpp
   EnvelopeEditor.h
   EnvelopeHandle.cpp
   EnvelopeHandle.h
   SelectHandle.cpp
   SelectHandle.h
   SliderHandle.cpp
   SliderHandle.h
   TimeShiftHandle.cpp
   TimeShiftHandle.h
   TrackArt.cpp
   TrackArt.h
   TrackButtonHandles.cpp
   TrackButtonHandles.h
   TrackInfo.cpp
   TrackInfo.h
   TrackSelectHandle.cpp
   TrackSelectHandle.h
   ZoomHandle.cpp
   ZoomHandle.h
)
audacity_module( mod-common-track-ui "${SOURCES}" "Audacity"
   "" "" )
