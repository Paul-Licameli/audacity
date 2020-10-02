if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )

list( APPEND SOURCES
   AUControl.h
   AUControl.mm
   AudioUnitEffect.cpp
   AudioUnitEffect.h
)
audacity_module( mod-audiounits "${SOURCES}" "Audacity"
   "" "" )

set_source_files_properties(
   AUControl.mm
   PROPERTIES
      COMPILE_FLAGS "-x objective-c++"
      SKIP_PRECOMPILE_HEADERS YES
)

endif()
