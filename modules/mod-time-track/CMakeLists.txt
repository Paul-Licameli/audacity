list( APPEND SOURCES
   TimeTrack.cpp
   TimeTrack.h
)
list( APPEND LIBRARIES
   lib-sample-track-interface
   lib-screen-geometry-interface
   lib-project-rate-interface
   PRIVATE
   wxBase
)
audacity_module( mod-time-track "${SOURCES}" "${LIBRARIES}"
   "" "" )
