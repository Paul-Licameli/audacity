list( APPEND SOURCES
   TimeToolBar.cpp
   TimeToolBar.h
)
audacity_module( mod-time-tool-bar "${SOURCES}" "Audacity"
   "" "" )
