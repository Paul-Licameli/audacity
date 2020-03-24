/**********************************************************************

  Audacity: A Digital Audio Editor

  Distortion.cpp

  Steve Daulton

  // TODO: Add a graph display of the waveshaper equation.
  // TODO: Allow the user to draw the graph.

******************************************************************//**

\class EffectDistortion
\brief A WaveShaper distortion effect.

*//*******************************************************************/


#include "Distortion.h"
#include "LoadEffects.h"

#include <cmath>
#include <algorithm>
#define _USE_MATH_DEFINES

// Belt and braces
#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923132169163975 
#endif

#include <wx/choice.h>
#include <wx/valgen.h>
#include <wx/log.h>
#include <wx/simplebook.h>
#include <wx/slider.h>
#include <wx/stattext.h>

#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../widgets/valnum.h"

static const EnumValueSymbol kTableTypeStrings[nTableTypes] =
{
   { XO("Hard Clipping") },
   { XO("Soft Clipping") },
   { XO("Soft Overdrive") },
   { XO("Medium Overdrive") },
   { XO("Hard Overdrive") },
   { XO("Cubic Curve (odd harmonics)") },
   { XO("Even Harmonics") },
   { XO("Expand and Compress") },
   { XO("Leveller") },
   { XO("Rectifier Distortion") },
   { XO("Hard Limiter 1413") }
};

struct EffectDistortion::UISpec {
   TranslatableString thresholdName, noiseFloorName, param1Name, param2Name, repeatsName;
   bool dcBlockEnabled;
};

static const EffectDistortion::UISpec specs[] = {
   // kHardClip
   { XO("Clipping level"), {}, XO("Drive"), XO("Make-up Gain"), {}, false },
   // kSoftClip
   { XO("Clipping threshold"), {}, XO("Hardness"), XO("Make-up Gain"), {}, false },
   // kHalfSinCurve
   { {}, {}, XO("Distortion amount"), XO("Output level"), {}, false },
   // kExpCurve
   { {}, {}, XO("Distortion amount"), XO("Output level"), {}, false },
   // kLogCurve
   { {}, {}, XO("Distortion amount"), XO("Output level"), {}, false },
   // kCubic
   { {}, {}, XO("Distortion amount"), XO("Output level"), XO("Repeat processing"), false },
   // kEvenHarmonics
   { {}, {}, XO("Distortion amount"), XO("Harmonic brightness"), {}, true },
   // kSinCurve
   { {}, {}, XO("Distortion amount"), XO("Output level"), {}, false },
   // kLeveller
   { {}, XO("Noise Floor"), XO("Levelling fine adjustment"), {}, XO("Degree of Levelling"), false },
   // kRectifier
   { {}, {}, XO("Distortion amount"), {}, {}, true },
   // kHardLimiter
   { XO("dB Limit"), {}, XO("Wet level"), XO("Residual level"), {}, false },
};

// Define keys, defaults, minimums, and maximums for the effect parameters
// (Note: 'Repeats' is the total number of times the effect is applied.)
//
//     Name             Type     Key                   Def     Min      Max                 Scale
static auto TableTypeIndx = EnumParameter(
                           L"Type",           0,       0,      nTableTypes-1,    1, kTableTypeStrings, nTableTypes    );
static auto DCBlock = Parameter<bool>(
                           L"DC Block",      false,   false,   true,                1    );
static auto Threshold_dB = Parameter<double>(
                           L"Threshold dB",  -6.0,  -100.0,     0.0,             1000.0f );
static auto NoiseFloor = Parameter<double>(
                           L"Noise Floor",   -70.0,  -80.0,   -20.0,                1    );
static auto Param1 = Parameter<double>(
                           L"Parameter 1",    50.0,    0.0,   100.0,                1    );
static auto Param2 = Parameter<double>(
                           L"Parameter 2",    50.0,    0.0,   100.0,                1    );
static auto Repeats = Parameter<int>(
                           L"Repeats",        1,       0,       5,                  1    );

// How many samples are processed before recomputing the lookup table again
#define skipsamples 1000

const double MIN_Threshold_Linear DB_TO_LINEAR(Threshold_dB.min);

static const struct
{
   const TranslatableString name;
   int mTableChoiceIndx;
   EffectDistortion::Params params;
}
FactoryPresets[] =
{
   //                                           Table    DCBlock  threshold   floor       Param1   Param2   Repeats
   // Defaults:                                   0       false   -6.0       -70.0(off)     50.0     50.0     1
   //
   // xgettext:no-c-format
   { XO("Hard clip -12dB, 80% make-up gain"),     0, {        0,      -12.0,      -70.0,      0.0,     80.0,    0 } },
   // xgettext:no-c-format
   { XO("Soft clip -12dB, 80% make-up gain"),     1, {        0,      -12.0,      -70.0,      50.0,    80.0,    0 } },
   { XO("Fuzz Box"),                              1, {        0,      -30.0,      -70.0,      80.0,    80.0,    0 } },
   { XO("Walkie-talkie"),                         1, {        0,      -50.0,      -70.0,      60.0,    80.0,    0 } },
   { XO("Blues drive sustain"),                   2, {        0,       -6.0,      -70.0,      30.0,    80.0,    0 } },
   { XO("Light Crunch Overdrive"),                3, {        0,       -6.0,      -70.0,      20.0,    80.0,    0 } },
   { XO("Heavy Overdrive"),                       4, {        0,       -6.0,      -70.0,      90.0,    80.0,    0 } },
   { XO("3rd Harmonic (Perfect Fifth)"),          5, {        0,       -6.0,      -70.0,     100.0,    60.0,    0 } },
   { XO("Valve Overdrive"),                       6, {        1,       -6.0,      -70.0,      30.0,    40.0,    0 } },
   { XO("2nd Harmonic (Octave)"),                 6, {        1,       -6.0,      -70.0,      50.0,     0.0,    0 } },
   { XO("Gated Expansion Distortion"),            7, {        0,       -6.0,      -70.0,      30.0,    80.0,    0 } },
   { XO("Leveller, Light, -70dB noise floor"),    8, {        0,       -6.0,      -70.0,       0.0,    50.0,    1 } },
   { XO("Leveller, Moderate, -70dB noise floor"), 8, {        0,       -6.0,      -70.0,       0.0,    50.0,    2 } },
   { XO("Leveller, Heavy, -70dB noise floor"),    8, {        0,       -6.0,      -70.0,       0.0,    50.0,    3 } },
   { XO("Leveller, Heavier, -70dB noise floor"),  8, {        0,       -6.0,      -70.0,       0.0,    50.0,    4 } },
   { XO("Leveller, Heaviest, -70dB noise floor"), 8, {        0,       -6.0,      -70.0,       0.0,    50.0,    5 } },
   { XO("Half-wave Rectifier"),                   9, {        0,       -6.0,      -70.0,      50.0,    50.0,    0 } },
   { XO("Full-wave Rectifier"),                   9, {        0,       -6.0,      -70.0,     100.0,    50.0,    0 } },
   { XO("Full-wave Rectifier (DC blocked)"),      9, {        1,       -6.0,      -70.0,     100.0,    50.0,    0 } },
   { XO("Percussion Limiter"),                    10, {       0,      -12.0,      -70.0,     100.0,    30.0,    0 } },
};

const TranslatableString defaultLabel[] = {
   XO("Upper Threshold"),
   XO("Noise Floor"),
   XO("Parameter 1"),
   XO("Parameter 2"),
   XO("Number of repeats"),
};

static const TranslatableString defaultSuffix[] = {
   /* i18n-hint: Control range. */
   XO("(-100 to 0 dB):"),
   /* i18n-hint: Control range. */
   XO("(-80 to -20 dB):"),
   /* i18n-hint: Control range. */
   XO("(0 to 100):"),
   /* i18n-hint: Control range. */
   XO("(0 to 100):"),
   /* i18n-hint: Control range. */
   XO("(0 to 5):"),
};

//
// EffectDistortion
//

const ComponentInterfaceSymbol EffectDistortion::Symbol
{ XO("Distortion") };

namespace{ BuiltinEffectsModule::Registration< EffectDistortion > reg; }

BEGIN_EVENT_TABLE(EffectDistortion, wxEvtHandler)
   EVT_CHOICE(ID_Type, EffectDistortion::OnTypeChoice)
   EVT_TEXT(ID_Threshold, EffectDistortion::OnThresholdText)
   EVT_SLIDER(ID_Threshold, EffectDistortion::OnThresholdSlider)
   EVT_TEXT(ID_NoiseFloor, EffectDistortion::OnNoiseFloorText)
   EVT_SLIDER(ID_NoiseFloor, EffectDistortion::OnNoiseFloorSlider)
   EVT_TEXT(ID_Param1, EffectDistortion::OnParam1Text)
   EVT_SLIDER(ID_Param1, EffectDistortion::OnParam1Slider)
   EVT_TEXT(ID_Param2, EffectDistortion::OnParam2Text)
   EVT_SLIDER(ID_Param2, EffectDistortion::OnParam2Slider)
   EVT_TEXT(ID_Repeats, EffectDistortion::OnRepeatsText)
   EVT_SLIDER(ID_Repeats, EffectDistortion::OnRepeatsSlider)
END_EVENT_TABLE()

EffectDistortion::EffectDistortion()
   : mParameters{
      mTableChoiceIndx,       TableTypeIndx,
      mParams.mDCBlock,       DCBlock,
      mParams.mThreshold_dB,  Threshold_dB,
      mParams.mNoiseFloor,    NoiseFloor,
      mParams.mParam1,        Param1,
      mParams.mParam2,        Param2,
      mParams.mRepeats,       Repeats,
   }
{
   Parameters().Reset();
   wxASSERT(nTableTypes == WXSIZEOF(kTableTypeStrings));
   mMakeupGain = 1.0;

   SetLinearEffectFlag(false);
}

EffectDistortion::~EffectDistortion()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectDistortion::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectDistortion::GetDescription()
{
   return XO("Waveshaping distortion effect");
}

wxString EffectDistortion::ManualPage()
{
   return L"Distortion";
}

// EffectDefinitionInterface implementation

EffectType EffectDistortion::GetType()
{
   return EffectTypeProcess;
}

bool EffectDistortion::SupportsRealtime()
{
#if defined(EXPERIMENTAL_REALTIME_AUDACITY_EFFECTS)
   return true;
#else
   return false;
#endif
}

// EffectClientInterface implementation

unsigned EffectDistortion::GetAudioInCount()
{
   return 1;
}

unsigned EffectDistortion::GetAudioOutCount()
{
   return 1;
}

bool EffectDistortion::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   InstanceInit(mMaster, mSampleRate);
   return true;
}

size_t EffectDistortion::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return InstanceProcess(mMaster, inBlock, outBlock, blockLen);
}

bool EffectDistortion::RealtimeInitialize()
{
   SetBlockSize(512);

   mSlaves.clear();

   return true;
}

bool EffectDistortion::RealtimeAddProcessor(unsigned WXUNUSED(numChannels), float sampleRate)
{
   EffectDistortionState slave;

   InstanceInit(slave, sampleRate);

   mSlaves.push_back(slave);

   return true;
}

bool EffectDistortion::RealtimeFinalize()
{
   mSlaves.clear();

   return true;
}

size_t EffectDistortion::RealtimeProcess(int group,
                                              float **inbuf,
                                              float **outbuf,
                                              size_t numSamples)
{

   return InstanceProcess(mSlaves[group], inbuf, outbuf, numSamples);
}

RegistryPaths EffectDistortion::GetFactoryPresets()
{
   RegistryPaths names;

   for (size_t i = 0; i < WXSIZEOF(FactoryPresets); i++)
   {
      names.push_back( FactoryPresets[i].name.Translation() );
   }

   return names;
}

bool EffectDistortion::LoadFactoryPreset(int id)
{
   if (id < 0 || id >= (int) WXSIZEOF(FactoryPresets))
   {
      return false;
   }

   mTableChoiceIndx = FactoryPresets[id].mTableChoiceIndx;
   mParams = FactoryPresets[id].params;
   Init();

   if (mUIDialog)
   {
      TransferDataToWindow();
   }

   return true;
}


// Effect implementation
void EffectDistortion::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);
   S.StartVerticalLay();
   {
      S.StartMultiColumn(4, wxCENTER);
      {
         mTypeChoiceCtrl =
         S
            .Id(ID_Type)
            .MinSize( { -1, -1 } )
            .Validator<wxGenericValidator>(&mTableChoiceIndx)
            .AddChoice(XXO("Distortion type:"),
               Msgids(kTableTypeStrings, nTableTypes));

         mBook1 =
         S
            .StartSimplebook();
         {
            size_t ii = 0;
            for ( const auto &spec : specs )
               PopulateCheckboxPage( S, spec, ii++ );
         }
         S.EndSimplebook();
      }
      S.EndMultiColumn();
      S.AddSpace(0, 10);


      S.StartStatic(XO("Threshold controls"));
      {
         mBook2 =
         S
            .StartSimplebook();
         {
            size_t ii = 0;
            for ( const auto &spec : specs )
               PopulateThresholdPage( S, spec, ii++ );
         }
         S.EndSimplebook();
      }
      S.EndStatic();

      S.StartStatic(XO("Parameter controls"));
      {
         mBook3 =
         S
            .StartSimplebook();
         {
            size_t ii = 0;
            for ( const auto &spec : specs )
               PopulateParameterPage( S, spec, ii++ );
         }
         S.EndSimplebook();
      }
      S.EndStatic();
   }
   S.EndVerticalLay();

   return;
}

void EffectDistortion::PopulateCheckboxPage(
   ShuttleGui &S, const UISpec &spec, size_t index )
{
   auto &params = mPageParams[ index ];
   Controls &controls = mControls[ index ];
   S
      .StartNotebookPage({});
   {
      if ( spec.dcBlockEnabled )
         S.Validator<wxGenericValidator>( &params.mDCBlock );
      else
         S.Disable();

      S
         .AddCheckBox( XXO("DC blocking filter"), DCBlock.def );
   }
   S.EndNotebookPage();
}

static TranslatableString LabelWithSuffix(
   const TranslatableString &str, size_t index )
{
   TranslatableString label, suffix;
   if ( !str.empty() )
      label = str, suffix = defaultSuffix[index];
   else
      label = defaultLabel[index], suffix = XO("(Not Used):");
   return label.Join( suffix, L" " );
};

void EffectDistortion::PopulateThresholdPage(
   ShuttleGui &S, const UISpec &spec, size_t index )
{
   auto &params = mPageParams[ index ];
   Controls &controls = mControls[ index ];
   TranslatableString label;

   S
      .StartNotebookPage({});
   S.StartMultiColumn(4, wxEXPAND);
   S.SetStretchyCol(2);
   {
      // Allow space for first Column
      S.AddSpace(250,0);
      S.AddSpace(0,0);
      S.AddSpace(0,0);
      S.AddSpace(0,0);

      // Upper threshold control
      label = LabelWithSuffix( spec.thresholdName, 0 );
      S
         .AddVariableText( label,
            false, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

      if ( spec.thresholdName.empty() )
         S.Disable();
      else
         S.Validator<FloatingPointValidator<double>>(
            2, &params.mThreshold_dB, NumValidatorStyle::DEFAULT,
            Threshold_dB.min, Threshold_dB.max );

      controls.mThresholdT =
      S
         .Id(ID_Threshold)
         .Text( label )
         .AddTextBox( {}, L"", 10);

      controls.mThresholdS =
      S
         .Id(ID_Threshold)
         .Text( label )
         .Disable( spec.thresholdName.empty() )
         .Style(wxSL_HORIZONTAL)
         .AddSlider( {}, 0,
            DB_TO_LINEAR(Threshold_dB.max) * Threshold_dB.scale,
            DB_TO_LINEAR(Threshold_dB.min) * Threshold_dB.scale);

      S.AddSpace(20, 0);

      // Noise floor control
      label = LabelWithSuffix( spec.noiseFloorName, 1 );
      S
         .AddVariableText( label,
            false, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

      if ( spec.noiseFloorName.empty() )
         S.Disable();
      else
         S.Validator<FloatingPointValidator<double>>(
            2, &params.mNoiseFloor, NumValidatorStyle::DEFAULT,
            NoiseFloor.min, NoiseFloor.max );

      controls.mNoiseFloorT =
      S
         .Id(ID_NoiseFloor)
         .Text( label )
         .AddTextBox( {}, L"", 10);

      controls.mNoiseFloorS =
      S
         .Id(ID_NoiseFloor)
         .Text( label )
         .Disable( spec.noiseFloorName.empty() )
         .Style(wxSL_HORIZONTAL)
         .AddSlider( {}, 0, NoiseFloor.max, NoiseFloor.min);

      S.AddSpace(20, 0);
   }
   S.EndMultiColumn();
   S.EndNotebookPage();
}

void EffectDistortion::PopulateParameterPage(
   ShuttleGui &S, const UISpec &spec, size_t index )
{
   auto &params = mPageParams[ index ];
   Controls &controls = mControls[ index ];
   TranslatableString label, suffix;

   S
      .StartNotebookPage({});
   S.StartMultiColumn(4, wxEXPAND);
   S.SetStretchyCol(2);
   {
      // Allow space for first Column
      S.AddSpace(250,0);
      S.AddSpace(0,0);
      S.AddSpace(0,0);
      S.AddSpace(0,0);

      // Parameter1 control
      label = LabelWithSuffix( spec.param1Name, 2 );
      S
         .AddVariableText( label,
            false, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

      if ( spec.param1Name.empty() )
         S.Disable();
      else
         S.Validator<FloatingPointValidator<double>>(
            2, &params.mParam1, NumValidatorStyle::DEFAULT,
            Param1.min, Param1.max );

      controls.mParam1T =
      S
         .Id(ID_Param1)
         .Text( label )
         .AddTextBox( {}, L"", 10);

      controls.mParam1S =
      S
         .Id(ID_Param1)
         .Text( label )
         .Disable( spec.param1Name.empty() )
         .Style(wxSL_HORIZONTAL)
         .AddSlider( {}, 0, Param1.max, Param1.min);

      S.AddSpace(20, 0);

      // Parameter2 control
      label = LabelWithSuffix( spec.param2Name, 3 );
      S
         .AddVariableText( label,
            false, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

      if ( spec.param2Name.empty() )
         S.Disable();
      else
         S.Validator<FloatingPointValidator<double>>(
            2, &params.mParam2, NumValidatorStyle::DEFAULT,
            Param2.min, Param2.max );

      controls.mParam2T = S.Id(ID_Param2)
         .Text( label )
         .AddTextBox( {}, L"", 10);

      controls.mParam2S =
      S
         .Id(ID_Param2)
         .Text( label )
         .Disable( spec.param2Name.empty() )
         .Style(wxSL_HORIZONTAL)
         .AddSlider( {}, 0, Param2.max, Param2.min);

      S.AddSpace(20, 0);

      // Repeats control
      label = LabelWithSuffix( spec.repeatsName, 4 );
      S
         .AddVariableText( label,
            false, wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT);

      if ( spec.repeatsName.empty() )
         S.Disable();
      else
         S.Validator<IntegerValidator<int>>(
            &params.mRepeats, NumValidatorStyle::DEFAULT,
            Repeats.min, Repeats.max );

      controls.mRepeatsT =
      S
         .Id(ID_Repeats)
         .Text( label )
         .AddTextBox( {}, L"", 10);

      controls.mRepeatsS =
      S
         .Id(ID_Repeats)
         .Text( label )
         .Disable( spec.repeatsName.empty() )
         .Style(wxSL_HORIZONTAL)
         .AddSlider( {}, Repeats.def, Repeats.max, Repeats.min);

      S.AddSpace(20, 0);
   }
   S.EndMultiColumn();
   S.EndNotebookPage();
}

bool EffectDistortion::Init()
{
   for ( auto &param : mPageParams )
      param = mParams;
   return true;
}

bool EffectDistortion::TransferDataToWindow()
{
   const auto thresholdLinear = DB_TO_LINEAR( mParams.mThreshold_dB );

   if (!mUIParent->TransferDataToWindow())
   {
      return false;
   }

   auto &controls = mControls[ mTableChoiceIndx ];
   controls.mThresholdS->SetValue((int) (thresholdLinear * Threshold_dB.scale + 0.5));
   controls.mNoiseFloorS->SetValue((int) mParams.mNoiseFloor + 0.5);
   controls.mParam1S->SetValue((int) mParams.mParam1 + 0.5);
   controls.mParam2S->SetValue((int) mParams.mParam2 + 0.5);
   controls.mRepeatsS->SetValue(mParams.mRepeats);

   return true;
}

bool EffectDistortion::TransferDataFromWindow()
{
   if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
   {
      return false;
   }

   mParams = mPageParams[ mTableChoiceIndx ];

   return true;
}

void EffectDistortion::InstanceInit(EffectDistortionState & data, float sampleRate)
{
   data.samplerate = sampleRate;
   data.skipcount = 0;
   data.tablechoiceindx = mTableChoiceIndx;
   data.dcblock = mParams.mDCBlock;
   data.threshold = mParams.mThreshold_dB;
   data.noisefloor = mParams.mNoiseFloor;
   data.param1 = mParams.mParam1;
   data.param2 = mParams.mParam2;
   data.repeats = mParams.mRepeats;

   // DC block filter variables
   data.queuetotal = 0.0;

   //std::queue<float>().swap(data.queuesamples);
   while (!data.queuesamples.empty())
      data.queuesamples.pop();

   MakeTable();

   return;
}

size_t EffectDistortion::InstanceProcess(EffectDistortionState& data, float** inBlock, float** outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   float *obuf = outBlock[0];

   bool update = (mTableChoiceIndx == data.tablechoiceindx &&
                  mParams.mNoiseFloor == data.noisefloor &&
                  mParams.mThreshold_dB == data.threshold &&
                  mParams.mParam1 == data.param1 &&
                  mParams.mParam2 == data.param2 &&
                  mParams.mRepeats == data.repeats)? false : true;

   double p1 = mParams.mParam1 / 100.0;
   double p2 = mParams.mParam2 / 100.0;

   data.tablechoiceindx = mTableChoiceIndx;
   data.threshold = mParams.mThreshold_dB;
   data.noisefloor = mParams.mNoiseFloor;
   data.param1 = mParams.mParam1;
   data.repeats = mParams.mRepeats;

   for (decltype(blockLen) i = 0; i < blockLen; i++) {
      if (update && ((data.skipcount++) % skipsamples == 0)) {
         MakeTable();
      }

      switch (mTableChoiceIndx)
      {
      case kHardClip:
         // Param2 = make-up gain.
         obuf[i] = WaveShaper(ibuf[i]) * ((1 - p2) + (mMakeupGain * p2));
         break;
      case kSoftClip:
         // Param2 = make-up gain.
         obuf[i] = WaveShaper(ibuf[i]) * ((1 - p2) + (mMakeupGain * p2));
         break;
      case kHalfSinCurve:
         obuf[i] = WaveShaper(ibuf[i]) * p2;
         break;
      case kExpCurve:
         obuf[i] = WaveShaper(ibuf[i]) * p2;
         break;
      case kLogCurve:
         obuf[i] = WaveShaper(ibuf[i]) * p2;
         break;
      case kCubic:
         obuf[i] = WaveShaper(ibuf[i]) * p2;
         break;
      case kEvenHarmonics:
         obuf[i] = WaveShaper(ibuf[i]);
         break;
      case kSinCurve:
         obuf[i] = WaveShaper(ibuf[i]) * p2;
         break;
      case kLeveller:
         obuf[i] = WaveShaper(ibuf[i]);
         break;
      case kRectifier:
         obuf[i] = WaveShaper(ibuf[i]);
         break;
      case kHardLimiter:
         // Mix equivalent to LADSPA effect's "Wet / Residual" mix
         obuf[i] = (WaveShaper(ibuf[i]) * (p1 - p2)) + (ibuf[i] * p2);
         break;
      default:
         obuf[i] = WaveShaper(ibuf[i]);
      }
      if (mParams.mDCBlock) {
         obuf[i] = DCFilter(data, obuf[i]);
      }
   }

   return blockLen;
}

void EffectDistortion::OnTypeChoice(wxCommandEvent& /*evt*/)
{
   mParams = mPageParams[ mTableChoiceIndx ];
   mTypeChoiceCtrl->GetValidator()->TransferFromWindow();
   Init();
   for ( auto pBook : { mBook1, mBook2, mBook3 } )
      pBook->SetSelection( mTableChoiceIndx );
   if ( mUIDialog )
      mUIDialog->TransferDataToWindow();
}

void EffectDistortion::OnThresholdText(wxCommandEvent& /*evt*/)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   controls.mThresholdT->GetValidator()->TransferFromWindow();
   auto threshold = DB_TO_LINEAR(params.mThreshold_dB);
   controls.mThresholdS->SetValue((int) (threshold * Threshold_dB.scale + 0.5));
}

void EffectDistortion::OnThresholdSlider(wxCommandEvent& evt)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   auto threshold = (double) evt.GetInt() / Threshold_dB.scale;
   params.mThreshold_dB = wxMax(LINEAR_TO_DB(threshold), Threshold_dB.min);
   threshold = std::max(MIN_Threshold_Linear, threshold);
   controls.mThresholdT->GetValidator()->TransferToWindow();
}

void EffectDistortion::OnNoiseFloorText(wxCommandEvent& /*evt*/)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   controls.mNoiseFloorT->GetValidator()->TransferFromWindow();
   controls.mNoiseFloorS->SetValue((int) floor(params.mNoiseFloor + 0.5));
}

void EffectDistortion::OnNoiseFloorSlider(wxCommandEvent& evt)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   params.mNoiseFloor = (double) evt.GetInt();
   controls.mNoiseFloorT->GetValidator()->TransferToWindow();
}


void EffectDistortion::OnParam1Text(wxCommandEvent& /*evt*/)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   controls.mParam1T->GetValidator()->TransferFromWindow();
   controls.mParam1S->SetValue((int) floor(params.mParam1 + 0.5));
}

void EffectDistortion::OnParam1Slider(wxCommandEvent& evt)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   params.mParam1 = (double) evt.GetInt();
   controls.mParam1T->GetValidator()->TransferToWindow();
}

void EffectDistortion::OnParam2Text(wxCommandEvent& /*evt*/)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   controls.mParam2T->GetValidator()->TransferFromWindow();
   controls.mParam2S->SetValue((int) floor(params.mParam2 + 0.5));
}

void EffectDistortion::OnParam2Slider(wxCommandEvent& evt)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   params.mParam2 = (double) evt.GetInt();
   controls.mParam2T->GetValidator()->TransferToWindow();
}

void EffectDistortion::OnRepeatsText(wxCommandEvent& /*evt*/)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   controls.mRepeatsT->GetValidator()->TransferFromWindow();
   controls.mRepeatsS->SetValue(params.mRepeats);
}

void EffectDistortion::OnRepeatsSlider(wxCommandEvent& evt)
{
   auto &controls = mControls[ mTableChoiceIndx ];
   auto &params = mPageParams[ mTableChoiceIndx ];
   params.mRepeats = evt.GetInt();
   controls.mRepeatsT->GetValidator()->TransferToWindow();
   
}

void EffectDistortion::MakeTable()
{
   switch (mTableChoiceIndx)
   {
      case kHardClip:
         HardClip();
         break;
      case kSoftClip:
         SoftClip();
         break;
      case kHalfSinCurve:
         HalfSinTable();
         break;
      case kExpCurve:
         ExponentialTable();
         break;
      case kLogCurve:
         LogarithmicTable();
         break;
      case kCubic:
         CubicTable();
         break;
      case kEvenHarmonics:
         EvenHarmonicTable();
         break;
      case kSinCurve:
         SineTable();
         break;
      case kLeveller:
         Leveller();
         break;
      case kRectifier:
         Rectifier();
         break;
      case kHardLimiter:
         HardLimiter();
         break;
   }
}


//
// Preset tables for gain lookup
//

void EffectDistortion::HardClip()
{
   const auto thresholdLinear = DB_TO_LINEAR( mParams.mThreshold_dB );
   double lowThresh = 1 - thresholdLinear;
   double highThresh = 1 + thresholdLinear;

   for (int n = 0; n < TABLESIZE; n++) {
      if (n < (STEPS * lowThresh))
         mTable[n] = - thresholdLinear;
      else if (n > (STEPS * highThresh))
         mTable[n] = thresholdLinear;
      else
         mTable[n] = n/(double)STEPS - 1;

      mMakeupGain = 1.0 / thresholdLinear;
   }
}

void EffectDistortion::SoftClip()
{
   const auto thresholdLinear = DB_TO_LINEAR( mParams.mThreshold_dB );
   double threshold = 1 + thresholdLinear;
   double amount = std::pow(2.0, 7.0 * mParams.mParam1 / 100.0); // range 1 to 128
   double peak = LogCurve(thresholdLinear, 1.0, amount);
   mMakeupGain = 1.0 / peak;
   mTable[STEPS] = 0.0;   // origin

   // positive half of table
   for (int n = STEPS; n < TABLESIZE; n++) {
      if (n < (STEPS * threshold)) // origin to threshold
         mTable[n] = n/(float)STEPS - 1;
      else
         mTable[n] = LogCurve(thresholdLinear, n/(double)STEPS - 1, amount);
   }
   CopyHalfTable();
}

float EffectDistortion::LogCurve(double threshold, float value, double ratio)
{
   return threshold + ((std::exp(ratio * (threshold - value)) - 1) / -ratio);
}

void EffectDistortion::ExponentialTable()
{
   double amount = std::min(0.999, DB_TO_LINEAR(-1 * mParams.mParam1));   // avoid divide by zero

   for (int n = STEPS; n < TABLESIZE; n++) {
      double linVal = n/(float)STEPS;
      double scale = -1.0 / (1.0 - amount);   // unity gain at 0dB
      double curve = std::exp((linVal - 1) * std::log(amount));
      mTable[n] = scale * (curve -1);
   }
   CopyHalfTable();
}

void EffectDistortion::LogarithmicTable()
{
   double amount = mParams.mParam1;
   double stepsize = 1.0 / STEPS;
   double linVal = 0;

   if (amount == 0){
      for (int n = STEPS; n < TABLESIZE; n++) {
      mTable[n] = linVal;
      linVal += stepsize;
      }
   }
   else {
      for (int n = STEPS; n < TABLESIZE; n++) {
         mTable[n] = std::log(1 + (amount * linVal)) / std::log(1 + amount);
         linVal += stepsize;
      }
   }
   CopyHalfTable();
}

void EffectDistortion::HalfSinTable()
{
   int iter = std::floor(mParams.mParam1 / 20.0);
   double fractionalpart = (mParams.mParam1 / 20.0) - iter;
   double stepsize = 1.0 / STEPS;
   double linVal = 0;

   for (int n = STEPS; n < TABLESIZE; n++) {
      mTable[n] = linVal;
      for (int i = 0; i < iter; i++) {
         mTable[n] = std::sin(mTable[n] * M_PI_2);
      }
      mTable[n] += ((std::sin(mTable[n] * M_PI_2) - mTable[n]) * fractionalpart);
      linVal += stepsize;
   }
   CopyHalfTable();
}

void EffectDistortion::CubicTable()
{
   double amount = mParams.mParam1 * std::sqrt(3.0) / 100.0;
   double gain = 1.0;
   if (amount != 0.0)
      gain = 1.0 / Cubic(std::min(amount, 1.0));

   double stepsize = amount / STEPS;
   double x = -amount;
   
   if (amount == 0) {
      for (int i = 0; i < TABLESIZE; i++) {
         mTable[i] = (i / (double)STEPS) - 1.0;
      }
   }
   else {
      for (int i = 0; i < TABLESIZE; i++) {
         mTable[i] = gain * Cubic(x);
         for (int j = 0; j < mParams.mRepeats; j++) {
            mTable[i] = gain * Cubic(mTable[i] * amount);
         }
         x += stepsize;
      }
   }
}

double EffectDistortion::Cubic(double x)
{
   if (mParams.mParam1 == 0.0)
      return x;

   return x - (std::pow(x, 3.0) / 3.0);
}


void EffectDistortion::EvenHarmonicTable()
{
   double amount = mParams.mParam1 / -100.0;
   // double C = std::sin(std::max(0.001, mParams.mParam2) / 100.0) * 10.0;
   double C = std::max(0.001, mParams.mParam2) / 10.0;

   double step = 1.0 / STEPS;
   double xval = -1.0;

   for (int i = 0; i < TABLESIZE; i++) {
      mTable[i] = ((1 + amount) * xval) -
                  (xval * (amount / std::tanh(C)) * std::tanh(C * xval));
      xval += step;
   }
}

void EffectDistortion::SineTable()
{
   int iter = std::floor(mParams.mParam1 / 20.0);
   double fractionalpart = (mParams.mParam1 / 20.0) - iter;
   double stepsize = 1.0 / STEPS;
   double linVal = 0.0;

   for (int n = STEPS; n < TABLESIZE; n++) {
      mTable[n] = linVal;
      for (int i = 0; i < iter; i++) {
         mTable[n] = (1.0 + std::sin((mTable[n] * M_PI) - M_PI_2)) / 2.0;
      }
      mTable[n] += (((1.0 + std::sin((mTable[n] * M_PI) - M_PI_2)) / 2.0) - mTable[n]) * fractionalpart;
      linVal += stepsize;
   }
   CopyHalfTable();
}

void EffectDistortion::Leveller()
{
   double noiseFloor = DB_TO_LINEAR(mParams.mNoiseFloor);
   int numPasses = mParams.mRepeats;
   double fractionalPass = mParams.mParam1 / 100.0;

   const int numPoints = 6;
   const double gainFactors[numPoints] = { 0.80, 1.00, 1.20, 1.20, 1.00, 0.80 };
   double gainLimits[numPoints] = { 0.0001, 0.0, 0.1, 0.3, 0.5, 1.0 };
   double addOnValues[numPoints];

   gainLimits[1] = noiseFloor;
   /* In the original Leveller effect, behaviour was undefined for threshold > 20 dB.
    * If we want to support > 20 dB we need to scale the points to keep them non-decreasing.
    * 
    * if (noiseFloor > gainLimits[2]) {
    *    for (int i = 3; i < numPoints; i++) {
    *    gainLimits[i] = noiseFloor + ((1 - noiseFloor)*((gainLimits[i] - 0.1) / 0.9));
    * }
    * gainLimits[2] = noiseFloor;
    * }
    */

   // Calculate add-on values
   addOnValues[0] = 0.0;
   for (int i = 0; i < numPoints-1; i++) {
      addOnValues[i+1] = addOnValues[i] + (gainLimits[i] * (gainFactors[i] - gainFactors[1 + i]));
   }

   // Positive half of table.
   // The original effect increased the 'strength' of the effect by
   // repeated passes over the audio data.
   // Here we model that more efficiently by repeated passes over a linear table.
   for (int n = STEPS; n < TABLESIZE; n++) {
      mTable[n] = ((double) (n - STEPS) / (double) STEPS);
      for (int j = 0; j < numPasses; j++) {
         // Find the highest index for gain adjustment
         int index = numPoints - 1;
         for (int i = index; i >= 0 && mTable[n] < gainLimits[i]; i--) {
            index = i;
         }
         // the whole number of 'repeats'
         mTable[n] = (mTable[n] * gainFactors[index]) + addOnValues[index];
      }
      // Extrapolate for fine adjustment.
      // tiny fractions are not worth the processing time
      if (fractionalPass > 0.001) {
         int index = numPoints - 1;
         for (int i = index; i >= 0 && mTable[n] < gainLimits[i]; i--) {
            index = i;
         }
         mTable[n] += fractionalPass * ((mTable[n] * (gainFactors[index] - 1)) + addOnValues[index]);
      }
   }
   CopyHalfTable();
}

void EffectDistortion::Rectifier()
{
   double amount = (mParams.mParam1 / 50.0) - 1;
   double stepsize = 1.0 / STEPS;
   int index = STEPS;

   // positive half of waveform is passed unaltered.
   for  (int n = 0; n <= STEPS; n++) {
      mTable[index] = n * stepsize;
      index += 1;
   }

   // negative half of table
   index = STEPS - 1;
   for (int n = 1; n <= STEPS; n++) {
      mTable[index] = n * stepsize * amount;
      index--;
   }
}

void EffectDistortion::HardLimiter()
{
   // The LADSPA "hardLimiter 1413" is basically hard clipping,
   // but with a 'kind of' wet/dry mix:
   // out = ((wet-residual)*clipped) + (residual*in)
   HardClip();
}


// Helper functions for lookup tables

void EffectDistortion::CopyHalfTable()
{
   // Copy negative half of table from positive half
   int count = TABLESIZE - 1;
   for (int n = 0; n < STEPS; n++) {
      mTable[n] = -mTable[count];
      count--;
   }
}


float EffectDistortion::WaveShaper(float sample)
{
   float out;
   int index;
   double xOffset;
   double amount = 1;

   switch (mTableChoiceIndx)
   {
      // Do any pre-processing here
      case kHardClip:
         // Pre-gain
         amount = mParams.mParam1 / 100.0;
         sample *= 1+amount;
         break;
      default: break;
   }

   index = std::floor(sample * STEPS) + STEPS;
   index = std::max<int>(std::min<int>(index, 2 * STEPS - 1), 0);
   xOffset = ((1 + sample) * STEPS) - index;
   xOffset = std::min<double>(std::max<double>(xOffset, 0.0), 1.0);   // Clip at 0dB

   // linear interpolation: y = y0 + (y1-y0)*(x-x0)
   out = mTable[index] + (mTable[index + 1] - mTable[index]) * xOffset;

   return out;
}


float EffectDistortion::DCFilter(EffectDistortionState& data, float sample)
{
   // Rolling average gives less offset at the start than an IIR filter.
   const unsigned int queueLength = std::floor(data.samplerate / 20.0);

   data.queuetotal += sample;
   data.queuesamples.push(sample);

   if (data.queuesamples.size() > queueLength) {
      data.queuetotal -= data.queuesamples.front();
      data.queuesamples.pop();
   }

   return sample - (data.queuetotal / data.queuesamples.size());
}
