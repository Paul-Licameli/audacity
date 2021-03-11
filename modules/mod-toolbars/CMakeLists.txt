list( APPEND SOURCES
   Grabber.cpp
   Grabber.h
   ToolBar.cpp
   ToolBar.h
   ToolDock.cpp
   ToolDock.h
   ToolManager.cpp
   ToolManager.h
)
audacity_module( mod-toolbars "${SOURCES}" "Audacity"
   "" "" )
