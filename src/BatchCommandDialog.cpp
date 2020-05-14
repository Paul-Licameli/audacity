/**********************************************************************

  Audacity: A Digital Audio Editor

  BatchCommandDialog.cpp

  Dominic Mazzoni
  James Crook

*******************************************************************//*!

\class MacroCommandDialog
\brief Provides a list of configurable commands for use with MacroCommands

Provides a list of commands, mostly effects, which can be chained
together in a simple linear sequence.  Can configure parameters on each
selected command.

*//*******************************************************************/


#include "BatchCommandDialog.h"

#ifdef __WXMSW__
    #include  <wx/ownerdrw.h>
#endif

//
#include <wx/defs.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/listctrl.h>


#include "Project.h"
#include "effects/EffectManager.h"
#include "ShuttleGui.h"
#include "widgets/HelpSystem.h"


#define CommandsListID        7001

BEGIN_EVENT_TABLE(MacroCommandDialog, wxDialogWrapper)
   EVT_LIST_ITEM_ACTIVATED(CommandsListID, MacroCommandDialog::OnItemSelected)
   EVT_LIST_ITEM_SELECTED(CommandsListID,  MacroCommandDialog::OnItemSelected)
END_EVENT_TABLE();

MacroCommandDialog::MacroCommandDialog(
   wxWindow * parent, wxWindowID id, AudacityProject &project):
   wxDialogWrapper(parent, id, XO("Select Command"),
            wxDefaultPosition, wxDefaultSize,
            wxCAPTION | wxRESIZE_BORDER)
   , mCatalog{ &project }
{
   SetLabel(XO("Select Command"));         // Provide visual label
   SetName(XO("Select Command"));          // Provide audible label
   Populate();
}

void MacroCommandDialog::Populate()
{
   //------------------------- Main section --------------------
   ShuttleGui S(this);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void MacroCommandDialog::PopulateOrExchange(ShuttleGui &S)
{
   S.StartVerticalLay(true);
   {
      S.StartMultiColumn(4,
                         GroupOptions{}.Position(wxEXPAND).StretchyColumn(1));
      {
         S
            .AddTextBox(XXO("&Command"), L"", 20)
            .Initialize( &wxTextCtrl::SetEditable, false )
            .Assign(mCommand);

         S
            .Enable( [this]{
               return !mPluginID.empty();
            } )
            .Action( [this]{ OnEditParams(); } )
            .AddButton(XXO("&Edit Parameters"));

         S
            .Enable( [this]{
               // If ID is empty, then the effect wasn't found, in which case,
               // the user must have
               // selected one of the "special" commands.
               return mHasPresets;
            } )
            .Action( [this]{ OnUsePreset(); } )
            .AddButton(XXO("&Use Preset"));
      }
      S.EndMultiColumn();

      S.StartMultiColumn(2,
                         GroupOptions{}.Position(wxEXPAND).StretchyColumn(1));
      {
         S
            .AddTextBox(XXO("&Parameters"), L"", 0)
            .Initialize( &wxTextCtrl::SetEditable, false )
            .Assign(mParameters);

         auto prompt = XXO("&Details");

         S
            .Prop(0)
            .AddPrompt(prompt);

         S
            .Text( prompt.Stripped() )
            .AddTextWindow( L"")
            .Initialize( &wxTextCtrl::SetEditable, false );
      }
      S.EndMultiColumn();

      S
         .Prop(10)
         .StartStatic(XO("Choose command"), true);
      {
         S
            .Id(CommandsListID)
            .Style(wxSUNKEN_BORDER | wxLC_LIST | wxLC_SINGLE_SEL)
            .AddListControl()
            .Assign(mChoices);
      }
      S.EndStatic();
   }
   S.EndVerticalLay();

   S
      .AddStandardButtons( 0, {
         S.Item( eOkButton ).Action( [this]{ OnOk(); } ),
         S.Item( eCancelButton ).Action( [this]{ OnCancel(); } ),
         S.Item( eHelpButton ).Action( [this]{ OnHelp(); } )
      } );

   PopulateCommandList();
   if (mChoices->GetItemCount() > 0) {
      // set first item to be selected (and the focus when the
      // list first becomes the focus)
      mChoices->SetItemState(0, wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED,
         wxLIST_STATE_FOCUSED | wxLIST_STATE_SELECTED);
   }

   SetMinSize(wxSize(780, 560));
   Fit();
   Center();
}

void MacroCommandDialog::PopulateCommandList()
{
   mChoices->DeleteAllItems();
   long ii = 0;
   for ( const auto &entry : mCatalog )
      // insert the user-facing string
      mChoices->InsertItem( ii++, entry.name.Translation() );
}

void MacroCommandDialog::ValidateChoices()
{
}

void MacroCommandDialog::OnChoice(wxCommandEvent & WXUNUSED(event))
{
}

void MacroCommandDialog::OnOk()
{
   mSelectedCommand = mInternalCommandName
      // .Strip(wxString::both) // PRL: used to do this, here only,
         // but ultimately mSelectedCommand is looked up in the catalog without
         // similar adjustment of whitespace in the comparison
   ;
   mSelectedParameters = mParameters->GetValue().Strip(wxString::trailing);
   EndModal(true);
}

void MacroCommandDialog::OnCancel()
{
   EndModal(false);
}

void MacroCommandDialog::OnHelp()
{
   auto page = GetHelpPageName();
   HelpSystem::ShowHelp(this, page, true);
}

void MacroCommandDialog::OnItemSelected(wxListEvent &event)
{
   const auto &command = mCatalog[ event.GetIndex() ];

   EffectManager & em = EffectManager::Get();
   PluginID ID = em.GetEffectByIdentifier( command.name.Internal() );
   mPluginID = ID;
   mHasPresets = em.HasPresets( mPluginID );

   auto value = command.name.Translation();
   if ( value == mCommand->GetValue() )
      // This uses the assumption of uniqueness of translated names!
      return;

   mCommand->SetValue(value);
   mInternalCommandName = command.name.Internal();

   auto params = MacroCommands::GetCurrentParamsFor(mInternalCommandName);
   if (params.empty())
   {
      params = em.GetDefaultPreset(ID);
   }

   // using GET to expose a CommandID to the user!
   // Cryptic command and category.
   // Later we can put help information there, perhaps.
   // Macro command details are one place that we do expose Identifier
   // to (more sophisticated) users
   mDetails->SetValue(
      mInternalCommandName.GET() + "\r\n" + command.category.Translation()  );
   mParameters->SetValue(params);
}

void MacroCommandDialog::OnEditParams()
{
   auto command = mInternalCommandName;
   auto params  = mParameters->GetValue();

   params = MacroCommands::PromptForParamsFor(command, params, *this).Trim();

   mParameters->SetValue(params);
   mParameters->Refresh();
}

void MacroCommandDialog::OnUsePreset()
{
   auto command = mInternalCommandName;
   auto params  = mParameters->GetValue();

   auto preset = MacroCommands::PromptForPresetFor(command, params, this).Trim();

   mParameters->SetValue(preset);
   mParameters->Refresh();
}

void MacroCommandDialog::SetCommandAndParams(const CommandID &Command, const wxString &Params)
{
   auto iter = mCatalog.ByCommandId( Command );

   mParameters->SetValue( Params );

   mInternalCommandName = Command;
   if (iter == mCatalog.end()) {
      // uh oh, using GET to expose an internal name to the user!
      // in default of any better friendly name
      mPluginID = PluginID{};
      mHasPresets = false;
      mCommand->SetValue( Command.GET() );
   }
   else {
      mCommand->SetValue( iter->name.Translation() );
      // using GET to expose a CommandID to the user!
      // Macro command details are one place that we do expose Identifier
      // to (more sophisticated) users
      mDetails->SetValue(
         iter->name.Internal() + "\r\n" + iter->category.Translation()  );
      mChoices->SetItemState(iter - mCatalog.begin(),
                             wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

      EffectManager & em = EffectManager::Get();
      mPluginID = em.GetEffectByIdentifier(Command);
      mHasPresets = em.HasPresets( mPluginID );
   }
}
