list( APPEND SOURCES
   AudacityTextEntryDialog.cpp
   AudacityTextEntryDialog.h
   FileDialog/FileDialog.cpp
   FileDialog/FileDialog.h
   $<$<PLATFORM_ID:Windows>:
      FileDialog/win/FileDialogPrivate.cpp
      FileDialog/win/FileDialogPrivate.h
   >
   $<$<PLATFORM_ID:Darwin>:
      FileDialog/mac/FileDialogPrivate.mm
      FileDialog/mac/FileDialogPrivate.h
   >
   $<$<PLATFORM_ID:Linux,FreeBSD,CYGWIN>:
      FileDialog/gtk/FileDialogPrivate.cpp
      FileDialog/gtk/FileDialogPrivate.h
   >
   WindowAccessible.cpp
   WindowAccessible.h
   wxPanelWrapper.cpp
   wxPanelWrapper.h
)
audacity_library( lib-wx-wrappers "${SOURCES}"
   "lib-files-interface;PRIVATE;wxwidgets::wxwidgets"
   ""
   "$<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD,NetBSD,CYGWIN>:PkgConfig::GTK>"
)
