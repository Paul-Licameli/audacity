list( APPEND SOURCES
   BackedPanel.cpp
   BackedPanel.h
   CellularPanel.cpp
   CellularPanel.h
   HitTestResult.h
   Overlay.cpp
   Overlay.h
   OverlayPanel.cpp
   OverlayPanel.h
   RefreshCode.h
   TrackPanelCell.cpp
   TrackPanelCell.h
   TrackPanelDrawable.cpp
   TrackPanelDrawable.h
   TrackPanelDrawingContext.h
   TrackPanelMouseEvent.h
   UIHandle.cpp
   UIHandle.h
)
audacity_library( lib-cellular-panel "${SOURCES}"
   "lib-wx-wrappers-interface;lib-keyboard-capture-interface;PRIVATE;wxwidgets::wxwidgets"
   "" ""
)
