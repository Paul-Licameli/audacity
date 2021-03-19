list( APPEND SOURCES
   TagsEditor.cpp
   TagsEditor.h
)
list( APPEND LIBRARIES
   lib-common-command-flags-interface
   lib-help-system-interface
   lib-widgets-interface
   mod-tags-interface
   wxwidgets::wxwidgets
)
audacity_module( mod-tags-ui "${SOURCES}" "${LIBRARIES}"
   "" "" )
