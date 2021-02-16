set( SOURCES
   ExportPCM.cpp
   ImportPCM.cpp
)
set( LIBRARIES
   mod-import-export-interface
   libsndfile
)
if (USE_LIBID3TAG)
   list( APPEND LIBRARIES libid3tag::libid3tag )
endif()

audacity_module( mod-pcm "${SOURCES}" "${LIBRARIES}"
   "" "" )
