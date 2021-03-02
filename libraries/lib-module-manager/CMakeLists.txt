list( APPEND SOURCES
   ModuleManager.cpp
   ModuleManager.h
   ModuleSettings.cpp
   ModuleSettings.h
   PluginManager.cpp
   PluginManager.h
)
audacity_library( lib-module-manager "${SOURCES}"
   "lib-files-interface;PRIVATE;wxBase"
   "" ""
)
