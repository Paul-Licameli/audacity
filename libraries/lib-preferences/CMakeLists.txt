list( APPEND SOURCES
   FileConfig.cpp
   FileConfig.h
   Prefs.cpp
   Prefs.h
)
audacity_library( lib-preferences "${SOURCES}"
   "lib-utility-interface;lib-basic-ui-interface;lib-components-interface;PRIVATE;wxBase"
   "" ""
)
