if ( NOT TARGET portsmf )
   addlib( portsmf            portsmf     PORTSMF     NO    YES   "portSMF >= 0.1" )
endif()

list( APPEND SOURCES
   MidiIOPrefs.cpp
   MidiIOPrefs.h
   NoteTrackAffordanceControls.cpp
   NoteTrackAffordanceControls.h
   NoteTrackButtonHandle.cpp
   NoteTrackButtonHandle.h
   NoteTrackControls.cpp
   NoteTrackControls.h
   NoteTrackDisplayData.cpp
   NoteTrackDisplayData.h
   NoteTrackMenuItems.cpp
   NoteTrackShifter.cpp
   NoteTrackSliderHandles.cpp
   NoteTrackSliderHandles.h
   NoteTrackVRulerControls.cpp
   NoteTrackVRulerControls.h
   NoteTrackVZoomHandle.cpp
   NoteTrackVZoomHandle.h
   NoteTrackView.cpp
   NoteTrackView.h
   StretchHandle.cpp
   StretchHandle.h
)
audacity_module( mod-note-track-ui "${SOURCES}"
   "mod-playable-track-ui-interface;portsmf;mod-note-track-interface"
   "" "" )
