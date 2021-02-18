list( APPEND SOURCES
   ProjectFileIORegistry.cpp
   ProjectFileIORegistry.h
   XMLFileReader.cpp
   XMLFileReader.h
   XMLTagHandler.cpp
   XMLTagHandler.h
   XMLWriter.cpp
   XMLWriter.h
)
audacity_library( lib-xml "${SOURCES}"
   "lib-files-interface;expat;PRIVATE;wxBase"
   "" ""
)
