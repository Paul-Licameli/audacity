list( APPEND SOURCES
   DeviceToolBar.cpp
   DeviceToolBar.h
)
audacity_module( mod-device-tool-bar "${SOURCES}" "Audacity"
   "" "" )
