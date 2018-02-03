/**********************************************************************

  Audacity: A Digital Audio Editor

  ChangeTempo.cpp

  Vaughan Johnson,
  Dominic Mazzoni

*******************************************************************//**

\class EffectChangeTempo
\brief An EffectSoundTouch provides speeding up or
  slowing down tempo without changing pitch.

*//*******************************************************************/



#if USE_SOUNDTOUCH
#include "ChangeTempo.h"

#include <math.h>

#include "../ShuttleGui.h"
#include "../widgets/numformatter.h"
#include "TimeWarper.h"

#include "LoadEffects.h"

// Soundtouch defines these as well, which are also in generated configmac.h
// and configunix.h, so get rid of them before including,
// to avoid compiler warnings, and be sure to do this
// after all other #includes, to avoid any mischief that might result
// from doing the un-definitions before seeing any wx headers.
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef PACKAGE_BUGREPORT
#undef PACKAGE
#undef VERSION
#include "SoundTouch.h"

// Soundtouch is not reasonable below -99% or above 3000%.

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name          Type     Key               Def   Min      Max      Scale
static auto Percentage = Parameter<double>(
                           L"Percentage", 0.0,  -95.0,   3000.0,  1  );
static auto UseSBSMS = Parameter<bool>(
                           L"SBSMS",     false, false,   true,    1  );

// We warp the slider to go up to 400%, but user can enter higher values.
static const double kSliderMax = 100.0;         // warped above zero to actually go up to 400%
static const double kSliderWarp = 1.30105;      // warp power takes max from 100 to 400.

//
// EffectChangeTempo
//

const ComponentInterfaceSymbol EffectChangeTempo::Symbol
{ XO("Change Tempo") };

namespace{ BuiltinEffectsModule::Registration< EffectChangeTempo > reg; }

EffectChangeTempo::EffectChangeTempo()
   : mParameters{
      m_PercentChange, Percentage,
      mUseSBSMS, UseSBSMS
   }
{
   Parameters().Reset();
   m_FromBPM = 0.0; // indicates not yet set
   m_FromLength = 0.0;

   SetLinearEffectFlag(true);
}

EffectChangeTempo::~EffectChangeTempo()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectChangeTempo::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectChangeTempo::GetDescription()
{
   return XO("Changes the tempo of a selection without changing its pitch");
}

wxString EffectChangeTempo::ManualPage()
{
   return L"Change_Tempo";
}

// EffectDefinitionInterface implementation

EffectType EffectChangeTempo::GetType()
{
   return EffectTypeProcess;
}

bool EffectChangeTempo::SupportsAutomation()
{
   return true;
}

// Effect implementation

double EffectChangeTempo::CalcPreviewInputLength(double previewLength)
{
   return previewLength * (100.0 + m_PercentChange) / 100.0;
}

bool EffectChangeTempo::CheckWhetherSkipEffect()
{
   return (m_PercentChange == 0.0);
}

bool EffectChangeTempo::Init()
{
   // The selection might have changed since the last time EffectChangeTempo
   // was invoked, so recalculate the Length parameters.
   m_FromLength = mT1 - mT0;

   return true;
}

bool EffectChangeTempo::Process()
{
   bool success = false;

#if USE_SBSMS
   if (mUseSBSMS)
   {
      double tempoRatio = 1.0 + m_PercentChange / 100.0;
      EffectSBSMS proxy;
      proxy.mProxyEffectName = XO("High Quality Tempo Change");
      proxy.setParameters(tempoRatio, 1.0);
      success = Delegate(proxy, *mUIParent, nullptr);
   }
   else
#endif
   {
      auto initer = [&](soundtouch::SoundTouch *soundtouch)
      {
         soundtouch->setTempoChange(m_PercentChange);
      };
      double mT1Dashed = mT0 + (mT1 - mT0)/(m_PercentChange/100.0 + 1.0);
      RegionTimeWarper warper{ mT0, mT1,
         std::make_unique<LinearTimeWarper>(mT0, mT0, mT1, mT1Dashed )  };
      success = EffectSoundTouch::ProcessWithTimeWarper(initer, warper, false);
   }

   if(success)
      mT1 = mT0 + (mT1 - mT0)/(m_PercentChange/100 + 1.);

   return success;
}

enum { precision = 2 };

static inline wxString FormatLength( double length )
{
   return NumberFormatter::ToString( length, precision,
      NumberFormatter::Style_TwoTrailingZeroes );
}

void EffectChangeTempo::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.StartVerticalLay(0);
   {
      S.AddSpace(0, 5);

      S
         .AddTitle(XO("Change Tempo without Changing Pitch"));

      S.SetBorder(5);

      //
      S.StartMultiColumn(2, GroupOptions{}.Position(wxCENTER));
      {
         S
            .Target( m_PercentChange,
               NumValidatorStyle::THREE_TRAILING_ZEROES, 3,
               Percentage.min, Percentage.max )
            .AddTextBox(XXO("Percent C&hange:"), L"", 12);
      }
      S.EndMultiColumn();

      //
      S.StartHorizontalLay(wxEXPAND);
      {
         S
            .Text(XO("Percent Change"))
            .Style(wxSL_HORIZONTAL)
            .Target( Transform( m_PercentChange,
               [](double output){
                  return output > 0
                     // Un-warp values above zero to actually go up to
                     // kSliderMax.
                     ? pow( output, (1.0 / kSliderWarp ) )
                     : output;
               },
               [](double input){
                  return input > 0
                     // Warp positive values to actually go up faster & further
                     // than negatives.
                     ? pow( input, kSliderWarp )
                     : input;
               } ) )
            .AddSlider( {}, 0, (int)kSliderMax, (int)Percentage.min);
      }
      S.EndHorizontalLay();

      S.StartStatic(XO("Beats per minute"));
      {
         S.StartHorizontalLay(wxALIGN_CENTER);
         {
            S
               /* i18n-hint: changing tempo "from" one value "to" another */
               .Text(XO("Beats per minute, from"))
               .Target( m_FromBPM,
                  NumValidatorStyle::THREE_TRAILING_ZEROES
                     | NumValidatorStyle::ZERO_AS_BLANK, 3, 0)
               /* i18n-hint: changing tempo "from" one value "to" another */
               .AddTextBox(XXC("&from", "change tempo"), L"", 12);

            S
               /* i18n-hint: changing tempo "from" one value "to" another */
               .Text(XO("Beats per minute, to"))
               .Target( Transform( m_PercentChange,
                     [this](double output){
                        return m_FromBPM > 0
                           ? (m_FromBPM * (100.0 + output)) / 100.0
                           : 0;
                     },
                     [this](double input){
                        return m_FromBPM > 0
                           ? ((input * 100.0) / m_FromBPM) - 100.0
                           : m_PercentChange;
                     } ),
                  NumValidatorStyle::THREE_TRAILING_ZEROES
                     | NumValidatorStyle::ZERO_AS_BLANK, 3, 0)
               /* i18n-hint: changing tempo "from" one value "to" another */
               .AddTextBox(XXC("&to", "change tempo"), L"", 12);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      //
      S.StartStatic(XO("Length (seconds)"));
      {
         S.StartHorizontalLay(wxALIGN_CENTER);
         {
            /* i18n-hint: changing tempo "from" one value "to" another */
            S.AddPrompt(XXC("from", "change tempo"));

            S
               .Size( { 60, -1 } )
               .Style( wxALIGN_RIGHT )
               .VariableText( [this]{ return Label( Verbatim(
                   FormatLength( m_FromLength ) ) ); } )
               .AddVariableText( Verbatim( FormatLength( m_FromLength ) ),
                  false, wxALL | wxALIGN_CENTRE_VERTICAL );

            S
               .Target( Transform( m_PercentChange,
                     [this](double output){
                        return (m_FromLength * 100.0) / (100.0 + output);
                     },
                     [this](double input){
                        return input > 0
                           ? ((m_FromLength * 100.0) / input) - 100.0
                           : m_PercentChange;
                     } ),
                  NumValidatorStyle::TWO_TRAILING_ZEROES, 2,
                  // min and max need same precision as what we're validating (bug 963)
                  RoundValue( precision,
                     (m_FromLength * 100.0) / (100.0 + Percentage.max) ),
                  RoundValue( precision,
                     (m_FromLength * 100.0) / (100.0 + Percentage.min) ) )
               .VariableText( [this]{ return ControlText{
                  XO("Length in seconds from %s, to")
                     .Format( FormatLength( m_FromLength ) ) }; } )
               /* i18n-hint: changing tempo "from" one value "to" another */
               .AddTextBox(XXC("t&o", "change tempo"), L"", 12);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

#if USE_SBSMS
      S.StartMultiColumn(2);
      {
         S
            .Target( mUseSBSMS )
            .AddCheckBox(XXO("&Use high quality stretching (slow)"),
                                             mUseSBSMS);
      }
      S.EndMultiColumn();
#endif

   }
   S.EndVerticalLay();

   return;
}

bool EffectChangeTempo::TransferDataToWindow()
{
   // Reset from length because it can be changed by Preview
   m_FromLength = mT1 - mT0;
   return true;
}

#endif // USE_SOUNDTOUCH
