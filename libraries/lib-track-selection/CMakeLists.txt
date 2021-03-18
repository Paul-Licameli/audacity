set( SOURCES
   SelectionState.cpp
   SelectionState.h
   SyncLock.cpp
   SyncLock.h
)
set( LIBRARIES
   lib-screen-geometry-interface
   lib-track-interface
   PRIVATE
   wxBase
)
audacity_library( lib-track-selection "${SOURCES}" "${LIBRARIES}"
   "" ""
)
