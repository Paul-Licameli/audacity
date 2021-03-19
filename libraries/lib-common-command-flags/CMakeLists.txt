set( SOURCES
   CommonCommandFlags.cpp
   CommonCommandFlags.h
)
set( LIBRARIES
   lib-audio-io-interface
   lib-command-manager-interface
   lib-project-history-interface
   lib-track-selection-interface
   lib-wave-track-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-common-command-flags "${SOURCES}" "${LIBRARIES}"
   "" ""
)
