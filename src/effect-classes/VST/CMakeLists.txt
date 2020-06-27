if(USE_VST)

set(
   MY_SOURCES
      VSTEffect.cpp
      VSTEffect.h
      aeffectx.h
)

if(wxIS_GTK)
   list( APPEND MY_SOURCES VSTControlGTK.cpp VSTControlGTK.h )
endif()

if(wxIS_WIN)
   list( APPEND MY_SOURCES VSTControlMSW.cpp VSTControlMSW.h )
endif()

if(wxIS_MAC)
   list( APPEND MY_SOURCES VSTControlOSX.h )
endif()

do_directory(
   INCLUDE_PRIVATE
      src/effects
      src/etc
      src/track
      src/xml

   PRIVATE
      src/utility/math
      src/utility/registries
      src/widgets
      src/widgets/DialogDefinition
      src/widgets/utility
      wxWidgets

   OBJCPP_SOURCES
      VSTControlOSX.mm

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
      Ruler.h

   SOURCES
      ${MY_SOURCES}
)

endif()
