list( APPEND SOURCES
   FreqWindow.cpp
   FreqWindow.h
)
audacity_module( mod-frequency-plot "${SOURCES}" "Audacity"
   "" "" )
