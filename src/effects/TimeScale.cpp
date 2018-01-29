/**********************************************************************

  Audacity: A Digital Audio Editor

  TimeScale.cpp

  Clayton Otey

*******************************************************************//**

\class EffectTimeScale
\brief An EffectTimeScale does high quality sliding time scaling/pitch shifting

*//*******************************************************************/



#if USE_SBSMS
#include "TimeScale.h"
#include "LoadEffects.h"

#include <math.h>

#include "../ShuttleGui.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name                Type    Key                            Def   Min      Max    Scale
static auto RatePercentStart = Parameter<double>(
                           L"RatePercentChangeStart",  0.0,  -90.0,   500,   1  );
static auto RatePercentEnd = Parameter<double>(
                           L"RatePercentChangeEnd",    0.0,  -90.0,   500,   1  );
static auto HalfStepsStart = Parameter<double>(
                           L"PitchHalfStepsStart",     0.0,  -12.0,   12.0,  1  );
static auto HalfStepsEnd = Parameter<double>(
                           L"PitchHalfStepsEnd",       0.0,  -12.0,   12.0,  1  );
static auto PitchPercentStart = Parameter<double>(
                           L"PitchPercentChangeStart", 0.0,  -50.0,   100.0, 1  );
static auto PitchPercentEnd = Parameter<double>(
                           L"PitchPercentChangeEnd",   0.0,  -50.0,   100.0, 1  );

//
// EffectTimeScale
//

const ComponentInterfaceSymbol EffectTimeScale::Symbol
{ L"Sliding Stretch", XO("Sliding Stretch") };

namespace{ BuiltinEffectsModule::Registration< EffectTimeScale > reg; }

EffectTimeScale::EffectTimeScale()
   : mParameters{
      m_RatePercentChangeStart,  RatePercentStart,
      m_RatePercentChangeEnd,    RatePercentEnd,
      m_PitchHalfStepsStart,     HalfStepsStart,
      m_PitchHalfStepsEnd,       HalfStepsEnd,
      m_PitchPercentChangeStart, PitchPercentStart,
      m_PitchPercentChangeEnd,   PitchPercentEnd
   }
{
   Parameters().Reset();

   slideTypeRate = SlideLinearOutputRate;
   slideTypePitch = SlideLinearOutputRate;
   bPreview = false;
   previewSelectedDuration = 0.0;
   
   SetLinearEffectFlag(true);
}

EffectTimeScale::~EffectTimeScale()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectTimeScale::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectTimeScale::GetDescription()
{
   return XO("Allows continuous changes to the tempo and/or pitch");
}

wxString EffectTimeScale::ManualPage()
{
   return L"Sliding_Stretch";
}

// EffectDefinitionInterface implementation

EffectType EffectTimeScale::GetType()
{
   return EffectTypeProcess;
}

// Effect implementation

bool EffectTimeScale::Init()
{
   return true;
}

double EffectTimeScale::CalcPreviewInputLength(double previewLength)
{
   double inputLength = Effect::GetDuration();
   if(inputLength == 0.0) {
      return 0.0;
   } else {
      double rateStart1 = PercentChangeToRatio(m_RatePercentChangeStart);
      double rateEnd1 = PercentChangeToRatio(m_RatePercentChangeEnd);
      double tOut = previewLength/inputLength;
      double t = EffectSBSMS::getInvertedStretchedTime(rateStart1,rateEnd1,slideTypeRate,tOut);
      return t * inputLength;
   }
}

void EffectTimeScale::Preview(bool dryOnly)
{
   previewSelectedDuration = Effect::GetDuration();
   auto cleanup = valueRestorer( bPreview, true );
   Effect::Preview(dryOnly);
}

bool EffectTimeScale::Process()
{
   double pitchStart1 = PercentChangeToRatio(m_PitchPercentChangeStart);
   double pitchEnd1 = PercentChangeToRatio(m_PitchPercentChangeEnd);
   double rateStart1 = PercentChangeToRatio(m_RatePercentChangeStart);
   double rateEnd1 = PercentChangeToRatio(m_RatePercentChangeEnd);
  
   if(bPreview) {
      double t = (mT1-mT0) / previewSelectedDuration;
      rateEnd1 = EffectSBSMS::getRate(rateStart1,rateEnd1,slideTypeRate,t);
      pitchEnd1 = EffectSBSMS::getRate(pitchStart1,pitchEnd1,slideTypePitch,t);
   }
   
   EffectSBSMS::setParameters(rateStart1,rateEnd1,pitchStart1,pitchEnd1,slideTypeRate,slideTypePitch,false,false,false);
   return EffectSBSMS::Process();
}

void EffectTimeScale::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      // Rate Start
      S.StartStatic(XO("Initial Tempo Change (%)"));
      {
         S.StartMultiColumn(1, wxCENTER);
         {
            S
               .Target( m_RatePercentChangeStart,
                  NumValidatorStyle::NO_TRAILING_ZEROES, 3,
                  RatePercentStart.min, RatePercentStart.max )
               .AddTextBox( {}, L"", 12);
         }
         S.EndMultiColumn();
         S.StartHorizontalLay(wxEXPAND, 0);
         {
            S
               .Target( m_RatePercentChangeStart )
               .Style(wxSL_HORIZONTAL)
               .AddSlider( {}, RatePercentStart.def, RatePercentStart.max, RatePercentStart.min);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S.StartStatic(XO("Final Tempo Change (%)"));
      {
         S.StartMultiColumn(1, wxCENTER);
         {
            S
               .Target( m_RatePercentChangeEnd,
                  NumValidatorStyle::NO_TRAILING_ZEROES, 3,
                  RatePercentEnd.min, RatePercentEnd.max )
               .AddTextBox( {}, L"", 12);
         }
         S.EndMultiColumn();
         S.StartHorizontalLay(wxEXPAND, 0);
         {
            S
               .Target( m_RatePercentChangeEnd )
               .Style(wxSL_HORIZONTAL)
               .AddSlider( {}, RatePercentEnd.def, RatePercentEnd.max, RatePercentEnd.min);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      // Pitch Start
      S.StartStatic(XO("Initial Pitch Shift"));
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S
               .Target( Transform( m_PitchPercentChangeStart,
                     PercentChangeToHalfSteps,
                     HalfStepsToPercentChange ),
                  NumValidatorStyle::NO_TRAILING_ZEROES, 3,
                  HalfStepsStart.min, HalfStepsStart.max )
               .AddTextBox(XXO("(&semitones) [-12 to 12]:"), L"", 12);

            S
               .Target( m_PitchPercentChangeStart,
                  NumValidatorStyle::NO_TRAILING_ZEROES, 3,
                  PitchPercentStart.min, PitchPercentStart.max )
               .AddTextBox(XXO("(%) [-50 to 100]:"), L"", 12);
         }
         S.EndMultiColumn();
      }
      S.EndStatic();

      // Pitch End
      S.StartStatic(XO("Final Pitch Shift"));
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S
               .Target( Transform( m_PitchPercentChangeEnd,
                     PercentChangeToHalfSteps,
                     HalfStepsToPercentChange ),
                  NumValidatorStyle::NO_TRAILING_ZEROES, 3,
                  HalfStepsEnd.min, HalfStepsEnd.max )
               .AddTextBox(XXO("(s&emitones) [-12 to 12]:"), L"", 12);

            S
               .Target( m_PitchPercentChangeEnd,
                  NumValidatorStyle::NO_TRAILING_ZEROES, 3,
                  PitchPercentStart.min, PitchPercentStart.max)
               .AddTextBox(XXO("(%) [-50 to 100]:"), L"", 12);
         }
         S.EndMultiColumn();
      }
      S.EndStatic();
   }
   S.EndMultiColumn();

   return;
}

inline double EffectTimeScale::PercentChangeToRatio(double percentChange)
{
   return 1.0 + percentChange / 100.0;
}

inline double EffectTimeScale::HalfStepsToPercentChange(double halfSteps)
{
   return 100.0 * (pow(2.0,halfSteps/12.0) - 1.0);
}

inline double EffectTimeScale::PercentChangeToHalfSteps(double percentChange)
{
   return 12.0 * log2(PercentChangeToRatio(percentChange));
}

#endif
