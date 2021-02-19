list( APPEND SOURCES
   ReadOnlyText.h
   Shuttle.cpp
   Shuttle.h
   ShuttleGui.cpp
   ShuttleGui.h
   ShuttlePrefs.cpp
   ShuttlePrefs.h
   WrappedType.cpp
   WrappedType.h
   wxTextCtrlWrapper.h
)
audacity_library( lib-shuttlegui "${SOURCES}"
   "lib-theme-interface;lib-wx-wrappers-interface;PRIVATE;wxwidgets::wxwidgets"
   "" ""
)
