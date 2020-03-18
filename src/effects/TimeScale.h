/**********************************************************************

  Audacity: A Digital Audio Editor

  TimeScale.h

  Clayton Otey

**********************************************************************/

#ifndef __AUDACITY_EFFECT_TIMESCALE__
#define __AUDACITY_EFFECT_TIMESCALE__



#if USE_SBSMS

#include "SBSMSEffect.h"
#include "../Shuttle.h"

class ShuttleGui;

class EffectTimeScale final : public EffectSBSMS
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectTimeScale();
   virtual ~EffectTimeScale();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // Effect implementation

   bool Init() override;
   void Preview(bool dryOnly) override;
   bool Process() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   double CalcPreviewInputLength(double previewLength) override;

private:
   // EffectTimeScale implementation

   static inline double PercentChangeToRatio(double percentChange);
   static inline double HalfStepsToPercentChange(double halfSteps);
   static inline double PercentChangeToHalfSteps(double percentChange);

private:
   bool bPreview;
   double previewSelectedDuration;
   SlideType slideTypeRate;
   SlideType slideTypePitch;
   double m_RatePercentChangeStart;
   double m_RatePercentChangeEnd;
   double m_PitchHalfStepsStart; // unused!
   double m_PitchHalfStepsEnd; // unused!
   double m_PitchPercentChangeStart;
   double m_PitchPercentChangeEnd;

   CapturedParameters mParameters;
   CapturedParameters& Parameters() override { return mParameters; }
};

#endif // __AUDACITY_EFFECT_TIMESCALE

#endif // USE_SBSMS
