addlib( twolame            twolame     LIBTWOLAME  NO    YES   "twolame >= 0.3.13" )

if ( USE_LIBTWOLAME )

   list( APPEND SOURCES
      ExportMP2.cpp
   )

   set( LIBS
      mod-import-export-interface
      twolame
   )

   if ( USE_LIBID3TAG )
      list( APPEND LIBS
         libid3tag::libid3tag
      )
   endif()

   audacity_module( mod-mp2 "${SOURCES}" "${LIBS}"
      "" "" )

endif()
