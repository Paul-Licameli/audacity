set( SOURCES
   ActiveProject.cpp
   ActiveProject.h
)
set( LIBRARIES
   lib-keyboard-capture-interface
   lib-project-windows-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-active-project "${SOURCES}" "${LIBRARIES}"
   "" ""
)
