/**********************************************************************

   Audacity: A Digital Audio Editor
   Audacity(R) is copyright (c) 1999-2008 Audacity Team.
   License: GPL v2.  See License.txt.

   ScoreAlignDialog.cpp
   <TODO: authors>

******************************************************************//**

\class ScoreAlignDialog
\brief ScoreAlignDialog is \TODO.

It \TODO: description

*//*******************************************************************/


#include "ScoreAlignDialog.h"

#ifdef EXPERIMENTAL_SCOREALIGN

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/brush.h>
#include <wx/file.h>
#include <wx/statusbr.h>
#endif

#include <fstream>
#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../lib-src/header-substitutes/allegro.h"
#include "audioreader.h"
#include "scorealign.h"
#include "scorealign-glue.h"

static std::unique_ptr<ScoreAlignDialog> gScoreAlignDialog{};

//IMPLEMENT_CLASS(ScoreAlignDialog, wxDialogWrapper)

ScoreAlignDialog::ScoreAlignDialog(ScoreAlignParams &params)
   : wxDialogWrapper(NULL, -1, XO("Align MIDI to Audio"),
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE)
{
   gScoreAlignDialog.reset(this); // Allows anyone to close dialog by calling
                             // CloseScoreAlignDialog()
   gPrefs->Read(L"/Tracks/Synchronize/FramePeriod", &p.mFramePeriod,
                float(SA_DFT_FRAME_PERIOD));
   gPrefs->Read(L"/Tracks/Synchronize/WindowSize", &p.mWindowSize,
                float(SA_DFT_WINDOW_SIZE));
   gPrefs->Read(L"/Tracks/Synchronize/SilenceThreshold",
                &p.mSilenceThreshold, float(SA_DFT_SILENCE_THRESHOLD));
   gPrefs->Read(L"/Tracks/Synchronize/ForceFinalAlignment",
                &p.mForceFinalAlignment, float(SA_DFT_FORCE_FINAL_ALIGNMENT));
   gPrefs->Read(L"/Tracks/Synchronize/IgnoreSilence",
                &p.mIgnoreSilence, float(SA_DFT_IGNORE_SILENCE));
   gPrefs->Read(L"/Tracks/Synchronize/PresmoothTime", &p.mPresmoothTime,
                float(SA_DFT_PRESMOOTH_TIME));
   gPrefs->Read(L"/Tracks/Synchronize/LineTime", &p.mLineTime,
                float(SA_DFT_LINE_TIME));
   gPrefs->Read(L"/Tracks/Synchronize/SmoothTime", &p.mSmoothTime,
                float(SA_DFT_SMOOTH_TIME));

   //wxButton *ok = safenew wxButton(this, wxID_OK, _("OK"));
   //wxButton *cancel = safenew wxButton(this, wxID_CANCEL, _("Cancel"));
   //wxSlider *sl = safenew wxSliderWrapper(this, ID_SLIDER, 0, 0, 100,
   //                     wxDefaultPosition, wxSize(20, 124),
   //                     wxSL_HORIZONTAL);

   using namespace DialogDefinition;
   auto S = ShuttleGui{ this };
   //ok->SetDefault();

   S.StartVerticalLay(true);
   S.StartStatic(XO("Align MIDI to Audio"));
   S.StartMultiColumn(3, wxEXPAND | wxALIGN_CENTER_VERTICAL);
      GroupOptions{}.StretchyColumn(1));

   S
      .AddVariableText(
         XO("Frame Period:"), true, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

   S
      .Text(XO("Frame Period"))
      .Style(wxSL_HORIZONTAL)
      .MinSize( { 300, -1 } )
      .Target( Scale( p.mFramePeriod, 100 ) )
      .AddSlider( {},
       /*pos*/ (int) (p.mFramePeriod * 100 + 0.5), /*max*/ 50, /*min*/ 5);

   S
      .VariableText( [this]{ return Label(
         XO("%.2f secs").Format( p.mFramePeriod )
      ); } )
      .AddVariableText(
         SA_DFT_FRAME_PERIOD_TEXT, true, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

   S
      .AddVariableText(
         XO("Window Size:"), true, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

   S
      .Text(XO("Window Size"))
      .Style(wxSL_HORIZONTAL)
      .Target( Scale( p.mWindowSize, 100 ) )
      .AddSlider( {},
       /*pos*/ (int) (p.mWindowSize * 100 + 0.5), /*max*/ 100, /*min*/ 5);

   S
      .VariableText( [this]{ return Label(
         XO("%.2f secs").Format( p.mWindowSize )
      ); } )
      .AddVariableText(
         SA_DFT_WINDOW_SIZE_TEXT, true, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

   S
      .Text (XO("Force Final Alignment"))
      .Target( p.mForceFinalAlignment )
      .AddCheckBox(
                XO("Force Final Alignment"),
                p.mForceFinalAlignment);

   S
      .Text(XO("Ignore Silence at Beginnings and Endings"))
      .Target( p.mIgnoreSilence )
      .AddCheckBox(
         XO("Ignore Silence at Beginnings and Endings"),
         p.mIgnoreSilence );

   // need a third column after checkboxes:
   S
      .AddVariableText({}, true, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

   S
      .AddVariableText(XO("Silence Threshold:"),
         true, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

   S
      .Text(XO("Silence Threshold"))
      .Style(wxSL_HORIZONTAL)
      .Target( Scale( p.mSilenceThreshold, 1000 ) )
      .AddSlider( {},
         /*pos*/ (int) (p.mSilenceThreshold * 1000 + 0.5), /*max*/ 500);

   S
      .VariableText( [this]{ return Label(
         Verbatim("%.3f").Format( p.mSilenceThreshold )
      ); } )
      .AddVariableText(
         SA_DFT_SILENCE_THRESHOLD_TEXT,
         true, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

   S
      .AddVariableText(
      /* i18n-hint: The English would be clearer if it had 'Duration' rather than 'Time'
         This is a NEW experimental effect, and until we have it documented in the user
         manual we don't have a clear description of what this parameter does.
         It is OK to leave it in English. */
         XO("Presmooth Time:"), true, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );

   S
   /* i18n-hint: The English would be clearer if it had 'Duration' rather than 'Time'
      This is a NEW experimental effect, and until we have it documented in the user
      manual we don't have a clear description of what this parameter does.
      It is OK to leave it in English. */
      .Text(XO("Presmooth Time"))
      .Style(wxSL_HORIZONTAL)
      .Target( Scale( p.mPresmoothTime, 100 ) )
      .AddSlider( {},
               /*pos*/ (int) (p.mPresmoothTime * 100 + 0.5), /*max*/ 500);

   S
      .VariableText( [this]{ return Label(
         p.mPresmoothTime > 0
            ? XO("%.2f secs").Format( p.mPresmoothTime )
            /* i18n-hint as in "turned off" */
            : XO("(off)")
      ); } )
      .AddVariableText(
         SA_DFT_PRESMOOTH_TIME_TEXT, true, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

   S
      /* i18n-hint: The English would be clearer if it had 'Duration' rather than 'Time'
         This is a NEW experimental effect, and until we have it documented in the user
         manual we don't have a clear description of what this parameter does.
         It is OK to leave it in English. */
      .AddVariableText(XO("Line Time:"), true,
         wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

   S
   /* i18n-hint: The English would be clearer if it had 'Duration' rather than 'Time'
      This is a NEW experimental effect, and until we have it documented in the user
      manual we don't have a clear description of what this parameter does.
      It is OK to leave it in English. */
      .Text(XO("Line Time"))
      .Style(wxSL_HORIZONTAL)
      .Target( Scale( p.mLineTime, 100 ) )
      .AddSlider( {},
                    /*pos*/ (int) (p.mLineTime * 100 + 0.5), /*max*/ 500);

   S
      .VariableText( [this]{ return Label(
         p.mLineTime > 0
            ? XO("%.2f secs").Format( p.mLineTime )
            /* i18n-hint as in "turned off" */
            : XO("(off)")
      ); } )
      .AddVariableText(
         SA_DFT_LINE_TIME_TEXT, true, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

   S
      .AddVariableText(
      /* i18n-hint: The English would be clearer if it had 'Duration' rather than 'Time'
         This is a NEW experimental effect, and until we have it documented in the user
         manual we don't have a clear description of what this parameter does.
         It is OK to leave it in English. */
         XO("Smooth Time:"), true, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL );

   S
   /* i18n-hint: The English would be clearer if it had 'Duration' rather than 'Time'
      This is a NEW experimental effect, and until we have it documented in the user
      manual we don't have a clear description of what this parameter does.
      It is OK to leave it in English. */
      .Text(XO("Smooth Time"))
      .Style(wxSL_HORIZONTAL)
      .Target( Scale( p.mSmoothTime, 100 ) )
      .AddSlider( {},
                  /*pos*/ (int) (p.mSmoothTime * 100 + 0.5), /*max*/ 500);

   S
      .VariableText( [this]{ return Label(
         XO("%.2f secs").Format( p.mSmoothTime )
      ); } )
      .AddVariableText(
         SA_DFT_SMOOTH_TIME_TEXT, true, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

   S.EndMultiColumn();
   S.EndStatic();

   S
      .AddStandardButtons(0, {
            S.Item( eOkButton ).Action( [this]{ OnOK ),
            S.Item( eCancelButton ).Action( [this]{ OnCancel )
         },
         safenew wxButton( this, ID_DEFAULT ),
         S
            .Item()
            .Text({ XO("Restore Defaults"), {}, {}, XO("Use Defaults") })
            .Action( [this]{ OnDefault ) );

   S.EndVerticalLay();
   Layout();
   Fit();
   Center();

   TransferDataFromWindow(); // set labels according to actual initial values

   params.mStatus = p.mStatus = ShowModal();

   if (p.mStatus == wxID_OK) {
       // Retain the settings
       gPrefs->Write(L"/Tracks/Synchronize/FramePeriod", p.mFramePeriod);
       gPrefs->Write(L"/Tracks/Synchronize/WindowSize", p.mWindowSize);
       gPrefs->Write(L"/Tracks/Synchronize/SilenceThreshold",
                     p.mSilenceThreshold);
       gPrefs->Write(L"/Tracks/Synchronize/ForceFinalAlignment",
                     p.mForceFinalAlignment);
       gPrefs->Write(L"/Tracks/Synchronize/IgnoreSilence",
                     p.mIgnoreSilence);
       gPrefs->Write(L"/Tracks/Synchronize/PresmoothTime",
                     p.mPresmoothTime);
       gPrefs->Write(L"/Tracks/Synchronize/LineTime", p.mLineTime);
       gPrefs->Write(L"/Tracks/Synchronize/SmoothTime", p.mSmoothTime);
       gPrefs->Flush();

       params = p; // return all parameters through params
   }
}

ScoreAlignDialog::~ScoreAlignDialog()
{
}


//void ScoreAlignDialog::OnOK()
//{
//   EndModal(wxID_OK);
//}

//void ScoreAlignDialog::OnCancel()
//{
//   EndModal(wxID_CANCEL);
//}

void ScoreAlignDialog::OnDefault()
{
   p.mFramePeriod = SA_DFT_FRAME_PERIOD;
   p.mWindowSize = SA_DFT_WINDOW_SIZE;
   p.mSilenceThreshold = SA_DFT_SILENCE_THRESHOLD;
   p.mPresmoothTime = SA_DFT_PRESMOOTH_TIME;
   p.mLineTime = SA_DFT_LINE_TIME;
   p.mSmoothTime = SA_DFT_SMOOTH_TIME;

   p.mForceFinalAlignment = SA_DFT_FORCE_FINAL_ALIGNMENT;
   p.mIgnoreSilence = SA_DFT_IGNORE_SILENCE;
}

void CloseScoreAlignDialog()
{
   gScoreAlignDialog.reset();
}


#endif
