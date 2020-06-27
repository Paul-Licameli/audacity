do_directory( TYPE STATIC
   INCLUDE_PRIVATE
#      src/etc

   PRIVATE
      src/utility/exceptions
      wxWidgets
      expat

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      XMLFileReader.cpp
      XMLFileReader.h
      XMLTagHandler.cpp
      XMLTagHandler.h
      XMLWriter.cpp
      XMLWriter.h
      audacityproject.dtd
)
