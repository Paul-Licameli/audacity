list( APPEND SOURCES
   ProjectWindowBase.cpp
   ProjectWindowBase.h
   ProjectWindows.cpp
   ProjectWindows.h
)
audacity_library( lib-project-windows "${SOURCES}"
   "lib-project-interface;lib-cellular-panel-interface;PRIVATE;wxBase"
   "" ""
)
