list( APPEND SOURCES
   Decibels.cpp
   Decibels.h
   NumberScale.h
   SelectedRegion.cpp
   SelectedRegion.h
   ViewInfo.cpp
   ViewInfo.h
   ZoomInfo.cpp
   ZoomInfo.h
)
audacity_library( lib-screen-geometry "${SOURCES}"
   "lib-preferences-interface;lib-project-interface;lib-xml-interface;PRIVATE;wxBase"
   "" ""
)
