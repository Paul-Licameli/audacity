list( APPEND SOURCES
   SelectionBar.cpp
   SelectionBar.h
)
audacity_module( mod-selection-tool-bar "${SOURCES}"
   "mod-toolbars-interface"
   "" "" )
