list( APPEND SOURCES
   TimerRecordDialog.cpp
   TimerRecordDialog.h
)
audacity_module( mod-timer-record "${SOURCES}"
  "mod-import-export-interface"
   "" "" )
