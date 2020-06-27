set(
   MY_SOURCES
      FormatClassifier.cpp
      FormatClassifier.h
      Import.cpp
      Import.h
      ImportForwards.h
      MultiFormatReader.cpp
      MultiFormatReader.h
      RawAudioGuess.cpp
      RawAudioGuess.h
      ImportLOF.cpp
      ImportPCM.cpp
      ImportPlugin.h
      ImportPlugin.cpp
      ImportRaw.cpp
      ImportRaw.h )

if( USE_FFMPEG )
   list( APPEND MY_SOURCES ImportFFmpeg.cpp )
endif()

if( USE_LIBFLAC )
   list( APPEND MY_SOURCES ImportFLAC.cpp )
endif()

if( USE_GSTREAMER )
   list( APPEND MY_SOURCES ImportGStreamer.cpp )
endif()

if( USE_MIDI )
   list( APPEND MY_SOURCES ImportMIDI.cpp ImportMIDI.h )
endif()

if( USE_LIBMAD )
   list( APPEND MY_SOURCES ImportMP3 )
endif()

if( USE_LIBOGG AND USE_LIBVORBIS )
   list( APPEND MY_SOURCES ImportOGG.cpp )
endif()

do_directory(
   INCLUDE_PRIVATE
      src/blockfile
      src/etc
      src/ondemand
      src/prefs
      src/track
      src/xml

   PRIVATE
      src/utility/math
      src/utility/registries
      src/widgets/DialogDefinition
      src/widgets/etc
      src/widgets/utility
      wxWidgets
      libsndfile
      $<$<BOOL:${USE_FFMPEG}>:ffmpeg>
      $<$<BOOL:${USE_LIBID3TAG}>:libid3tag>
      $<$<BOOL:${USE_LIBFLAC}>:libflac>
      $<$<BOOL:${USE_LIBMAD}>:libmad>
      $<$<BOOL:${USE_LIBVORBIS}>:libvorbis>
      $<$<BOOL:${USE_MIDI}>:portsmf>

   PRECOMPILE
      AudioIO.h
      BlockFile.h
      Diags.h
      DirManager.h
      Envelope.h
      FFT.h
      FileFormats.h
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
      ${MY_SOURCES}
)
