# Handle Ladspa option
cmd_option(
   ${_OPT}use_ladspa
   "Use LADSPA plug-in support [on, off]"
   ON
)
set( USE_LADSPA ${${_OPT}use_ladspa} CACHE INTERNAL "" )

if(USE_LADSPA)
   list( APPEND SOURCES
      LadspaEffect.cpp
      LadspaEffect.h
      ladspa.h
   )
   set( LIBS
      lib-components-interface
      lib-math-interface
      lib-module-manager-interface
      lib-shuttlegui-interface
      lib-widgets-interface
      wxwidgets::wxwidgets
   )
   audacity_module( mod-ladspa "${SOURCES}" "${LIBS}"
      "" "" )
endif()
