if(USE_LADSPA)

do_directory(
   INCLUDE_PRIVATE
      src/etc

   PRIVATE
      src/utility/math
      src/widgets
      src/widgets/DialogDefinition
      src/widgets/utility
      wxWidgets

   PRECOMPILE
      AudioIO.h
      BlockFile.h
      Diags.h
      FFT.h
      Mix.h
      Prefs.h
      ASlider.h
      ProgressDialog.h
      Ruler.h

   SOURCES
      LadspaEffect.cpp
      LadspaEffect.h
      ladspa.h
)

endif()
