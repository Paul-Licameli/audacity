addlib( libvamp            vamp        VAMP        NO    YES   "vamp >= 2.5" "vamp-hostsdk >= 2.5" )

list( APPEND SOURCES
   LoadVamp.cpp
   LoadVamp.h
   VampEffect.cpp
   VampEffect.h
)
audacity_module( mod-vamp "${SOURCES}"
   "mod-effects-interface;libvamp;mod-label-track-interface"
   "" "" )
