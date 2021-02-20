list( APPEND SOURCES
   Project.cpp
   Project.h
   ProjectStatus.cpp
   ProjectStatus.h
)
audacity_library( lib-project "${SOURCES}"
   "lib-registries-interface;PRIVATE;wxBase"
   "" ""
)
