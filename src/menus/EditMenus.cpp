#include "../CommonCommandFlags.h"
#include "../Project.h"
#include "../ProjectHistory.h"
#include "../ProjectWindow.h"
#include "ProjectWindows.h"
#include "../TrackPanelAx.h"
#include "../UndoManager.h"
#include "../WaveTrack.h"
#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../prefs/PrefsDialog.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/VetoDialogHook.h"

namespace EditActions {

// Menu handler functions

struct Handler : CommandHandlerObject {

void OnUndo(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &trackPanel = GetProjectPanel( project );
   auto &undoManager = UndoManager::Get( project );
   auto &window = ProjectWindow::Get( project );

   if (!ProjectHistory::Get( project ).UndoAvailable()) {
      AudacityMessageBox( XO("Nothing to undo") );
      return;
   }

   // can't undo while dragging
   if (trackPanel.IsMouseCaptured()) {
      return;
   }

   undoManager.Undo(
      [&]( const UndoStackElem &elem ){
         ProjectHistory::Get( project ).PopState( elem.state ); } );

   auto t = *tracks.Selected().begin();
   if (!t)
      t = *tracks.Any().begin();
   if (t) {
      TrackFocus::Get(project).Set(t);
      t->EnsureVisible();
   }
}

void OnRedo(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &trackPanel = GetProjectPanel( project );
   auto &undoManager = UndoManager::Get( project );
   auto &window = ProjectWindow::Get( project );

   if (!ProjectHistory::Get( project ).RedoAvailable()) {
      AudacityMessageBox( XO("Nothing to redo") );
      return;
   }
   // Can't redo whilst dragging
   if (trackPanel.IsMouseCaptured()) {
      return;
   }

   undoManager.Redo(
      [&]( const UndoStackElem &elem ){
         ProjectHistory::Get( project ).PopState( elem.state ); } );

   auto t = *tracks.Selected().begin();
   if (!t)
      t = *tracks.Any().begin();
   if (t) {
      TrackFocus::Get(project).Set(t);
      t->EnsureVisible();
   }
}

void OnPreferences(const CommandContext &context)
{
   auto &project = context.project;

   GlobalPrefsDialog dialog(&GetProjectFrame( project ) /* parent */, &project );

   if( ::CallVetoDialogHook( &dialog ) )
      return;

   if (!dialog.ShowModal()) {
      // Canceled
      return;
   }

   // LL:  Moved from PrefsDialog since wxWidgets on OSX can't deal with
   //      rebuilding the menus while the PrefsDialog is still in the modal
   //      state.
   for (auto p : AllProjects{}) {
      MenuManager::Get(*p).RebuildMenuBar(*p);
// TODO: The comment below suggests this workaround is obsolete.
#if defined(__WXGTK__)
      // Workaround for:
      //
      //   http://bugzilla.audacityteam.org/show_bug.cgi?id=458
      //
      // This workaround should be removed when Audacity updates to wxWidgets
      // 3.x which has a fix.
      auto &window = GetProjectFrame( *p );
      wxRect r = window.GetRect();
      window.SetSize(wxSize(1,1));
      window.SetSize(r.GetSize());
#endif
   }
}
}; // struct Handler

} // namespace

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static EditActions::Handler instance;
   return instance;
};

// Menu definitions

#define FN(X) (& EditActions::Handler :: X)

namespace {
using namespace MenuTable;
BaseItemSharedPtr EditMenu()
{
   using Options = CommandManager::Options;

   // The default shortcut key for Redo is different on different platforms.
   static constexpr auto redoKey =
#ifdef __WXMSW__
      wxT("Ctrl+Y")
#else
      wxT("Ctrl+Shift+Z")
#endif
   ;

      // The default shortcut key for Preferences is different on different
      // platforms.
   static constexpr auto prefKey =
#ifdef __WXMAC__
      wxT("Ctrl+,")
#else
      wxT("Ctrl+P")
#endif
   ;

   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( wxT("Edit"), XXO("&Edit"),
      Section( "UndoRedo",
         Command( wxT("Undo"), XXO("&Undo"), FN(OnUndo),
            AudioIONotBusyFlag() | UndoAvailableFlag(), wxT("Ctrl+Z") ),

         Command( wxT("Redo"), XXO("&Redo"), FN(OnRedo),
            AudioIONotBusyFlag() | RedoAvailableFlag(), redoKey ),
            
         Special( wxT("UndoItemsUpdateStep"),
         [](AudacityProject &project, wxMenu&) {
            // Change names in the CommandManager as a side-effect
            MenuManager::ModifyUndoMenuItems(project);
         })
      ),

      // Note that on Mac, the Preferences menu item is specially handled in
      // CommandManager (assigned a special wxWidgets id) so that it does
      // not appear in the Edit menu but instead under Audacity, consistent with
      // MacOS conventions.
      Section( "Preferences",
         Command( wxT("Preferences"), XXO("Pre&ferences..."), FN(OnPreferences),
            AudioIONotBusyFlag(), prefKey )
      )

   ) ) };
   return menu;
}

AttachedItem sAttachment1{
   wxT(""),
   Shared( EditMenu() )
};

}
#undef FN
