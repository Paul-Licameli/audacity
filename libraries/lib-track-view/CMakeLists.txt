set( SOURCES
   CommonTrackPanelCell.cpp
   CommonTrackPanelCell.h
   TrackArtist.cpp
   TrackArtist.h
   TrackControls.cpp
   TrackControls.h
   TrackVRulerControls.cpp
   TrackVRulerControls.h
   TrackView.cpp
   TrackView.h
)
set( LIBRARIES
   lib-cellular-panel-interface
   lib-screen-geometry-interface
   lib-track-interface
   lib-theme-interface
   PRIVATE
   wxwidgets::wxwidgets
)
audacity_library( lib-track-view "${SOURCES}" "${LIBRARIES}"
   "" ""
)
