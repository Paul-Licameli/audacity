/**********************************************************************

  Audacity: A Digital Audio Editor

  ImportExportPrefs.h

  Joshua Haberman
  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_IMPORT_EXPORT_PREFS__
#define __AUDACITY_IMPORT_EXPORT_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

class ShuttleGui;

#define IMPORT_EXPORT_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("IMPORT EXPORT") }

template< typename Enum > class EnumSetting;

class PREFERENCE_PAGES_API ImportExportPrefs final : public PrefsPanel
{
   struct PopulatorItem;
 public:

   //! Type of function that adds to the Library preference page
   using Populator = std::function< void(ShuttleGui&) >;

   //! To be statically constructed, it registers additions to the Library preference page
   struct PREFERENCE_PAGES_API RegisteredControls
      : public Registry::RegisteredItem<PopulatorItem>
   {
      // Whether any controls have been registered
      static bool Any();

      RegisteredControls( const Identifier &id, Populator populator,
         const Registry::Placement &placement = { wxEmptyString, {} } );

      struct PREFERENCE_PAGES_API Init{ Init(); };
   };

   static EnumSetting< bool > ExportDownMixSetting;

   ImportExportPrefs(wxWindow * parent, wxWindowID winid);
   ~ImportExportPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   ManualPageID HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;

 private:
   struct PREFERENCE_PAGES_API PopulatorItem final : Registry::SingleItem {
      static Registry::GroupItem &Registry();
   
      PopulatorItem(const Identifier &id, Populator populator);

      Populator mPopulator;
   };
   void Populate();
};

// Guarantees registry exists before attempts to use it
static ImportExportPrefs::RegisteredControls::Init
   sInitRegisteredImpExpControls;

#endif
