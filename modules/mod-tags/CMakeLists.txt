list( APPEND SOURCES
   Tags.cpp
   Tags.h
)
audacity_module( mod-tags "${SOURCES}"
   "lib-project-history-interface;PRIVATE;wxBase"
   "" "" )
