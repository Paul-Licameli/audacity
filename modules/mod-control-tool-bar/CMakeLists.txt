list( APPEND SOURCES
   ControlToolBar.cpp
   ControlToolBar.h
)
audacity_module( mod-control-tool-bar "${SOURCES}"
   "mod-toolbars-interface"
   "" "" )
