do_directory(
   PRIVATE
      src/utility
      src/widgets/utility
      wxWidgets

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      CellularPanel.cpp
      CellularPanel.h
      HitTestResult.h
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
