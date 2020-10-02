list( APPEND SOURCES
   MixerToolBar.cpp
   MixerToolBar.h
)
audacity_module( mod-mixer-tool-bar "${SOURCES}" "Audacity"
   "" "" )
