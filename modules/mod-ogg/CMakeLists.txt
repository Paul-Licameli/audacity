addlib( libogg             ogg         LIBOGG      NO    YES   "ogg >= 1.3.1" )
addlib( libvorbis          vorbis      LIBVORBIS   NO    YES   "vorbis >= 1.3.3" "vorbisenc >= 1.3.3" "vorbisfile >= 1.3.3" )

if ( USE_LIBFLAC AND USE_LIBVORBIS )

   list( APPEND SOURCES
      ExportOGG.cpp
      ImportOGG.cpp
   )

   list( APPEND LIBS
      mod-import-export-interface
      libogg
      libvorbis
   )

   audacity_module( mod-ogg "${SOURCES}" "${LIBS}"
      "" "" )

   if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
      list( APPEND MAC_RESOURCES
         "${CMAKE_CURRENT_SOURCE_DIR}/mac/resources/AudacityOGG.icns"
      )
   elseif( NOT CMAKE_SYSTEM_NAME MATCHES "Windows" )
      list( APPEND MIMETYPES
         application/ogg
         audio/x-vorbis+ogg
      )
   endif()
  
   foreach(var MIMETYPES MAC_RESOURCES)
      set( ${var} "${${var}}" PARENT_SCOPE )
   endforeach()

endif()
