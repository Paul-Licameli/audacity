if(USE_NYQUIST)

do_directory(
   INCLUDE_PRIVATE
      src/commands
      src/etc
      src/effects
      src/ondemand
      src/prefs
      src/track
      src/wavetrack

   PRIVATE
      src/utility/math
      src/utility/registries
      src/widgets/DialogDefinition
      src/widgets/etc
      src/widgets/utility
      src/widgets/CellularPanel
      src/xml
      wxWidgets
      $<$<BOOL:${USE_NYQUIST}>:libnyquist>

   PRECOMPILE
      AudioIO.h
      BlockFile.h
      Diags.h
      DirManager.h
      Envelope.h
      FFT.h
      LabelTrack.h
      Mix.h
      NoteTrack.h
      Prefs.h
      Project.h
      Sequence.h
      TimeTrack.h
      UndoManager.h
      WaveTrack.h
      ASlider.h
      ProgressDialog.h

   SOURCES
      LoadNyquist.cpp
      LoadNyquist.h
      Nyquist.cpp
      Nyquist.h
)

endif()
