addlib( lv2                lv2         LV2         NO    YES   "lilv-0 >= 0.24.6" "lv2 >= 1.16.0" "serd-0 >= 0.30.2" "sord-0 >= 0.16.4" "sratom-0 >= 0.6.4" "suil-0 >= 0.10.6" )

list( APPEND SOURCES
   LV2Effect.cpp
   LV2Effect.h
   LoadLV2.cpp
   LoadLV2.h
   NativeWindow.h
   lv2_external_ui.h
   zix/common.h
   zix/ring.cpp
   zix/ring.h
)
audacity_module( mod-lv2 "${SOURCES}" "Audacity;lv2"
   ""
   "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:PkgConfig::GTK>" )
