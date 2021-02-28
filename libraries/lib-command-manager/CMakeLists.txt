list( APPEND SOURCES
   CommandContext.cpp
   CommandContext.h
   CommandFlag.cpp
   CommandFlag.h
   CommandFunctors.h
   CommandID.h
   CommandManager.cpp
   CommandManager.h
   CommandTargets.cpp
   CommandTargets.h
   Keyboard.cpp
   Keyboard.h
   MenuHandle.cpp
   MenuHandle.h
   Menus.cpp
   Menus.h
)
audacity_library( lib-command-manager "${SOURCES}"
   "lib-project-windows-interface;lib-xml-interface;PRIVATE;wxwidgets::wxwidgets"
   "" ""
)
