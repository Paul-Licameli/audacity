list( APPEND SOURCES
   TranscriptionToolBar.cpp
   TranscriptionToolBar.h
   VoiceKey.cpp
   VoiceKey.h
)
audacity_module( mod-transcription-tool-bar "${SOURCES}" "mod-note-track-interface"
   "" "" )
