addlib( libflac            flac        LIBFLAC     NO    YES   "flac >= 1.3.1" "flac++ >= 1.3.1" )

if ( USE_LIBFLAC )

   list( APPEND SOURCES
      ExportFLAC.cpp
      ImportFLAC.cpp
   )

   set( LIBS
      mod-import-export-interface
      libflac
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag::libid3tag
      )
   endif()

   audacity_module( mod-flac "${SOURCES}" "${LIBS}"
      "" "" )

   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
      list( APPEND MIMETYPES
         audio/flac
         audio/x-flac
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()

endif()
