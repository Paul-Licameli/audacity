do_directory(
   PRIVATE
      wxBase

   PUBLIC
      src/utility

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      AudacityException.cpp
      AudacityException.h
      FileException.cpp
      FileException.h
      InconsistencyException.cpp
      InconsistencyException.h
      UserException.cpp
      UserException.h
)
