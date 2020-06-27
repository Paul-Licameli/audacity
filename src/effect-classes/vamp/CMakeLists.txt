if(USE_VAMP)

do_directory(
   INCLUDE_PRIVATE
      src/effects
      src/etc
      src/track

   PRIVATE
      src/utility/math
      src/utility/registries
      src/widgets/DialogDefinition
      src/widgets/utility
      src/xml
      wxWidgets
      $<$<BOOL:${USE_VAMP}>:libvamp>

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

   SOURCES
      LoadVamp.cpp
      LoadVamp.h
      VampEffect.cpp
      VampEffect.h
)

endif()
