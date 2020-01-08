/**********************************************************************

  Audacity: A Digital Audio Editor

  Normalize.h

  Dominic Mazzoni
  Vaughan Johnson (Preview)

**********************************************************************/

#ifndef __AUDACITY_EFFECT_NORMALIZE__
#define __AUDACITY_EFFECT_NORMALIZE__

#include "../Experimental.h"

#include "Effect.h"
#include "Biquad.h"

class wxCheckBox;
class wxStaticText;
class wxTextCtrl;
class ShuttleGui;

#define NORMALIZE_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Normalize") }

class EffectNormalize final : public Effect
{
public:
   EffectNormalize();
   virtual ~EffectNormalize();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   bool DefineParams( ShuttleParams & S ) override;
   bool GetAutomationParameters(CommandParameters & parms) override;
   bool SetAutomationParameters(CommandParameters & parms) override;

   // Effect implementation

   bool CheckWhetherSkipEffect() override;
   bool Startup() override;
   bool Process( EffectContext &context ) override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

private:
   // EffectNormalize implementation

   enum AnalyseOperation
   {
      ANALYSE_DC, ANALYSE_LOUDNESS, ANALYSE_LOUDNESS_DC
   };

   bool ProcessOne( const EffectContext &context,
      WaveTrack * t, const TranslatableString &msg, double& progress, float offset);
   bool AnalyseTrack(const WaveTrack * track, const TranslatableString &msg,
                     double &progress, float &offset, float &extent);
   bool AnalyseTrackData(const WaveTrack * track, const TranslatableString &msg, double &progress,
                     AnalyseOperation op, float &offset);
   void AnalyseDataDC(float *buffer, size_t len);
   void ProcessData(float *buffer, size_t len, float offset);

   void OnUpdateUI(wxCommandEvent & evt);
   void UpdateUI();

private:
   double mPeakLevel;
   bool   mGain;
   bool   mDC;
   bool   mStereoInd;

   double mCurT0;
   double mCurT1;
   float  mMult;
   double mSum;
   sampleCount    mCount;

   wxCheckBox *mGainCheckBox;
   wxCheckBox *mDCCheckBox;
   wxTextCtrl *mLevelTextCtrl;
   wxStaticText *mLeveldB;
   wxStaticText *mWarning;
   wxCheckBox *mStereoIndCheckBox;
   bool mCreating;


   DECLARE_EVENT_TABLE()
};

#endif
