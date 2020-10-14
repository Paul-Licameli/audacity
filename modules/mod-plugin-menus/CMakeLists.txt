list( APPEND SOURCES
   PluginMenus.cpp
   PluginRegistrationDialog.cpp
   PluginRegistrationDialog.h
)
audacity_module( mod-plugin-menus "${SOURCES}" "Audacity"
   "" "" )
