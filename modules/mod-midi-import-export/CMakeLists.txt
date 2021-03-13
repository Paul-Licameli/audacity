if ( NOT TARGET portsmf )
   addlib( portsmf            portsmf     PORTSMF     NO    YES   "portSMF >= 0.1" )
endif()

list( APPEND SOURCES
   ExportMIDI.cpp
   ImportMIDI.cpp
)
set( LIBS
   mod-import-export-interface
   portsmf
   mod-note-track-interface
   mod-preference-pages-interface
)
audacity_module( mod-midi-import-export "${SOURCES}" "${LIBS}"
   "" "" )
