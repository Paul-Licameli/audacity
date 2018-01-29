/**********************************************************************

  Audacity: A Digital Audio Editor

  Warning.cpp

  Dominic Mazzoni

*******************************************************************//**

\class WarningDialog
\brief Gives a warning message, that can be dismissed, with crucially
the ability to not see similar warnings again for this session.

*//********************************************************************/




#include "Warning.h"

#include "../ShuttleGui.h"

#include <wx/artprov.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include "wxPanelWrapper.h"

class WarningDialog final : public wxDialogWrapper
{
 public:
   // constructors and destructors
   WarningDialog(wxWindow *parent,
                 const TranslatableString &message,
                 const TranslatableLabel &footer,
                 bool showCancelButton);

 private:
   void OnOK();

   wxCheckBox *mCheckBox;
};

const TranslatableLabel &DefaultWarningFooter()
{
   static auto result = XXO("Don't show this warning again");
   return result;
}

WarningDialog::WarningDialog(wxWindow *parent, const TranslatableString &message,
                             const TranslatableLabel &footer,
                             bool showCancelButton)
:  wxDialogWrapper(parent, wxID_ANY, XO("Warning"),
            wxDefaultPosition, wxDefaultSize,
            (showCancelButton ? wxDEFAULT_DIALOG_STYLE : wxCAPTION | wxSYSTEM_MENU)) // Unlike wxDEFAULT_DIALOG_STYLE, no wxCLOSE_BOX.
{
   SetName();

   SetIcon(wxArtProvider::GetIcon(wxART_WARNING, wxART_MESSAGE_BOX));
   ShuttleGui S{ this };

   S.StartVerticalLay(false, 10);
   {
      S
         .AddFixedText(message);

      S
         .AddCheckBox(footer, false)
         .Assign(mCheckBox);
   }
   S.EndVerticalLay();

   S
      .AddStandardButtons(showCancelButton ? eCancelButton : 0, {
         S.Item( eOkButton ).Action( [this]{ OnOK(); } )
      } );

   Layout();
   GetSizer()->Fit(this);
   CentreOnParent();
}

void WarningDialog::OnOK()
{
   EndModal(mCheckBox->GetValue() ? wxID_NO : wxID_YES); // return YES, if message should be shown again
}

int ShowWarningDialog(wxWindow *parent,
                      BoolSetting &setting,
                      const TranslatableString &message,
                      bool showCancelButton,
                      const TranslatableLabel &footer)
{
   if ( ! setting.Read() )
      return wxID_OK;

   WarningDialog dlog(parent, message, footer, showCancelButton);

   int retCode = dlog.ShowModal();
   if (retCode == wxID_CANCEL)
      return retCode;

   setting.Write( retCode == wxID_YES );
   gPrefs->Flush();
   return wxID_OK;
}
