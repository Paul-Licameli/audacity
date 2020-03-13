/**********************************************************************

  Audacity: A Digital Audio Editor

  EditToolBar.cpp

  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

  See EditToolBar.h for details

*******************************************************************//*!

\class EditToolBar
\brief A ToolBar that has the edit buttons on it.

  This class, which is a child of Toolbar, creates the
  window containing interfaces to commonly-used edit
  functions that are otherwise only available through
  menus. The window can be embedded within a normal project
  window, or within a ToolBarFrame.

  All of the controls in this window were custom-written for
  Audacity - they are not native controls on any platform -
  however, it is intended that the images could be easily
  replaced to allow "skinning" or just customization to
  match the look and feel of each platform.

*//*******************************************************************/



#include "EditToolBar.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros

#ifndef WX_PRECOMP
#include <wx/tooltip.h>
#endif

#include "../AllThemeResources.h"
#include "../BatchCommands.h"
#include "../ImageManipulation.h"
#include "../Menus.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../UndoManager.h"
#include "../prefs/TracksBehaviorsPrefs.h"
#include "../widgets/AButton.h"

#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"

IMPLEMENT_CLASS(EditToolBar, ToolBar);

const int BUTTON_WIDTH = 27;
const int SEPARATOR_WIDTH = 14;

////////////////////////////////////////////////////////////
/// Methods for EditToolBar
////////////////////////////////////////////////////////////

//Standard constructor
EditToolBar::EditToolBar( AudacityProject &project )
: ToolBar(project, EditBarID, XO("Edit"), L"Edit")
{
}

EditToolBar::~EditToolBar()
{
}

void EditToolBar::Create(wxWindow * parent)
{
   ToolBar::Create(parent);
   UpdatePrefs();
}

void EditToolBar::AddSeparator()
{
   AddSpacer();
}

/// This is a convenience function that allows for button creation in
/// MakeButtons() with fewer arguments
/// Very similar to code in ControlToolBar...
AButton *EditToolBar::AddButton(
   EditToolBar *pBar,
   teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
   int id,
   const TranslatableString &label,
   bool toggle,
   std::function< void() > action )
{
   AButton *&r = pBar->mButtons[id];

   r = ToolBar::MakeButton(pBar,
      bmpRecoloredUpSmall, bmpRecoloredDownSmall, bmpRecoloredUpHiliteSmall, bmpRecoloredHiliteSmall,
      eEnabledUp, eEnabledDown, eDisabled,
      wxWindowID(id+first_ETB_ID),
      wxDefaultPosition,
      toggle,
      theTheme.ImageSize( bmpRecoloredUpSmall ), std::move( action ) );

   r->SetLabel(label);
// JKC: Unlike ControlToolBar, does not have a focus rect.  Shouldn't it?
// r->SetFocusRect( r->GetRect().Deflate( 4, 4 ) );

   pBar->Add( r, 0, wxALIGN_CENTER );

   return r;
}

void EditToolBar::Populate()
{
   SetBackgroundColour( theTheme.Colour( clrMedium  ) );
   MakeButtonBackgroundsSmall();

   /* Buttons */
   // Tooltips slightly more verbose than the menu entries are.

   struct Entry {
      TranslatableString label;

      teBmps enabledUp, disabled;
      teBmps enabledDown;

      bool enable;

      Entry( const TranslatableString &label, teBmps enabledUp, teBmps disabled,
         bool enable = true, teBmps enabledDown = -1 )
      : label{label}, enabledUp{enabledUp}, disabled{disabled}, enable{enable}
      , enabledDown{enabledDown}
      {
         if (enabledDown == -1)
            enabledDown = enabledUp;
      }

      bool isToggle() const { return enabledDown != enabledUp; }
   };
   using Section = std::vector< Entry >;
   static const Section table[] = {
      {
{ XO("Cut selection"),                bmpCut,         bmpCutDisabled },
{ XO("Copy selection"),               bmpCopy,        bmpCopyDisabled },
{ XO("Paste"),                        bmpPaste,       bmpPasteDisabled,      false },
{ XO("Trim audio outside selection"), bmpTrim,        bmpTrimDisabled },
{ XO("Silence audio selection"),      bmpSilence,     bmpSilenceDisabled },
      },

      {
{ XO("Undo"),                         bmpUndo,        bmpUndoDisabled },
{ XO("Redo"),                         bmpRedo,        bmpRedoDisabled },
      },

#ifdef OPTION_SYNC_LOCK_BUTTON
      {
         // Toggle button
{ XO("Sync-Lock Tracks"), bmpSyncLockTracksUp, bmpSyncLockTracksUp, true, bmpSyncLockTracksDown },
      },
#endif

      {
{ XO("Zoom In"),                      bmpZoomIn,      bmpZoomInDisabled,     false },
{ XO("ZoomOut"),                      bmpZoomOut,     bmpZoomOutDisabled,    false },
{ XO("Zoom to Selection"),            bmpZoomSel,     bmpZoomSelDisabled,    false },
{ XO("Fit to Width"),                 bmpZoomFit,     bmpZoomFitDisabled,    false },
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
{ XO("Zoom Toggle"),                  bmpZoomToggle,  bmpZoomToggleDisabled, false },
#endif
      },

#if defined(EXPERIMENTAL_EFFECTS_RACK)
      {
{ XO("Show Effects Rack"),            bmpEditEffects, bmpEditEffects },
      },
#endif
   };

   int ii = 0;
   for ( const auto &section : table ) {
      if ( ii > 0 )
         AddSeparator();
      for ( const auto &entry : section ) {
         AddButton( this, entry.enabledUp, entry.enabledDown, entry.disabled,
            ii, entry.label, entry.isToggle(), [this, ii]{ OnButton( ii ); } )
               ->SetEnabled( entry.enable );
         ++ii;
      }
   }

#ifdef OPTION_SYNC_LOCK_BUTTON
   mButtons[ETBSyncLockID]->PushDown();
#endif

   RegenerateTooltips();
}

void EditToolBar::UpdatePrefs()
{
   RegenerateTooltips();

   // Set label to pull in language change
   SetLabel(XO("Edit"));

   // Give base class a chance
   ToolBar::UpdatePrefs();
}

void EditToolBar::RegenerateTooltips()
{
   ForAllButtons( ETBActTooltips );
}

void EditToolBar::EnableDisableButtons()
{
   ForAllButtons( ETBActEnableDisable );
}


static const struct Entry {
   int tool;
   CommandID commandName;
   TranslatableString untranslatedLabel;
} EditToolbarButtonList[] = {
   { ETBCutID,      L"Cut",         XO("Cut")  },
   { ETBCopyID,     L"Copy",        XO("Copy")  },
   { ETBPasteID,    L"Paste",       XO("Paste")  },
   { ETBTrimID,     L"Trim",        XO("Trim audio outside selection")  },
   { ETBSilenceID,  L"Silence",     XO("Silence audio selection")  },
   { ETBUndoID,     L"Undo",        XO("Undo")  },
   { ETBRedoID,     L"Redo",        XO("Redo")  },

#ifdef OPTION_SYNC_LOCK_BUTTON
   { ETBSyncLockID, L"SyncLock",    XO("Sync-Lock Tracks")  },
#endif

   { ETBZoomInID,   L"ZoomIn",      XO("Zoom In")  },
   { ETBZoomOutID,  L"ZoomOut",     XO("Zoom Out")  },
#ifdef EXPERIMENTAL_ZOOM_TOGGLE_BUTTON
   { ETBZoomToggleID,   L"ZoomToggle",      XO("Zoom Toggle")  },
#endif 
   { ETBZoomSelID,  L"ZoomSel",     XO("Fit selection to width")  },
   { ETBZoomFitID,  L"FitInWindow", XO("Fit project to width")  },

#if defined(EXPERIMENTAL_EFFECTS_RACK)
   { ETBEffectsID,  L"ShowEffectsRack", XO("Open Effects Rack")  },
#endif
};


void EditToolBar::ForAllButtons(int Action)
{
   AudacityProject *p;
   CommandManager* cm = nullptr;

   if( Action & ETBActEnableDisable ){
      p = &mProject;
      cm = &CommandManager::Get( *p );
#ifdef OPTION_SYNC_LOCK_BUTTON
      bool bSyncLockTracks = TracksBehaviorsSyncLockTracks.Read();

      if (bSyncLockTracks)
         mButtons[ETBSyncLockID]->PushDown();
      else
         mButtons[ETBSyncLockID]->PopUp();
#endif
   }


   for (const auto &entry : EditToolbarButtonList) {
#if wxUSE_TOOLTIPS
      if( Action & ETBActTooltips ){
         ComponentInterfaceSymbol command{
            entry.commandName, entry.untranslatedLabel };
         ToolBar::SetButtonToolTip( mProject,
            *mButtons[entry.tool], &command, 1u );
      }
#endif
      if (cm) {
         mButtons[entry.tool]->SetEnabled(cm->GetEnabled(entry.commandName));
      }
   }
}

void EditToolBar::OnButton( size_t id )
{
   // Be sure the pop-up happens even if there are exceptions, except for buttons which toggle.
   auto cleanup = finally( [&] { mButtons[id]->InteractionOver();});

   AudacityProject *p = &mProject;
   auto &cm = CommandManager::Get( *p );

   auto flags = MenuManager::Get(*p).GetUpdateFlags();
   const CommandContext context( *p );
   MacroCommands::HandleTextualCommand( cm,
      EditToolbarButtonList[id].commandName, context, flags, false);

#if defined(__WXMAC__)
   // Bug 2402
   // LLL: It seems that on the Mac the IDLE events are processed
   //      differently than on Windows/GTK and the AdornedRulerPanel's
   //      OnPaint() method gets called sooner that expected. This is
   //      evident when zooming from this toolbar only. When zooming from
   //      the Menu or from keyboard ommand, the zooming works correctly.
   wxTheApp->ProcessIdle();
#endif
}

static RegisteredToolbarFactory factory{ EditBarID,
   []( AudacityProject &project ){
      return ToolBar::Holder{ safenew EditToolBar{ project } }; }
};

#include "ToolManager.h"

namespace {
AttachedToolBarMenuItem sAttachment{
   /* i18n-hint: Clicking this menu item shows the toolbar for editing */
   EditBarID, L"ShowEditTB", XXO("&Edit Toolbar")
};
}

