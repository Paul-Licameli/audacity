list( APPEND SOURCES
   AudacityLogger.cpp
   AudacityLogger.h
   FileException.cpp
   FileException.h
   FileIO.cpp
   FileIO.h
   FileNames.cpp
   FileNames.h
   PlatformCompatibility.cpp
   PlatformCompatibility.h
   TempDirectory.cpp
   TempDirectory.h
   wxFileNameWrapper.h
)
audacity_library( lib-files "${SOURCES}"
   "lib-exceptions-interface;lib-preferences-interface;PRIVATE;wxBase"
   "" ""
)
