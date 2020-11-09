list( APPEND SOURCES
   TimeTrack.cpp
   TimeTrack.h
)
audacity_module( mod-time-track "${SOURCES}"
   "Audacity"
   "" "" )
