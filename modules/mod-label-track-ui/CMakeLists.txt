list( APPEND SOURCES
   DropoutDetector.cpp
   LabelDefaultClickHandle.cpp
   LabelDefaultClickHandle.h
   LabelDialog.cpp
   LabelDialog.h
   LabelGlyphHandle.cpp
   LabelGlyphHandle.h
   LabelImportExport.cpp
   LabelImportExport.h
   LabelMenus.cpp
   LabelTextHandle.cpp
   LabelTextHandle.h
   LabelTrackControls.cpp
   LabelTrackControls.h
   LabelTrackShifter.cpp
   LabelTrackVRulerControls.cpp
   LabelTrackVRulerControls.h
   LabelTrackView.cpp
   LabelTrackView.h
)
audacity_module( mod-label-track-ui "${SOURCES}"
   "mod-common-track-ui-interface;mod-import-export-interface"
   "" "" )
