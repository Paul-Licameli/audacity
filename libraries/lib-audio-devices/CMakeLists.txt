list( APPEND SOURCES
   AudioIOBase.cpp
   AudioIOBase.h
   AudioIOExt.cpp
   AudioIOExt.h
   DeviceChange.cpp
   DeviceChange.h
   DeviceManager.cpp
   DeviceManager.h
   Meter.cpp
   Meter.h
)
audacity_library( lib-audio-devices "${SOURCES}"
   "portaudio-v19;portmixer;lib-preferences-interface;PRIVATE;wxBase"
   "" ""
)
