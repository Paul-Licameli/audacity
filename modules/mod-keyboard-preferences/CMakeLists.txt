list( APPEND SOURCES
   KeyConfigPrefs.cpp
   KeyConfigPrefs.h
   KeyView.cpp
   KeyView.h
)
audacity_module( mod-keyboard-preferences "${SOURCES}" "Audacity"
   "" "" )
