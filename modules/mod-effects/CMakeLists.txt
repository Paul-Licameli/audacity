list( APPEND SOURCES
   Effect.cpp
   Effect.h
   EffectManager.cpp
   EffectManager.h
   EffectUI.cpp
   EffectUI.h
   EffectsPrefs.cpp
   EffectsPrefs.h
   Generator.cpp
   Generator.h
   LoadEffects.cpp
   LoadEffects.h
   ShuttleGetDefinition.cpp
   ShuttleGetDefinition.h
   SimpleMono.cpp
   SimpleMono.h
   TwoPassSimpleMono.cpp
   TwoPassSimpleMono.h
)
audacity_module( mod-effects "${SOURCES}" "mod-label-track-interface"
   "" "" )

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
   # These sources must be compiled as Objective C++ and excluded
   # from precompiled header usage
   set_source_files_properties(
      EffectUI.cpp
      PROPERTIES
         COMPILE_FLAGS "-x objective-c++"
         SKIP_PRECOMPILE_HEADERS YES
   )
endif()
