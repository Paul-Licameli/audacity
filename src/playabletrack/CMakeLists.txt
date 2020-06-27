do_directory(
   INCLUDE_PRIVATE
      src/commands
      src/etc
      src/track
      src/wavetrack
      src/xml

   PRIVATE
      src/utility/math
      src/utility/registries
      src/widgets/theme
      src/widgets/utility
      src/widgets/CellularPanel
      wxWidgets

   PRECOMPILE
      AColor.h
      AudioIO.h
      Diags.h
      DirManager.h
      Envelope.h
      FFT.h
      ImageManipulation.h
      LabelTrack.h
      Mix.h
      NoteTrack.h
      Prefs.h
      Project.h
      TimeTrack.h
      WaveTrack.h

   SOURCES
      PlayableTrackButtonHandles.cpp
      PlayableTrackButtonHandles.h
      PlayableTrackControls.cpp
      PlayableTrackControls.h
)

