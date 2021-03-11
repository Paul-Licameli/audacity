list( APPEND SOURCES
   ToolsToolBar.cpp
   ToolsToolBar.h
)
audacity_module( mod-tools-tool-bar "${SOURCES}"
   "mod-toolbars-interface"
   "" "" )
