do_directory(
   INCLUDE_PRIVATE
      src/commands
      src/etc
      src/xml

   PUBLIC
      src/widgets/utility/FileDialog

   PRIVATE
      src/utility/math
      src/utility/registries
      src/widgets/DialogDefinition
      src/widgets/help
      src/widgets/theme
      src/widgets/utility
      wxWidgets

   PRECOMPILE
      AColor.h
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
      AttachableScrollBar.cpp
      AttachableScrollBar.h
      Grabber.cpp
      Grabber.h
      KeyView.cpp
      KeyView.h
      Meter.cpp
      Meter.h
      MeterPanelBase.cpp
      MeterPanelBase.h
      PopupMenuTable.cpp
      PopupMenuTable.h
      Ruler.cpp
      Ruler.h

   SUBDIRECTORIES
      CellularPanel
      DialogDefinition
      etc
      prefs
      theme
      help
      utility
)
