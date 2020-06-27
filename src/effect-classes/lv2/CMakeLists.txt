if(USE_LV2)

do_directory(
   INCLUDE_PRIVATE
      src/etc

   PRIVATE
      src/utility/math
      src/widgets
      src/widgets/DialogDefinition
      src/widgets/utility
      wxWidgets
      $<$<BOOL:${USE_LV2}>:lv2>

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
      LV2Effect.cpp
      LV2Effect.h
      LoadLV2.cpp
      LoadLV2.h
      NativeWindow.h
      lv2_external_ui.h
      zix/common.h
      zix/ring.cpp
      zix/ring.h
)

endif()
