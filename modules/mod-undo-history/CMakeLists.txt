list( APPEND SOURCES
   HistoryWindow.cpp
   HistoryWindow.h
)
audacity_module( mod-undo-history "${SOURCES}" "Audacity"
   "" "" )
