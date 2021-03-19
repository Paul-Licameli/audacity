set( SOURCES
   AudioIO.cpp
   AudioIO.h
   AudioIOListener.h
   PlaybackSchedule.cpp
   PlaybackSchedule.h
   ProjectAudioIO.cpp
   ProjectAudioIO.h
   RealtimeEffectManager.cpp
   RealtimeEffectManager.h
   RingBuffer.cpp
   RingBuffer.h
)
set( LIBRARIES
   lib-audio-devices-interface
   lib-project-rate-interface
   lib-project-windows-interface
   lib-sample-track-interface
   lib-screen-geometry-interface
   lib-transactions-interface
   PRIVATE
   wxBase
)
audacity_library( lib-audio-io "${SOURCES}" "${LIBRARIES}"
   "" ""
)
