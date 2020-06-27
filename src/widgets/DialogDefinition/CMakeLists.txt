do_directory(
   PRIVATE
      src/utility
      src/widgets/theme
      src/widgets/utility
      wxWidgets

   PRECOMPILE
      AColor.h
      Diags.h
      ImageManipulation.h
      Prefs.h

   SOURCES
      Shuttle.cpp
      Shuttle.h
      ShuttleGui.cpp
      ShuttleGui.h
      ShuttlePrefs.cpp
      ShuttlePrefs.h
      WrappedType.cpp
      WrappedType.h
)
