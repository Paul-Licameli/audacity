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

#include <vector>
#include "PrefsPanel.h"
#include "../tracks/playabletrack/wavetrack/ui/WaveTrackViewConstants.h"
#include "WaveformSettings.h" // for ScaleTypeValues

class BoolSetting;
class ShuttleGui;
class StringSetting;

#define TRACKS_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Tracks") }

class AUDACITY_DLL_API TracksPrefs final : public PrefsPanel
{
 public:
   TracksPrefs(wxWindow * parent, wxWindowID winid);
   ~TracksPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   wxString HelpPageName() override;

   static bool GetPinnedHeadPreference();
   static void SetPinnedHeadPreference(bool value, bool flush = false);
   
   static double GetPinnedHeadPositionPreference();
   static void SetPinnedHeadPositionPreference(double value, bool flush = false);
   
   static WaveTrackViewConstants::Display ViewModeChoice();
   static WaveformSettings::ScaleTypeValues WaveformScaleChoice();
   static WaveTrackViewConstants::SampleDisplay SampleViewChoice();
   static WaveTrackViewConstants::ZoomPresets Zoom1Choice();
   static WaveTrackViewConstants::ZoomPresets Zoom2Choice();

 private:
   void PopulateOrExchange(ShuttleGui & S) override;

   static int iPreferencePinned;
};

extern BoolSetting AudioIOPinnedHead;
extern DoubleSetting AudioIOPinnedHeadPosition; // a fraction in [ 0, 1 ]

extern BoolSetting
     TracksAutoScroll
   , TracksCollapseToHalfWave
   , TracksFitVerticallyZoomed
   , TracksShowName
;

extern StringSetting
     TracksDefaultName
;

#endif
