list( APPEND SOURCES
   MixerBoard.cpp
   MixerBoard.h
)
audacity_module( mod-mixer-board "${SOURCES}"
   "mod-note-track-interface;mod-track-panel-interface"
   "" "" )
