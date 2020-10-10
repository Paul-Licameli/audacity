addlib( libnyquist         nyquist     NYQUIST     NO    YES   "" )

list( APPEND SOURCES
   LoadNyquist.cpp
   LoadNyquist.h
   Nyquist.cpp
   Nyquist.h
)
audacity_module( mod-nyquist "${SOURCES}" "mod-effects-interface;libnyquist;mod-spectrum-ui-interface"
   "" "" )
