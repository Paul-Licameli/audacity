addlib( ffmpeg             ffmpeg      FFMPEG      NO    NO    "libavcodec >= 51.53" "libavformat >= 52.12" "libavutil >= 52.66" )

if (USE_FFMPEG)

   list( APPEND SOURCES
      ExportFFmpeg.cpp
      ExportFFmpegDialogs.cpp
      ExportFFmpegDialogs.h
      FFmpeg.cpp
      FFmpeg.h
      FFmpegPrefs.cpp
      ImportFFmpeg.cpp
   )
   audacity_module( mod-ffmpeg "${SOURCES}"
      "mod-import-export-interface;ffmpeg;libsndfile"
      "" "" )

endif()

if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
   if( "${${_OPT}use_ffmpeg}" STREQUAL "linked" )
      set( DISABLE_DYNAMIC_LOADING_FFMPEG YES )
   endif()
   if( USE_FFMPEG )
      list( APPEND MIMETYPES
         audio/aac
         audio/ac3
         audio/mp4
         audio/x-ms-wma
         video/mpeg
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()
endif()
