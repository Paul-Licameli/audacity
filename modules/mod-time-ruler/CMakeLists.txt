list( APPEND SOURCES
   AdornedRulerPanel.cpp
   AdornedRulerPanel.h
   ScrubUI.cpp
   ScrubUI.h
)
audacity_module( mod-time-ruler "${SOURCES}"
   "mod-toolbars-interface"
   "" "" )
