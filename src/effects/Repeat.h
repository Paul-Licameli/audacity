/**********************************************************************

  Audacity: A Digital Audio Editor

  Repeat.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EFFECT_REPEAT__
#define __AUDACITY_EFFECT_REPEAT__

#include "Effect.h"
#include "../Shuttle.h"

class wxTextCtrl;
class ShuttleGui;

class wxStaticText;

class EffectRepeat final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectRepeat();
   virtual ~EffectRepeat();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // Effect implementation

   bool Process() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;

private:
   // EffectRepeat implementation

   void OnRepeatTextChange();
   void DisplayNewTime();

   bool CanApply() override;

private:
   int repeatCount;

   wxStaticText *mCurrentTime;
   wxStaticText *mTotalTime;

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
};

#endif

