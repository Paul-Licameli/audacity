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
   TracksPrefs::kZoomToFit,
   TracksPrefs::kZoomToSelection,
   TracksPrefs::kZoomDefault,
   TracksPrefs::kZoomMinutes,
   TracksPrefs::kZoomSeconds,
   TracksPrefs::kZoom5ths,
   TracksPrefs::kZoom10ths,
   TracksPrefs::kZoom20ths,
   TracksPrefs::kZoom50ths,
   TracksPrefs::kZoom100ths,
   TracksPrefs::kZoom500ths,
   TracksPrefs::kZoomMilliSeconds,
   TracksPrefs::kZoomSamples,
   TracksPrefs::kZoom4To1,
   TracksPrefs::kMaxZoom,
};

static EnumSetting< TracksPrefs::ZoomPresets > zoom1Setting{
   wxT("/GUI/ZoomPreset1Choice"),
   choicesZoom,
   2, // kZoomDefault

   // for migrating old preferences:
   enumChoicesZoom,
   wxT("/GUI/ZoomPreset1")
};

static EnumSetting< TracksPrefs::ZoomPresets > zoom2Setting{
   wxT("/GUI/ZoomPreset2Choice"),
   choicesZoom,
   13, // kZoom4To1

   // for migrating old preferences:
   enumChoicesZoom,
   wxT("/GUI/ZoomPreset2")
};

TracksPrefs::ZoomPresets TracksPrefs::Zoom1Choice()
{
   return zoom1Setting.ReadEnum();
}

TracksPrefs::ZoomPresets TracksPrefs::Zoom2Choice()
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

         {
            static Registry::OrderingPreferenceInitializer init {
               PathStart,
               { { wxT(""), wxT("Collapse,Mode,Scale,Samples") } }
            };
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
