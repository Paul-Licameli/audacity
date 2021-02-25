list( APPEND SOURCES
   HelpSystem.cpp
   HelpSystem.h
   HelpText.cpp
   HelpText.h
   HtmlWindow.cpp
   HtmlWindow.h
)
audacity_library( lib-help-system "${SOURCES}"
   "lib-shuttlegui;PRIVATE;wxwidgets::wxwidgets"
   ""
   ""
)
