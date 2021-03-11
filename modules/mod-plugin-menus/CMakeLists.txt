list( APPEND SOURCES
   PluginMenus.cpp
   PluginRegistrationDialog.cpp
   PluginRegistrationDialog.h
)
audacity_module( mod-plugin-menus "${SOURCES}"
   "mod-effects-interface;mod-toolbars-interface"
   "" "" )
