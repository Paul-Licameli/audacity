list( APPEND SOURCES
   LabelTrack.cpp
   LabelTrack.h
)
audacity_module( mod-label-track "${SOURCES}"
   "lib-track-interface;lib-screen-geometry-interface;PRIVATE;wxBase"
   "" "" )
