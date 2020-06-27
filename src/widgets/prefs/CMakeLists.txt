do_directory(
   PRIVATE
      src/utility
      src/widgets/DialogDefinition
      src/widgets/help
      src/widgets/utility
      wxWidgets

   PUBLIC
      src/utility/registries

   PRECOMPILE
      Diags.h
      Prefs.h

   SOURCES
      LangChoice
      Languages
      PrefsDialog
      PrefsPanel
)
