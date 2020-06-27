do_directory(
   PRIVATE
      src/utility/exceptions
      wxWidgets

   PUBLIC
      src/widgets/utility/FileDialog

   OBJCPP_SOURCES
      KeyboardCapture.cpp

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      BackedPanel.cpp
      BackedPanel.h
      FileHistory.cpp
      FileHistory.h
      FileNames.cpp
      FileNames.h
      HtmlWindow.cpp
      HtmlWindow.h
      ImageRoll.cpp
      ImageRoll.h
      Keyboard.cpp
      Keyboard.h
      KeyboardCapture.cpp
      KeyboardCapture.h
      Overlay.cpp
      Overlay.h
      OverlayPanel.cpp
      OverlayPanel.h
      ProjectStatus.cpp
      ProjectStatus.h
      WindowAccessible.cpp
      WindowAccessible.h
      valnum.cpp
      valnum.h
      wxPanelWrapper.cpp
      wxPanelWrapper.h

   SUBDIRECTORIES
      FileDialog
)
