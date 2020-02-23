/**********************************************************************

  Audacity: A Digital Audio Editor

  Amplify.h

  Dominic Mazzoni

  This rewritten class supports a smart Amplify effect - it calculates
  the maximum amount of gain that can be applied to all tracks without
  causing clipping and selects this as the default parameter.

**********************************************************************/

#ifndef __AUDACITY_EFFECT_AMPLIFY__
#define __AUDACITY_EFFECT_AMPLIFY__

#include "Effect.h"
#include "../Shuttle.h"


class wxSlider;
class wxTextCtrl;
class ShuttleGui;

class EffectAmplify final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectAmplify();
   virtual ~EffectAmplify();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;
   bool LoadFactoryDefaults() override;

   // Effect implementation

   bool Init() override;
   void Preview(bool dryOnly) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

private:
   // EffectAmplify implementation

   void OnAmpText(wxCommandEvent & evt);
   void OnPeakText(wxCommandEvent & evt);
   void OnAmpSlider(wxCommandEvent & evt);
   void OnClipCheckBox();
   void CheckClip();
   bool CanApply() override;

private:
   double mPeak;

   double mRatio;
   double mRatioClip;   // maximum value of mRatio which does not cause clipping
   double mAmp;
   double mNewPeak;
   bool mCanClip;

   wxSlider *mAmpS;
   wxTextCtrl *mAmpT;
   wxTextCtrl *mNewPeakT;

   CapturedParameters mParameters;
   CapturedParameters mBatchParameters;
   CapturedParameters &Parameters() override {
      // Parameters differ depending on batch mode.  Option to disable clipping
      // is interactive only.
      return IsBatchProcessing() ? mBatchParameters : mParameters;
   }

   DECLARE_EVENT_TABLE()
};

#endif // __AUDACITY_EFFECT_AMPLIFY__
