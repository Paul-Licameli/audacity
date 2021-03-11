list( APPEND SOURCES
   EditToolBar.cpp
   EditToolBar.h
)
audacity_module( mod-edit-tool-bar "${SOURCES}"
   "mod-toolbars-interface"
   "" "" )
