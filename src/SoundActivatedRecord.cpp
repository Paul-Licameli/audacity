/**********************************************************************

  Audacity: A Digital Audio Editor

  SoundActivatedRecord.cpp

  Martyn Shaw

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

********************************************************************//**

\class SoundActivatedRecordDialog
\brief Configures sound activated recording.

*//********************************************************************/


#include "SoundActivatedRecord.h"

#include "ShuttleGui.h"
#include "Prefs.h"
#include "prefs/GUIPrefs.h"
#include "prefs/RecordingPrefs.h"

SoundActivatedRecordDialog::SoundActivatedRecordDialog(wxWindow* parent)
: wxDialogWrapper(parent, -1, XO("Sound Activated Record"), wxDefaultPosition,
           wxDefaultSize, wxCAPTION )
//           wxDefaultSize, wxCAPTION | wxTHICK_FRAME)
{
   SetName();
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   Fit();
   Center();
}

SoundActivatedRecordDialog::~SoundActivatedRecordDialog()
{
}

void SoundActivatedRecordDialog::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(5);

   S.StartVerticalLay();
   {
      S.StartMultiColumn(2, wxEXPAND);
      S.SetStretchyCol(1);

      S
         .TieSlider(
            XXO("Activation level (dB):"),
            AudioIOSilenceLevel,
            0, -GUIdBRange.Read() )
            ->SetMinSize(wxSize(300, wxDefaultCoord));

      S.EndMultiColumn();
   }
   S.EndVerticalLay();

   S
      .AddStandardButtons( eCancelButton, {
         S.Item( eOkButton ).Action( [this]{ OnOK(); } )
      });
}

void SoundActivatedRecordDialog::OnOK()
{
   wxDialog::TransferDataFromWindow();
   ShuttleGui S( this, eIsSavingToPrefs );
   PopulateOrExchange( S );

   gPrefs->Flush();

   EndModal(0);
}

