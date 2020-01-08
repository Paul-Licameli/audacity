/**********************************************************************

  Audacity: A Digital Audio Editor

  Silence.cpp

  Dominic Mazzoni

*******************************************************************//**

\class EffectSilence
\brief An effect to add silence.

*//*******************************************************************/

#include "../Audacity.h"
#include "Silence.h"

#include <wx/intl.h>

#include "../ShuttleGui.h"
#include "../WaveTrack.h"
#include "../widgets/NumericTextCtrl.h"

EffectSilence::EffectSilence()
{
   SetLinearEffectFlag(true);
}

EffectSilence::~EffectSilence()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectSilence::GetSymbol()
{
   return SILENCE_PLUGIN_SYMBOL;
}

TranslatableString EffectSilence::GetDescription()
{
   return XO("Creates audio of zero amplitude");
}

wxString EffectSilence::ManualPage()
{
   return wxT("Silence");
}


// EffectDefinitionInterface implementation

EffectType EffectSilence::GetType()
{
   return EffectTypeGenerate;
}

// Effect implementation

void EffectSilence::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay();
      {
         S.AddPrompt(XO("Duration:"));
         mDurationT = safenew
            NumericTextCtrl(S.GetParent(), wxID_ANY,
                              NumericConverter::TIME,
                              GetDurationFormat(),
                              GetDuration(),
                               context.projectRate,
                               NumericTextCtrl::Options{}
                                  .AutoPos(true));
         S.Name(XO("Duration"))
            .Position(wxALIGN_CENTER | wxALL)
            .AddWindow(mDurationT);
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   return;
}

bool EffectSilence::TransferDataToWindow()
{
   mDurationT->SetValue(GetDuration());

   return true;
}

bool EffectSilence::TransferDataFromWindow()
{
   SetDuration(mDurationT->GetValue());

   return true;
}

bool EffectSilence::GenerateTrack(WaveTrack *tmp,
                                  const WaveTrack & WXUNUSED(track),
                                  int WXUNUSED(ntrack))
{
   tmp->InsertSilence(0.0, GetDuration());
   return true;
}
