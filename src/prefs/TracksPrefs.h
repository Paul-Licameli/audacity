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
#include "PrefsPanel.h"
#include "../tracks/playabletrack/wavetrack/ui/WaveTrackViewConstants.h"

class ShuttleGui;

#define TRACKS_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Tracks") }

class AUDACITY_DLL_API TracksPrefs final : public PrefsPanel
{
   struct PopulatorItem;
 public:
   //! Type of function that adds to the Tracks preference page
   using Populator = std::function< void(ShuttleGui&) >;

   //! To be statically constructed, it registers additions to the Library preference page
   struct AUDACITY_DLL_API RegisteredControls
      : public Registry::RegisteredItem<PopulatorItem>
   {
      RegisteredControls( const Identifier &id,
         unsigned section, //!< 0 for checkmarks section, 1 for choices
         Populator populator,
         const Registry::Placement &placement = { wxEmptyString, {} } );

      struct AUDACITY_DLL_API Init{ Init(); };
   };

   TracksPrefs(wxWindow * parent, wxWindowID winid);
   ~TracksPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   ManualPageID HelpPageName() override;

   static bool GetPinnedHeadPreference();
   static void SetPinnedHeadPreference(bool value, bool flush = false);
   
   static double GetPinnedHeadPositionPreference();
   static void SetPinnedHeadPositionPreference(double value, bool flush = false);
   
   static wxString GetDefaultAudioTrackNamePreference();

   static const wxChar *WaveformScaleKey();
   static const wxChar *DBValueString();

   static Identifier ViewModeChoice();
   static WaveTrackViewConstants::SampleDisplay SampleViewChoice();
   static WaveTrackViewConstants::ZoomPresets Zoom1Choice();
   static WaveTrackViewConstants::ZoomPresets Zoom2Choice();

 private:
   struct AUDACITY_DLL_API PopulatorItem final : Registry::SingleItem {
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
