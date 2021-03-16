list( APPEND SOURCES
   BackgroundCell.cpp
   BackgroundCell.h
   TrackPanel.cpp
   TrackPanel.h
   TrackPanelResizeHandle.cpp
   TrackPanelResizeHandle.h
   TrackPanelResizerCell.cpp
   TrackPanelResizerCell.h
)
audacity_module( mod-track-panel "${SOURCES}"
   "Audacity"
   "" "" )
