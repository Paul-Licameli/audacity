list( APPEND SOURCES
   Screenshot.cpp
   ScreenshotCommand.cpp
   ScreenshotCommand.h
)
audacity_module( mod-screenshot "${SOURCES}" "mod-scripting-interface"
   "" "" )
