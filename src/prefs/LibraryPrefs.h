/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormatPrefs.h

  Joshua Haberman
  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_FILE_FORMAT_PREFS__
#define __AUDACITY_FILE_FORMAT_PREFS__

#include <functional>
#include <wx/defs.h>

#include "PrefsPanel.h"

class wxStaticText;
class wxTextCtrl;
class ShuttleGui;

#define LIBRARY_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Library") }

class LibraryPrefs final : public PrefsPanel
{
   struct PopulatorItem;
 public:

   //! Type of function that adds to the Library preference page
   using Populator = std::function< void(ShuttleGui&) >;

   //! To be statically constructed, it registers additions to the Library preference page
   struct AUDACITY_DLL_API RegisteredControls
      : public Registry::RegisteredItem<PopulatorItem>
   {
      // Whether any controls have been registered
      static bool Any();

      RegisteredControls( const Identifier &id, Populator populator,
         const Registry::Placement &placement = { wxEmptyString, {} } );

      struct AUDACITY_DLL_API Init{ Init(); };
   };

   LibraryPrefs(wxWindow * parent, wxWindowID winid);
   ~LibraryPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   ManualPageID HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

 private:
   struct AUDACITY_DLL_API PopulatorItem final : Registry::SingleItem {
      static Registry::GroupItem &Registry();
   
      PopulatorItem(const Identifier &id, Populator populator);

      Populator mPopulator;
   };
   void Populate();
};

// Guarantees registry exists before attempts to use it
static LibraryPrefs::RegisteredControls::Init sInitRegisteredControls;

#endif
