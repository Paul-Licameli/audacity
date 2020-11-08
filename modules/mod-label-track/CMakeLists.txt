list( APPEND SOURCES
   LabelTrack.cpp
   LabelTrack.h
)
audacity_module( mod-label-track "${SOURCES}"
   "Audacity"
   "" "" )
