if ( NOT TARGET portsmf )
   addlib( portsmf            portsmf     PORTSMF     NO    YES   "portSMF >= 0.1" )
endif()

list( APPEND SOURCES
   ExportMIDI.cpp
   ImportMIDI.cpp
)
audacity_module( mod-midi-import-export "${SOURCES}"
   "mod-import-export-interface;portsmf;mod-note-track-interface"
   "" "" )
