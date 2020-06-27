if(USE_AUDIO_UNITS AND wxIS_MAC)

do_directory(
   INCLUDE_PRIVATE
      src/etc

   PRIVATE
      src/utility
      src/widgets
      src/widgets/utility
      wxWidgets

   OBJCPP_SOURCES
      AUControl.mm

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      AUControl.h
      AudioUnitEffect.cpp
      AudioUnitEffect.h
)

endif()
