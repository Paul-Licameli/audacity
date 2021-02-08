list( APPEND SOURCES
   PlayIndicatorOverlay.cpp
   PlayIndicatorOverlay.h
)
audacity_module( mod-play-indicator-overlay "${SOURCES}"
   "mod-label-track-interface;mod-time-ruler-interface"
   "" "" )
