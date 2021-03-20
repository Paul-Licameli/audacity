addlib( portmidi           midi        MIDI        NO    YES   "portmidi >= 0.1" )
if (NOT USE_MIDI AND
   "EXPERIMENTAL_MIDI_OUT" IN_LIST EXPERIMENTAL_OPTIONS_LIST )
   message(FATAL_ERROR "EXPERIMENTAL_MIDI_OUT requires USE_MIDI")
endif ()

if( NOT TARGET portsmf )
   addlib( portsmf            portsmf     PORTSMF     NO    YES   "portSMF >= 0.1" )
endif()

list( APPEND SOURCES
   MIDIPlay.cpp
   NoteTrack.cpp
   NoteTrack.h
)
set( LIBS
   lib-audio-io-interface
   portaudio-v19
   portsmf
   portmidi
   PRIVATE
   wxBase
)
audacity_module( mod-note-track "${SOURCES}" "${LIBS}"
   "" "" )
