list( APPEND SOURCES
   AppCommandEvent.cpp
   AppCommandEvent.h
   AudacityCommand.cpp
   AudacityCommand.h
   Command.cpp
   Command.h
   CommandBuilder.cpp
   CommandBuilder.h
   CommandDirectory.cpp
   CommandDirectory.h
   CommandHandler.cpp
   CommandHandler.h
   CommandMisc.h
   CommandSignature.cpp
   CommandSignature.h
   CommandType.cpp
   CommandType.h
   LoadCommands.cpp
   LoadCommands.h
   ScriptCommandRelay.cpp
   ScriptCommandRelay.h
   ShuttleAutomation.cpp
   ShuttleAutomation.h
   Validators.h
)
audacity_module( mod-scripting "${SOURCES}" "Audacity"
   "" "" )
