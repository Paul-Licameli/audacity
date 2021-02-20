list( APPEND SOURCES
   CommandManagerWindowClasses.h
   KeyboardCapture.cpp
   KeyboardCapture.h
)
audacity_library( lib-keyboard-capture "${SOURCES}"
   "lib-exceptions-interface;PRIVATE;wxwidgets::wxwidgets"
   ""
   "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:PkgConfig::GTK>"
)

if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
   # These sources must be compiled as Objective C++ and excluded
   # from precompiled header usage
   set_source_files_properties(
      KeyboardCapture.cpp
      PROPERTIES
	 COMPILE_FLAGS "-x objective-c++"
	 SKIP_PRECOMPILE_HEADERS YES
   )
endif()
