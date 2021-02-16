set( SOURCES
   ExportMP3.cpp
   ExportMP3.h
   ImportMP3.cpp
   MP3Prefs.cpp
)

set( LIBS
   mod-import-export-interface
)

list( APPEND LIBS libmp3lame::libmp3lame )

# Do not define these for Windows or Mac until further testing
# can be done against ExportMP3.cpp.  If either are defined, the
# build will fail.
if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
   if( "${${_OPT}use_lame}" STREQUAL "local" )
      set( DISABLE_DYNAMIC_LOADING_LAME YES )
   endif()
endif()

if ( USE_LIBMAD )

   list( APPEND SOURCES
      ImportMP3.cpp
   )

   list( APPEND LIBS
      libmad::libmad
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag::libid3tag
      )
   endif()

   if( NOT CMAKE_SYSTEM_NAME MATCHES "Windows|Darwin" )
      list( APPEND MIMETYPES
         audio/mpeg
      )
      set( MIMETYPES "${MIMETYPES}" PARENT_SCOPE )
   endif()

endif()

audacity_module( mod-mp3 "${SOURCES}" "${LIBS}"
   "" "" )
