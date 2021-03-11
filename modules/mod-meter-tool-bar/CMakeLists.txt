list( APPEND SOURCES
   MeterToolBar.cpp
   MeterToolBar.h
)
audacity_module( mod-meter-tool-bar "${SOURCES}"
   "mod-toolbars-interface"
   "" "" )
