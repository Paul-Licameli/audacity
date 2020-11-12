list( APPEND SOURCES
   TimerRecordDialog.cpp
   TimerRecordDialog.h
)
audacity_module( mod-timer-record "${SOURCES}" "Audacity"
   "" "" )
