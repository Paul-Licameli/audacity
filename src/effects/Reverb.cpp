/**********************************************************************

   Audacity: A Digital Audio Editor
   Audacity(R) is copyright (c) 1999-2013 Audacity Team.
   License: GPL v2.  See License.txt.

   Reverb.cpp
   Rob Sykes, Vaughan Johnson

******************************************************************//**

\class EffectReverb
\brief A reverberation effect

*//*******************************************************************/


#include "Reverb.h"
#include "LoadEffects.h"

#include <wx/arrstr.h>

#include "../Prefs.h"
#include "../ShuttleGui.h"

#include "Reverb_libSoX.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name          Type     Key                  Def      Min      Max   Scale
static auto RoomSize = Parameter<double>(
                           L"RoomSize",      75,      0,       100,  1  );
static auto PreDelay = Parameter<double>(
                           L"Delay",         10,      0,       200,  1  );
static auto Reverberance = Parameter<double>(
                           L"Reverberance",  50,      0,       100,  1  );
static auto HfDamping = Parameter<double>(
                           L"HfDamping",     50,      0,       100,  1  );
static auto ToneLow = Parameter<double>(
                           L"ToneLow",       100,     0,       100,  1  );
static auto ToneHigh = Parameter<double>(
                           L"ToneHigh",      100,     0,       100,  1  );
static auto WetGain = Parameter<double>(
                           L"WetGain",       -1,      -20,     10,   1  );
static auto DryGain = Parameter<double>(
                           L"DryGain",       -1,      -20,     10,   1  );
static auto StereoWidth = Parameter<double>(
                           L"StereoWidth",   100,     0,       100,  1  );
static auto WetOnly = Parameter<bool>(
                           L"WetOnly",       false,   false,   true, 1  );

static const struct
{
   const TranslatableString name;
   EffectReverb::Params params;
}
FactoryPresets[] =
{
   //                         Room  Pre            Hf       Tone Tone  Wet   Dry   Stereo Wet
   // Name                    Size, Delay, Reverb, Damping, Low, High, Gain, Gain, Width, Only
   { XO("Vocal I" ),          { 70,   20,    40,     99,      100, 50,   -12,  0,    70,    false } },
   { XO("Vocal II"),          { 50,   0,     50,     99,      50,  100,  -1,   -1,   70,    false } },
   { XO("Bathroom"),          { 16,   8,     80,     0,       0,   100,  -6,   0,    100,   false } },
   { XO("Small Room Bright"), { 30,   10,    50,     50,      50,  100,  -1,   -1,   100,   false } },
   { XO("Small Room Dark"),   { 30,   10,    50,     50,      100, 0,    -1,   -1,   100,   false } },
   { XO("Medium Room"),       { 75,   10,    40,     50,      100, 70,   -1,   -1,   70,    false } },
   { XO("Large Room"),        { 85,   10,    40,     50,      100, 80,    0,   -6,   90,    false } },
   { XO("Church Hall"),       { 90,   32,    60,     50,      100, 50,    0,   -12,  100,   false } },
   { XO("Cathedral"),         { 90,   16,    90,     50,      100, 0,     0,   -20,  100,   false } },
};

struct Reverb_priv_t
{
   reverb_t reverb;
   float *dry;
   float *wet[2];
};

//
// EffectReverb
//

const ComponentInterfaceSymbol EffectReverb::Symbol
{ XO("Reverb") };

namespace{ BuiltinEffectsModule::Registration< EffectReverb > reg; }

EffectReverb::EffectReverb()
   : mParameters {
      mParams.mRoomSize,       RoomSize,
      mParams.mPreDelay,       PreDelay,
      mParams.mReverberance,   Reverberance,
      mParams.mHfDamping,      HfDamping,
      mParams.mToneLow,        ToneLow,
      mParams.mToneHigh,       ToneHigh,
      mParams.mWetGain,        WetGain,
      mParams.mDryGain,        DryGain,
      mParams.mStereoWidth,    StereoWidth,
      mParams.mWetOnly,        WetOnly,
   }
{
   Parameters().Reset();
   SetLinearEffectFlag(true);
}

EffectReverb::~EffectReverb()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectReverb::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectReverb::GetDescription()
{
   return XO("Adds ambience or a \"hall effect\"");
}

wxString EffectReverb::ManualPage()
{
   return L"Reverb";
}

// EffectDefinitionInterface implementation

EffectType EffectReverb::GetType()
{
   return EffectTypeProcess;
}

// EffectClientInterface implementation

unsigned EffectReverb::GetAudioInCount()
{
   return mParams.mStereoWidth ? 2 : 1;
}

unsigned EffectReverb::GetAudioOutCount()
{
   return mParams.mStereoWidth ? 2 : 1;
}

static size_t BLOCK = 16384;

bool EffectReverb::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames chanMap)
{
   bool isStereo = false;
   mNumChans = 1;
   if (chanMap && chanMap[0] != ChannelNameEOL && chanMap[1] == ChannelNameFrontRight)
   {
      isStereo = true;
      mNumChans = 2;
   }

   mP = (Reverb_priv_t *) calloc(sizeof(*mP), mNumChans);

   for (unsigned int i = 0; i < mNumChans; i++)
   {
      reverb_create(&mP[i].reverb,
                    mSampleRate,
                    mParams.mWetGain,
                    mParams.mRoomSize,
                    mParams.mReverberance,
                    mParams.mHfDamping,
                    mParams.mPreDelay,
                    mParams.mStereoWidth * (isStereo ? 1 : 0),
                    mParams.mToneLow,
                    mParams.mToneHigh,
                    BLOCK,
                    mP[i].wet);
   }

   return true;
}

bool EffectReverb::ProcessFinalize()
{
   for (unsigned int i = 0; i < mNumChans; i++)
   {
      reverb_delete(&mP[i].reverb);
   }

   free(mP);

   return true;
}

size_t EffectReverb::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   float *ichans[2] = {NULL, NULL};
   float *ochans[2] = {NULL, NULL};

   for (unsigned int c = 0; c < mNumChans; c++)
   {
      ichans[c] = inBlock[c];
      ochans[c] = outBlock[c];
   }
   
   float const dryMult = mParams.mWetOnly ? 0 : dB_to_linear(mParams.mDryGain);

   auto remaining = blockLen;

   while (remaining)
   {
      auto len = std::min(remaining, decltype(remaining)(BLOCK));
      for (unsigned int c = 0; c < mNumChans; c++)
      {
         // Write the input samples to the reverb fifo.  Returned value is the address of the
         // fifo buffer which contains a copy of the input samples.
         mP[c].dry = (float *) fifo_write(&mP[c].reverb.input_fifo, len, ichans[c]);
         reverb_process(&mP[c].reverb, len);
      }

      if (mNumChans == 2)
      {
         for (decltype(len) i = 0; i < len; i++)
         {
            for (int w = 0; w < 2; w++)
            {
               ochans[w][i] = dryMult *
                              mP[w].dry[i] +
                              0.5 *
                              (mP[0].wet[w][i] + mP[1].wet[w][i]);
            }
         }
      }
      else
      {
         for (decltype(len) i = 0; i < len; i++)
         {
            ochans[0][i] = dryMult * 
                           mP[0].dry[i] +
                           mP[0].wet[0][i];
         }
      }

      remaining -= len;

      for (unsigned int c = 0; c < mNumChans; c++)
      {
         ichans[c] += len;
         ochans[c] += len;
      }
   }

   return blockLen;
}

RegistryPaths EffectReverb::GetFactoryPresets()
{
   RegistryPaths names;

   for (size_t i = 0; i < WXSIZEOF(FactoryPresets); i++)
   {
      names.push_back( FactoryPresets[i].name.Translation() );
   }

   return names;
}

bool EffectReverb::LoadFactoryPreset(int id)
{
   if (id < 0 || id >= (int) WXSIZEOF(FactoryPresets))
   {
      return false;
   }

   mParams = FactoryPresets[id].params;

   if (mUIParent)
      mUIParent->TransferDataToWindow();

   return true;
}

// Effect implementation

bool EffectReverb::Startup()
{
   wxString base = L"/Effects/Reverb/";

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + L"Migrated"))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      gPrefs->Read(base + L"RoomSize", &mParams.mRoomSize, RoomSize.def);
      gPrefs->Read(base + L"Delay", &mParams.mPreDelay, PreDelay.def);
      gPrefs->Read(base + L"Reverberance", &mParams.mReverberance, Reverberance.def);
      gPrefs->Read(base + L"HfDamping", &mParams.mHfDamping, HfDamping.def);
      gPrefs->Read(base + L"ToneLow", &mParams.mToneLow, ToneLow.def);
      gPrefs->Read(base + L"ToneHigh", &mParams.mToneHigh, ToneHigh.def);
      gPrefs->Read(base + L"WetGain", &mParams.mWetGain, WetGain.def);
      gPrefs->Read(base + L"DryGain", &mParams.mDryGain, DryGain.def);
      gPrefs->Read(base + L"StereoWidth", &mParams.mStereoWidth, StereoWidth.def);
      gPrefs->Read(base + L"WetOnly", &mParams.mWetOnly, WetOnly.def);

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + L"Migrated", true);
   }

   // Load the previous user presets
   for (int i = 0; i < 10; i++)
   {
      wxString path = base + wxString::Format(L"%d/", i);
      if (gPrefs->Exists(path))
      {
         Params save = mParams;
         wxString name;

         gPrefs->Read(path + L"RoomSize", &mParams.mRoomSize, RoomSize.def);
         gPrefs->Read(path + L"Delay", &mParams.mPreDelay, PreDelay.def);
         gPrefs->Read(path + L"Reverberance", &mParams.mReverberance, Reverberance.def);
         gPrefs->Read(path + L"HfDamping", &mParams.mHfDamping, HfDamping.def);
         gPrefs->Read(path + L"ToneLow", &mParams.mToneLow, ToneLow.def);
         gPrefs->Read(path + L"ToneHigh", &mParams.mToneHigh, ToneHigh.def);
         gPrefs->Read(path + L"WetGain", &mParams.mWetGain, WetGain.def);
         gPrefs->Read(path + L"DryGain", &mParams.mDryGain, DryGain.def);
         gPrefs->Read(path + L"StereoWidth", &mParams.mStereoWidth, StereoWidth.def);
         gPrefs->Read(path + L"WetOnly", &mParams.mWetOnly, WetOnly.def);
         gPrefs->Read(path + L"name", &name, wxEmptyString);
      
         if (!name.empty())
         {
            name.Prepend(L" - ");
         }
         name.Prepend(wxString::Format(L"Settings%d", i));

         SaveUserPreset(GetUserPresetsGroup(name));

         mParams = save;
      }
   }

   return true;
}

void EffectReverb::PopulateOrExchange(ShuttleGui & S)
{
   static const struct Entry{
      const EffectParameter<double> &parameter;
      TranslatableLabel prompt;
      double Params::*target;
   } table[] = {
      { RoomSize,       XXO("&Room Size (%):"),    &Params::mRoomSize },
      { PreDelay,       XXO("&Pre-delay (ms):"),   &Params::mPreDelay },
      { Reverberance,   XXO("Rever&berance (%):"), &Params::mReverberance },
      { HfDamping,      XXO("Da&mping (%):"),      &Params::mHfDamping },
      { ToneLow,        XXO("Tone &Low (%):"),     &Params::mToneLow },
      { ToneHigh,       XXO("Tone &High (%):"),    &Params::mToneHigh },
      { WetGain,        XXO("Wet &Gain (dB):"),    &Params::mWetGain },
      { DryGain,        XXO("Dr&y Gain (dB):"),    &Params::mDryGain },
      { StereoWidth,    XXO("Stereo Wid&th (%):"), &Params::mStereoWidth },
   };

   S.AddSpace(0, 5);

   S.StartMultiColumn(3, GroupOptions{ wxEXPAND }.StretchyColumn(2));
   {
      for ( auto &entry : table ) {
         auto &param = entry.parameter;
         auto &target = mParams.*( entry.target );
         S
            .Target( target )
            .AddSpinCtrl( entry.prompt, param.def, param.max, param.min);

         S
            .Style( wxSL_HORIZONTAL )
            .Target( target )
            .AddSlider( {}, param.def, param.max, param.min );
      }
   }
   S.EndMultiColumn();

   S.StartHorizontalLay(wxCENTER, false);
   {
      S
         .Target( mParams.mWetOnly )
         .AddCheckBox(XXO("Wet O&nly"), WetOnly.def);
   }
   S.EndHorizontalLay();

   return;
}

void EffectReverb::SetTitle(const wxString & name)
{
   mUIDialog->SetTitle(
      name.empty()
         ? _("Reverb")
         : wxString::Format( _("Reverb: %s"), name )
   );
}
