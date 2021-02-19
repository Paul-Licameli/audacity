list( APPEND SOURCES
   AColor.cpp
   AColor.h
   AColorResources.h
   AllThemeResources.cpp
   AllThemeResources.h
   ClassicThemeAsCeeCode.h
   DarkThemeAsCeeCode.h
   HiContrastThemeAsCeeCode.h
   ImageManipulation.cpp
   ImageManipulation.h
   LightThemeAsCeeCode.h
   MacroMagic.h
   Theme.cpp
   Theme.h
)
# wxGraphics
audacity_library( lib-theme "${SOURCES}"
   "lib-files-interface;PRIVATE;wxwidgets::wxwidgets"
   "" ""
)
