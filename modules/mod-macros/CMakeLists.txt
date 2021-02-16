list( APPEND SOURCES
   BatchCommandDialog.cpp
   BatchCommandDialog.h
   BatchCommands.cpp
   BatchCommands.h
   BatchEvalCommand.cpp
   BatchEvalCommand.h
   BatchPrefs.cpp
   BatchPrefs.h
   BatchProcessDialog.cpp
   BatchProcessDialog.h
)
audacity_module( mod-macros "${SOURCES}"
   "mod-import-export-interface"
   "" "" )
