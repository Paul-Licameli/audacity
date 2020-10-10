/**********************************************************************

Audacity: A Digital Audio Editor

WaveformSettings.cpp

Paul Licameli

*******************************************************************//**

\class WaveformSettings
\brief Waveform settings, either for one track or as defaults.

*//*******************************************************************/


#include "WaveformSettings.h"
#include "WaveTrack.h"

#include "GUISettings.h"
#include "GUIPrefs.h"
#include "TracksPrefs.h"

#include <algorithm>

#include "Prefs.h"


WaveformSettings::Globals::Globals()
{
   LoadPrefs();
}

void WaveformSettings::Globals::SavePrefs()
{
}

void WaveformSettings::Globals::LoadPrefs()
{
}

WaveformSettings::Globals
&WaveformSettings::Globals::Get()
{
   static Globals instance;
   return instance;
}

static WaveTrack::Caches::RegisteredFactory key1{
   [](WaveTrack&){
      return std::make_unique<WaveformSettings>(WaveformSettings::defaults());
   }
};

WaveformSettings &WaveformSettings::Get( WaveTrack &track )
{
   return static_cast<WaveformSettings&>(track.WaveTrack::Caches::Get(key1));
}

const WaveformSettings &WaveformSettings::Get( const WaveTrack &track )
{
   return Get(const_cast<WaveTrack&>(track));
}

void WaveformSettings::Set(WaveTrack &track, const WaveformSettings &settings)
{
   Get(track) = settings;
}

WaveformSettings::WaveformSettings()
{
   LoadPrefs();
}

WaveformSettings::WaveformSettings(const WaveformSettings &other)
   : scaleType(other.scaleType)
   , dBRange(other.dBRange)
{
}

WaveformSettings &WaveformSettings::operator= (const WaveformSettings &other)
{
   if (this != &other) {
      scaleType = other.scaleType;
      dBRange = other.dBRange;
   }
   return *this;
}

WaveformSettings& WaveformSettings::defaults()
{
   static WaveformSettings instance;
   return instance;
}

bool WaveformSettings::Validate(bool /* quiet */)
{
   scaleType = ScaleType(
      std::max(0, std::min((int)(stNumScaleTypes) - 1, (int)(scaleType)))
   );

   ConvertToEnumeratedDBRange();
   ConvertToActualDBRange();

   return true;
}

namespace {

static EnumSetting< WaveTrackViewConstants::SampleDisplay >
sampleDisplaySetting{
   wxT("/GUI/SampleViewChoice"),
   {
      { wxT("ConnectDots"), XO("Connect dots") },
      { wxT("StemPlot"), XO("Stem plot") }
   },
   1, // StemPlot

   // for migrating old preferences:
   {
      WaveTrackViewConstants::LinearInterpolate,
      WaveTrackViewConstants::StemPlot
   },
   wxT("/GUI/SampleView")
};
}

WaveTrackViewConstants::SampleDisplay WaveformSettings::SampleViewChoice()
{
   return sampleDisplaySetting.ReadEnum();
}

namespace {
static EnumSetting< WaveformSettings::ScaleTypeValues > waveformScaleSetting{
   TracksPrefs::WaveformScaleKey(),
   {
      { XO("Linear") },
      { TracksPrefs::DBValueString(), XO("Logarithmic (dB)") },
   },

   0, // linear
   
   {
      WaveformSettings::stLinear,
      WaveformSettings::stLogarithmic,
   }
};

WaveformSettings::ScaleTypeValues WaveformScaleChoice()
{
   return waveformScaleSetting.ReadEnum();
}
}

void WaveformSettings::LoadPrefs()
{
   scaleType = WaveformScaleChoice();

   dBRange = gPrefs->Read(ENV_DB_KEY, ENV_DB_RANGE);

   // Enforce legal values
   Validate(true);

   Update();
}

void WaveformSettings::SavePrefs()
{
}

void WaveformSettings::Update()
{
}

// This is a temporary hack until WaveformSettings gets fully integrated
void WaveformSettings::UpdatePrefs()
{
   if (scaleType == defaults().scaleType) {
      scaleType = WaveformScaleChoice();
   }

   if (dBRange == defaults().dBRange){
      dBRange = gPrefs->Read(ENV_DB_KEY, ENV_DB_RANGE);
   }

   // Enforce legal values
   Validate(true);
}

void WaveformSettings::ConvertToEnumeratedDBRange()
{
   // Assumes the codes are in ascending sequence.
   wxArrayStringEx codes;
   GUIPrefs::GetRangeChoices(nullptr, &codes);
   int ii = 0;
   for (int nn = codes.size(); ii < nn; ++ii) {
      long value = 0;
      codes[ii].ToLong(&value);
      if (dBRange < value)
         break;
   }
   dBRange = std::max(0, ii - 1);
}

void WaveformSettings::ConvertToActualDBRange()
{
   wxArrayStringEx codes;
   GUIPrefs::GetRangeChoices(nullptr, &codes);
   long value = 0;
   codes[std::max(0, std::min((int)(codes.size()) - 1, dBRange))]
      .ToLong(&value);
   dBRange = (int)(value);
}

void WaveformSettings::NextLowerDBRange()
{
   ConvertToEnumeratedDBRange();
   ++dBRange;
   ConvertToActualDBRange();
}

void WaveformSettings::NextHigherDBRange()
{
   ConvertToEnumeratedDBRange();
   --dBRange;
   ConvertToActualDBRange();
}

//static
const EnumValueSymbols &WaveformSettings::GetScaleNames()
{
   static const EnumValueSymbols result{
      // Keep in correspondence with ScaleTypeValues:
      XO("Linear"),
      XO("dB"),
   };
   return result;
}

WaveformSettings::~WaveformSettings()
{
}

auto WaveformSettings::Clone() const -> PointerType
{
   return std::make_unique<WaveformSettings>(*this);
}

static WaveTrack::Caches::RegisteredFactory key2{
   [](WaveTrack&){
      return std::make_unique<WaveformSettingsCache>();
   }
};

WaveformSettingsCache &WaveformSettingsCache::Get( WaveTrack &track )
{
   return static_cast<WaveformSettingsCache&>(
      track.WaveTrack::Caches::Get(key2));
}

const WaveformSettingsCache &WaveformSettingsCache::Get( const WaveTrack &track )
{
   return Get(const_cast<WaveTrack&>(track));
}

WaveformSettingsCache::~WaveformSettingsCache() = default;

auto WaveformSettingsCache::Clone() const -> PointerType
{
   return std::make_unique<WaveformSettingsCache>(*this);
}

int WaveformSettingsCache::ZeroLevelYCoordinate(wxRect rect) const
{
   return rect.GetTop() +
      (int)((mDisplayMax / (mDisplayMax - mDisplayMin)) * rect.height);
}

// Attach things to Tracks preferences page
#include "../ShuttleGui.h"

namespace {
void AddScale( ShuttleGui &S )
{
   S.TieChoice(XXO("Default Waveform scale:"),
               waveformScaleSetting );
}

void AddSamples( ShuttleGui &S )
{
   S.TieChoice(XXO("Display &samples:"),
               sampleDisplaySetting );
}

TracksPrefs::RegisteredControls reg{ wxT("Scale"), 1u, AddScale };
TracksPrefs::RegisteredControls reg2{ wxT("Samples"), 1u, AddSamples };
}
