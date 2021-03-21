set( SOURCES
   Clipboard.cpp
   Clipboard.h
)
set( LIBRARIES
   lib-track-interface
   PRIVATE
   wxBase
)
audacity_library( lib-clipboard "${SOURCES}" "${LIBRARIES}"
   ""
   ""
)
