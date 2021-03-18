list( APPEND SOURCES
   TransactionScope.cpp
   TransactionScope.h
)
audacity_library( lib-transactions "${SOURCES}"
   "lib-exceptions-interface;PRIVATE;wxBase"
   ""
   ""
)
