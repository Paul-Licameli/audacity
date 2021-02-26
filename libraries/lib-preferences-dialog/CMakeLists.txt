list( APPEND SOURCES
   PrefsDialog.cpp
   PrefsDialog.h
   PrefsPanel.cpp
   PrefsPanel.h
)
audacity_library( lib-preferences-dialog "${SOURCES}"
   "lib-project-windows-interface;lib-help-system-interface;PRIVATE;wxwidgets::wxwidgets"
   "" ""
)
