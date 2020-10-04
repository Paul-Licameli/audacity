list( APPEND SOURCES
   ScrubbingToolBar.cpp
   ScrubbingToolBar.h
)
audacity_module( mod-scrubbing-tool-bar "${SOURCES}" "Audacity"
   "" "" )
