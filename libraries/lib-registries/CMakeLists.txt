list( APPEND SOURCES
   AttachedVirtualFunction.h
   ClientData.cpp
   ClientData.h
   ClientDataHelpers.h
   Registrar.h
   Registry.cpp
   Registry.h
)
audacity_library( lib-registries "${SOURCES}"
   "lib-preferences-interface;lib-exceptions;PRIVATE;wxBase"
   "" ""
)
