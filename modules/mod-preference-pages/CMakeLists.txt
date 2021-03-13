list( APPEND SOURCES
   DevicePrefs.cpp
   DevicePrefs.h
   GUIPrefs.cpp
   GUIPrefs.h
   ImportExportPrefs.cpp
   ImportExportPrefs.h
   LibraryPrefs.cpp
   LibraryPrefs.h
   MousePrefs.cpp
   MousePrefs.h
   PlaybackPrefs.cpp
   PlaybackPrefs.h
   RecordingPrefs.cpp
   RecordingPrefs.h
   QualityPrefs.cpp
   QualityPrefs.h
   ThemePrefs.cpp
   ThemePrefs.h
   TracksBehaviorsPrefs.cpp
   TracksBehaviorsPrefs.h
   TracksPrefs.cpp
   TracksPrefs.h
   WarningsPrefs.cpp
   WarningsPrefs.h
)
audacity_module( mod-preference-pages "${SOURCES}" "Audacity"
   "" ""
)
