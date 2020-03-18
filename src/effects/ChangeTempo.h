/**********************************************************************

  Audacity: A Digital Audio Editor

  ChangeTempo.h

  Vaughan Johnson, Dominic Mazzoni

  Change Tempo effect provides speeding up or
  slowing down tempo without changing pitch.

**********************************************************************/


#if USE_SOUNDTOUCH

#ifndef __AUDACITY_EFFECT_CHANGETEMPO__
#define __AUDACITY_EFFECT_CHANGETEMPO__

#if USE_SBSMS
#include "SBSMSEffect.h"
#endif

#include "SoundTouchEffect.h"
#include "../Shuttle.h"

class wxSlider;
class wxStaticText;
class wxTextCtrl;
class ShuttleGui;

class EffectChangeTempo final : public EffectSoundTouch
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectChangeTempo();
   virtual ~EffectChangeTempo();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   bool SupportsAutomation() override;

   // Effect implementation

   bool Init() override;
   bool CheckWhetherSkipEffect() override;
   bool Process() override;
   double CalcPreviewInputLength(double previewLength) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;

private:
   bool           mUseSBSMS;
   double         m_PercentChange;  // percent change to apply to tempo
                                    // -100% is meaningless, but sky's the upper limit
   double         m_FromBPM;        // user-set beats-per-minute. Zero means not yet set.
   double         m_FromLength;     // starting length of selection

   // controls
   wxStaticText * m_pTextCtrl_FromLength;

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
};

#endif // __AUDACITY_EFFECT_CHANGETEMPO__

#endif // USE_SOUNDTOUCH
