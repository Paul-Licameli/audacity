list( APPEND SOURCES
   PlayableTrackButtonHandles.cpp
   PlayableTrackButtonHandles.h
   PlayableTrackControls.cpp
   PlayableTrackControls.h
)
audacity_module( mod-playable-track-ui "${SOURCES}" "mod-common-track-ui-interface"
   "" "" )
