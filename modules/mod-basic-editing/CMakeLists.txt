list( APPEND SOURCES
   EditUtilities.cpp
   EditUtilities.h
)
audacity_module( mod-basic-editing "${SOURCES}"
   "Audacity"
   "" "" )
