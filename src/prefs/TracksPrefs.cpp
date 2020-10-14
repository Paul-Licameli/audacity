/**********************************************************************

  Audacity: A Digital Audio Editor

  TracksPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class TracksPrefs
\brief A PrefsPanel for track display and behavior properties.

*//*******************************************************************/


#include "TracksPrefs.h"

//#include <algorithm>
//#include <wx/defs.h>

#include "Prefs.h"
#include "../ShuttleGui.h"

int TracksPrefs::iPreferencePinned = -1;

namespace {
   const wxChar *PinnedHeadPreferenceKey()
   {
      return wxT("/AudioIO/PinnedHead");
   }

   bool PinnedHeadPreferenceDefault()
   {
      return false;
   }
   
   const wxChar *PinnedHeadPositionPreferenceKey()
   {
      return wxT("/AudioIO/PinnedHeadPosition");
   }

   double PinnedHeadPositionPreferenceDefault()
   {
      return 0.5;
   }
}


const wxChar *TracksPrefs::WaveformScaleKey()
{
   return wxT("/GUI/DefaultWaveformScaleChoice");
}

const wxChar *TracksPrefs::DBValueString()
{
   return wxT("dB");
}

//////////
// There is a complicated migration history here!
namespace {
   const auto key0 = wxT("/GUI/DefaultViewMode");
   const auto key1 = wxT("/GUI/DefaultViewModeNew");
   const auto key2 = wxT("/GUI/DefaultViewModeChoice");
   const auto key3 = wxT("/GUI/DefaultViewModeChoiceNew");

   const wxString obsoleteValue{ wxT("WaveformDB") };
};

class TracksViewModeEnumSetting : public ChoiceSetting {
public:
   using ChoiceSetting::ChoiceSetting;

   enum class Display : int {

      MultiView = -1, //!< "Multi" is special, not really a view type on par with the others.

      // DO NOT REORDER OLD VALUES!  Replace obsoletes with placeholders.

      Waveform = 0,
      MinDisplay = Waveform,

      obsoleteWaveformDBDisplay,

      Spectrum,

      obsolete1, // was SpectrumLogDisplay
      obsolete2, // was SpectralSelectionDisplay
      obsolete3, // was SpectralSelectionLogDisplay
      obsolete4, // was PitchDisplay

      // Add values here, and update MaxDisplay.

      MaxDisplay = Spectrum,

      NoDisplay,            // Preview track has no display
   };
   // Handle remapping of enum values from 2.1.0 and earlier
   static Display ConvertLegacyDisplayValue(int oldValue);

   void Migrate( wxString &value ) override
   {
      // Special logic for this preference which was three times migrated!

      // PRL:  Bugs 1043, 1044
      // 2.1.1 writes a NEW key for this preference, which got NEW values,
      // to avoid confusing version 2.1.0 if it reads the preference file afterwards.
      // Prefer the NEW preference key if it is present

      static const EnumValueSymbol waveformSymbol{ XO("Waveform") };
      static const EnumValueSymbol spectrumSymbol{ XO("Spectrogram") };

      Display viewMode;
      int oldMode;
      wxString newValue;
      auto stringValue =
         []( Display display ){
         switch ( display ) {
            case Display::Spectrum:
               return spectrumSymbol.Internal();
            case Display::obsoleteWaveformDBDisplay:
               return obsoleteValue;
            default:
               return waveformSymbol.Internal();
         }
      };

      if ( gPrefs->Read(key0, // The very old key
         &oldMode,
         (int)(Display::Waveform) ) ) {
         viewMode = ConvertLegacyDisplayValue(oldMode);
         newValue = stringValue( viewMode );
      }
      else if ( gPrefs->Read(key1,
         &oldMode,
         (int)(Display::Waveform) ) ) {
         viewMode = static_cast<Display>( oldMode );
         newValue = stringValue( viewMode );
      }
      else
         gPrefs->Read( key2, &newValue );

      if ( !gPrefs->Read( key3, &value ) ) {
         if (newValue == obsoleteValue) {
            newValue = waveformSymbol.Internal();
            gPrefs->Write(
               TracksPrefs::WaveformScaleKey(), TracksPrefs::DBValueString());
         }

         Write( value = newValue );
         gPrefs->Flush();
         return;
      }
   }
};

// static
auto
TracksViewModeEnumSetting::ConvertLegacyDisplayValue(int oldValue) -> Display
{
   // Remap old values.
   enum class OldValues {
      Waveform,
      WaveformDB,
      Spectrogram,
      SpectrogramLogF,
      Pitch,
   };

   Display newValue;
   switch ((OldValues)oldValue) {
   default:
   case OldValues::Waveform:
      newValue = Display::Waveform; break;
   case OldValues::WaveformDB:
      newValue = Display::obsoleteWaveformDBDisplay; break;
   case OldValues::Spectrogram:
   case OldValues::SpectrogramLogF:
   case OldValues::Pitch:
      newValue = Display::Spectrum; break;
      /*
   case SpectrogramLogF:
      newValue = WaveTrack::SpectrumLogDisplay; break;
   case Pitch:
      newValue = WaveTrack::PitchDisplay; break;
      */
   }
   return newValue;
}

static TracksViewModeEnumSetting viewModeSetting()
{
   // Do a delayed computation, so that registration of sub-view types completes
   // first
   const auto &types = WaveTrackSubViewType::All();
   auto symbols = transform_container< EnumValueSymbols >(
      types, std::mem_fn( &WaveTrackSubViewType::name ) );

   // Special entry for multi
   if (types.size() > 1) {
      symbols.push_back( WaveTrackViewConstants::MultiViewSymbol );
   }

   return {
      key3,
      symbols,
      0,
   };
}

Identifier TracksPrefs::ViewModeChoice()
{
   return viewModeSetting().Read();
}

//////////
static const std::initializer_list<EnumValueSymbol> choicesZoom{
   { wxT("FitToWidth"), XO("Fit to Width") },
   { wxT("ZoomToSelection"), XO("Zoom to Selection") },
   { wxT("ZoomDefault"), XO("Zoom Default") },
   { XO("Minutes") },
   { XO("Seconds") },
   { wxT("FifthsOfSeconds"), XO("5ths of Seconds") },
   { wxT("TenthsOfSeconds"), XO("10ths of Seconds") },
   { wxT("TwentiethsOfSeconds"), XO("20ths of Seconds") },
   { wxT("FiftiethsOfSeconds"), XO("50ths of Seconds") },
   { wxT("HundredthsOfSeconds"), XO("100ths of Seconds") },
   { wxT("FiveHundredthsOfSeconds"), XO("500ths of Seconds") },
   { XO("MilliSeconds") },
   { XO("Samples") },
   { wxT("FourPixelsPerSample"), XO("4 Pixels per Sample") },
   { wxT("MaxZoom"), XO("Max Zoom") },
};
static auto enumChoicesZoom = {
   WaveTrackViewConstants::kZoomToFit,
   WaveTrackViewConstants::kZoomToSelection,
   WaveTrackViewConstants::kZoomDefault,
   WaveTrackViewConstants::kZoomMinutes,
   WaveTrackViewConstants::kZoomSeconds,
   WaveTrackViewConstants::kZoom5ths,
   WaveTrackViewConstants::kZoom10ths,
   WaveTrackViewConstants::kZoom20ths,
   WaveTrackViewConstants::kZoom50ths,
   WaveTrackViewConstants::kZoom100ths,
   WaveTrackViewConstants::kZoom500ths,
   WaveTrackViewConstants::kZoomMilliSeconds,
   WaveTrackViewConstants::kZoomSamples,
   WaveTrackViewConstants::kZoom4To1,
   WaveTrackViewConstants::kMaxZoom,
};

static EnumSetting< WaveTrackViewConstants::ZoomPresets > zoom1Setting{
   wxT("/GUI/ZoomPreset1Choice"),
   choicesZoom,
   2, // kZoomDefault

   // for migrating old preferences:
   enumChoicesZoom,
   wxT("/GUI/ZoomPreset1")
};

static EnumSetting< WaveTrackViewConstants::ZoomPresets > zoom2Setting{
   wxT("/GUI/ZoomPreset2Choice"),
   choicesZoom,
   13, // kZoom4To1

   // for migrating old preferences:
   enumChoicesZoom,
   wxT("/GUI/ZoomPreset2")
};

WaveTrackViewConstants::ZoomPresets TracksPrefs::Zoom1Choice()
{
   return zoom1Setting.ReadEnum();
}

WaveTrackViewConstants::ZoomPresets TracksPrefs::Zoom2Choice()
{
   return zoom2Setting.ReadEnum();
}

//////////
static const auto PathStart = wxT("TracksPreferences");

Registry::GroupItem &TracksPrefs::PopulatorItem::Registry()
{
   static Registry::TransparentGroupItem<> registry{ PathStart };
   return registry;
}

TracksPrefs::PopulatorItem::PopulatorItem(
   const Identifier &id, unsigned section, Populator populator)
   : SingleItem{ id }
   , mSection{ section }
   , mPopulator{ populator }
{}

TracksPrefs::RegisteredControls::RegisteredControls(
   const Identifier &id, unsigned section, Populator populator,
   const Registry::Placement &placement )
   : RegisteredItem{
      std::make_unique< PopulatorItem >( id, section, std::move(populator) ),
      placement
   }
{}

TracksPrefs::TracksPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: "Tracks" include audio recordings but also other collections of
 * data associated with a time line, such as sequences of labels, and musical
 * notes */
:  PrefsPanel(parent, winid, XO("Tracks"))
{
   Populate();
}

TracksPrefs::~TracksPrefs()
{
}

ComponentInterfaceSymbol TracksPrefs::GetSymbol()
{
   return TRACKS_PREFS_PLUGIN_SYMBOL;
}

TranslatableString TracksPrefs::GetDescription()
{
   return XO("Preferences for Tracks");
}

ManualPageID TracksPrefs::HelpPageName()
{
   return "Tracks_Preferences";
}

void TracksPrefs::Populate()
{
   // Keep view choices and codes in proper correspondence --
   // we don't display them by increasing integer values.


   // How samples are displayed when zoomed in:


   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void TracksPrefs::PopulateOrExchange(ShuttleGui & S)
{
   using namespace Registry;
   struct MyVisitor final : Visitor {
      MyVisitor( ShuttleGui &S, unsigned section ) : S{S}, section{section}
      {
      }

      void Visit( Registry::SingleItem &item, const Path &path ) override
      {
         auto &myItem = static_cast<PopulatorItem&>(item);
         if (myItem.mSection == section)
            myItem.mPopulator(S);
      }

      ShuttleGui &S;
      unsigned section;
   };

   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("Display"));
   {
      S.TieCheckBox(XXO("Auto-&fit track height"),
                    {wxT("/GUI/TracksFitVerticallyZoomed"),
                     false});
      S.TieCheckBox(XXO("Sho&w track name as overlay"),
                  {wxT("/GUI/ShowTrackNameInWaveform"),
                   false});

      {
         TransparentGroupItem<> top{ PathStart };
         MyVisitor visitor{ S, 0u };
         Registry::Visit( visitor, &top, &PopulatorItem::Registry() );
      }

#ifdef EXPERIMENTAL_HALF_WAVE
      S.TieCheckBox(XXO("Use &half-wave display when collapsed"),
                  {wxT("/GUI/CollapseToHalfWave"),
                   false});
#endif
#ifdef SHOW_PINNED_UNPINNED_IN_PREFS
      S.TieCheckBox(XXO("&Pinned Recording/Playback head"),
         {PinnedHeadPreferenceKey(),
          PinnedHeadPreferenceDefault()});
#endif
      S.TieCheckBox(XXO("A&uto-scroll if head unpinned"),
         {wxT("/GUI/AutoScroll"),
          true});

      S.AddSpace(10);

      S.StartMultiColumn(2);
      {
#ifdef SHOW_PINNED_POSITION_IN_PREFS
         S.TieNumericTextBox(
            XXO("Pinned &head position"),
            {PinnedHeadPositionPreferenceKey(),
             PinnedHeadPositionPreferenceDefault()},
            30
         );
#endif

         S.TieChoice(XXO("Default &view mode:"),
                     viewModeSetting() );

         {
            TransparentGroupItem<> top{ PathStart };
            MyVisitor visitor{ S, 1u };
            Registry::Visit( visitor, &top, &PopulatorItem::Registry() );
         }

         S.TieTextBox(XXO("Default audio track &name:"),
                      {wxT("/GUI/TrackNames/DefaultTrackName"),
                       _("Audio Track")},
                      30);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("Zoom Toggle"));
   {
      S.StartMultiColumn(4);
      {
         S.TieChoice(XXO("Preset 1:"),
                     zoom1Setting );

         S.TieChoice(XXO("Preset 2:"),
                     zoom2Setting );
      }
   }
   S.EndStatic();
   S.EndScroller();
}

bool TracksPrefs::GetPinnedHeadPreference()
{
   // JKC: Cache this setting as it is read many times during drawing, and otherwise causes screen flicker.
   // Correct solution would be to re-write wxFileConfig to be efficient.
   if( iPreferencePinned >= 0 )
      return iPreferencePinned == 1;
   bool bResult = gPrefs->ReadBool(PinnedHeadPreferenceKey(), PinnedHeadPreferenceDefault());
   iPreferencePinned = bResult ? 1: 0;
   return bResult;
}

void TracksPrefs::SetPinnedHeadPreference(bool value, bool flush)
{
   iPreferencePinned = value ? 1 :0;
   gPrefs->Write(PinnedHeadPreferenceKey(), value);
   if(flush)
      gPrefs->Flush();
}

double TracksPrefs::GetPinnedHeadPositionPreference()
{
   auto value = gPrefs->ReadDouble(
      PinnedHeadPositionPreferenceKey(),
      PinnedHeadPositionPreferenceDefault());
   return std::max(0.0, std::min(1.0, value));
}

void TracksPrefs::SetPinnedHeadPositionPreference(double value, bool flush)
{
   value = std::max(0.0, std::min(1.0, value));
   gPrefs->Write(PinnedHeadPositionPreferenceKey(), value);
   if(flush)
      gPrefs->Flush();
}

wxString TracksPrefs::GetDefaultAudioTrackNamePreference()
{
   const auto name =
      gPrefs->Read(wxT("/GUI/TrackNames/DefaultTrackName"), wxT(""));

   if (name.empty() || ( name == "Audio Track" ))
      // When nothing was specified,
      // the default-default is whatever translation of...
      /* i18n-hint: The default name for an audio track. */
      return _("Audio Track");
   else
      return name;
}

bool TracksPrefs::Commit()
{
   // Bug 1583: Clear the caching of the preference pinned state.
   iPreferencePinned = -1;
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   // Bug 1661: Don't store the name for new tracks if the name is the
   // default in that language.
   if (GetDefaultAudioTrackNamePreference() == _("Audio Track")) {
      gPrefs->DeleteEntry(wxT("/GUI/TrackNames/DefaultTrackName"));
      gPrefs->Flush();
   }

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "Tracks",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew TracksPrefs(parent, winid);
   }
};
}

TracksPrefs::RegisteredControls::Init::Init()
{
   (void) PopulatorItem::Registry();
}
