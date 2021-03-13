/**********************************************************************

  Audacity: A Digital Audio Editor

  TracksPrefs.h

  Brian Gunlogson
  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_TRACKS_PREFS__
#define __AUDACITY_TRACKS_PREFS__

//#include <wx/defs.h>

#include <functional>
#include <vector>
#include "prefs/PrefsPanel.h"

class ShuttleGui;

#define TRACKS_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Tracks") }

class PREFERENCE_PAGES_API TracksPrefs final : public PrefsPanel
{
   struct PopulatorItem;
 public:
   //! Type of function that adds to the Tracks preference page
   using Populator = std::function< void(ShuttleGui&) >;

   //! To be statically constructed, it registers additions to the Library preference page
   struct PREFERENCE_PAGES_API RegisteredControls
      : public Registry::RegisteredItem<PopulatorItem>
   {
      RegisteredControls( const Identifier &id,
         unsigned section, //!< 0 for checkmarks section, 1 for choices
         Populator populator,
         const Registry::Placement &placement = { wxEmptyString, {} } );

      struct PREFERENCE_PAGES_API Init{ Init(); };
   };

   // Various preset zooming levels.
   enum ZoomPresets {
      kZoomToFit = 0,
      kZoomToSelection,
      kZoomDefault,
      kZoomMinutes,
      kZoomSeconds,
      kZoom5ths,
      kZoom10ths,
      kZoom20ths,
      kZoom50ths,
      kZoom100ths,
      kZoom500ths,
      kZoomMilliSeconds,
      kZoomSamples,
      kZoom4To1,
      kMaxZoom,
   };

   TracksPrefs(wxWindow * parent, wxWindowID winid);
   ~TracksPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   ManualPageID HelpPageName() override;

   static const wxChar *WaveformScaleKey();
   static const wxChar *DBValueString();

   static ZoomPresets Zoom1Choice();
   static ZoomPresets Zoom2Choice();

 private:
   struct PREFERENCE_PAGES_API PopulatorItem final : Registry::SingleItem {
      static Registry::GroupItem &Registry();
   
      PopulatorItem(
         const Identifier &id, unsigned section, Populator populator);

      unsigned mSection;
      Populator mPopulator;
   };
   void Populate();
   void PopulateOrExchange(ShuttleGui & S) override;

   static int iPreferencePinned;
};

// Guarantees registry exists before attempts to use it
static TracksPrefs::RegisteredControls::Init sInitRegisteredControls;

#endif
