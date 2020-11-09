list( APPEND SOURCES
   TimeTrackControls.cpp
   TimeTrackControls.h
   TimeTrackMenuItems.cpp
   TimeTrackVRulerControls.cpp
   TimeTrackVRulerControls.h
   TimeTrackVZoomHandle.cpp
   TimeTrackVZoomHandle.h
   TimeTrackView.cpp
   TimeTrackView.h
)
audacity_module( mod-time-track-ui "${SOURCES}"
   "mod-common-track-ui-interface;mod-time-track-interface"
   "" "" )
