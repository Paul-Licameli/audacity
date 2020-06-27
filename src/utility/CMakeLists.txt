do_directory(
   PRIVATE
      wxBase

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      AudacityMessageBox.cpp
      AudacityMessageBox.h
      Diags.cpp
      Diags.h
      FileIO.cpp
      FileIO.h
      GUISettings.h
      Internat.cpp
      Internat.h
      MemoryX.h
      ODTaskThread.cpp
      ODTaskThread.h
      PitchName.cpp
      PitchName.h
      PlatformCompatibility.cpp
      PlatformCompatibility.h
      Prefs.cpp
      Prefs.h
      Profiler.cpp
      Profiler.h
      numformatter.cpp
      numformatter.h
      wxFileNameWrapper.h

   SUBDIRECTORIES
      exceptions
      math
)
