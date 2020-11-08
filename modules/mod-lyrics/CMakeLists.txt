list( APPEND SOURCES
   Lyrics.cpp
   Lyrics.h
   LyricsWindow.cpp
   LyricsWindow.h
)
audacity_module( mod-lyrics "${SOURCES}" "mod-label-track-interface"
   "" "" )
