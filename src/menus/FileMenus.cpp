#include "../CommonCommandFlags.h"
#include "FileNames.h"
#include "Prefs.h"
#include "Project.h"
#include "../ProjectFileIO.h"
#include "../ProjectFileManager.h"
#include "../ProjectHistory.h"
#include "../ProjectManager.h"
#include "ProjectWindows.h"
#include "../ProjectWindow.h"
#include "SelectFile.h"
#include "../SelectUtilities.h"
#include "../UndoManager.h"
#include "ViewInfo.h"
#include "WaveTrack.h"
#include "CommandContext.h"
#include "CommandManager.h"
#include "AudacityMessageBox.h"
#include "../widgets/FileHistory.h"
#include "wxPanelWrapper.h"
#include "MenuHandle.h"
#include <wx/app.h>

// Menu handler functions

namespace FileActions {

struct Handler : CommandHandlerObject {

void OnNew(const CommandContext & )
{
   ( void ) ProjectManager::New();
}

void OnOpen(const CommandContext &context )
{
   auto &project = context.project;
   ProjectManager::OpenFiles(&project);
}

// JKC: This is like OnClose, except it empties the project in place,
// rather than creating a new empty project (with new toolbars etc).
// It does not test for unsaved changes.
// It is not in the menus by default.  Its main purpose is/was for 
// developers checking functionality of ResetProjectToEmpty().
void OnProjectReset(const CommandContext &context)
{
   auto &project = context.project;
   ProjectManager::Get( project ).ResetProjectToEmpty();
}

void OnClose(const CommandContext &context )
{
   auto &project = context.project;
   auto &window = ProjectWindow::Get( project );
   ProjectFileManager::Get( project ).SetMenuClose(true);
   window.Close();
}

void OnCompact(const CommandContext &context)
{
   ProjectFileManager::Get(context.project).Compact();
}

void OnSave(const CommandContext &context )
{
   auto &project = context.project;
   auto &projectFileManager = ProjectFileManager::Get( project );
   projectFileManager.Save();
}

void OnSaveAs(const CommandContext &context )
{
   auto &project = context.project;
   auto &projectFileManager = ProjectFileManager::Get( project );
   projectFileManager.SaveAs();
}

void OnSaveCopy(const CommandContext &context )
{
   auto &project = context.project;
   auto &projectFileManager = ProjectFileManager::Get( project );
   projectFileManager.SaveCopy();
}

void OnExit(const CommandContext &WXUNUSED(context) )
{
   // Simulate the application Exit menu item
   wxCommandEvent evt{ wxEVT_MENU, wxID_EXIT };
   wxTheApp->AddPendingEvent( evt );
}

}; // struct Handler

} // namespace

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static FileActions::Handler instance;
   return instance;
};

// Menu definitions

#define FN(X) (& FileActions::Handler :: X)

namespace {
using namespace MenuTable;

BaseItemSharedPtr FileMenu()
{
   using Options = CommandManager::Options;

   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( wxT("File"), XXO("&File"),
      Section( "Basic",
         /*i18n-hint: "New" is an action (verb) to create a NEW project*/
         Command( wxT("New"), XXO("&New"), FN(OnNew),
            AudioIONotBusyFlag(), wxT("Ctrl+N") ),

         /*i18n-hint: (verb)*/
         Command( wxT("Open"), XXO("&Open..."), FN(OnOpen),
            AudioIONotBusyFlag(), wxT("Ctrl+O") ),

   #ifdef EXPERIMENTAL_RESET
         // Empty the current project and forget its name and path.  DANGEROUS
         // It's just for developers.
         // Do not translate this menu item (no XXO).
         // It MUST not be shown to regular users.
         Command( wxT("Reset"), XXO("&Dangerous Reset..."), FN(OnProjectReset),
            AudioIONotBusyFlag() ),
   #endif

   /////////////////////////////////////////////////////////////////////////////

         Menu( wxT("Recent"),
            {
   #ifdef __WXMAC__
               /* i18n-hint: This is the name of the menu item on Mac OS X only */
               XXO("Open Recent")
   #else
               /* i18n-hint: This is the name of the menu item on Windows and Linux */
               XXO("Recent &Files")
   #endif
               // Bug 143 workaround.
               // For a menu that has scrollers,
               // the scrollers have an ID of 0 (not wxID_NONE which is -3).
               // Therefore wxWidgets attempts to find a help string. See
               // wxFrameBase::ShowMenuHelp(int menuId)
               // It finds a bogus automatic help string of "Recent &Files"
               // from that submenu.
               // So we set the help string for command with Id 0 to empty.
               , Verbatim("")
            }
            ,
            Special( L"PopulateRecentFilesStep",
            [](AudacityProject &, Widgets::MenuHandle theMenu){
               // Recent Files and Recent Projects menus
               auto &history = FileHistory::Global();
               history.UseMenu( theMenu );
            } )
         ),

   /////////////////////////////////////////////////////////////////////////////

         Command( wxT("Close"), XXO("&Close"), FN(OnClose),
            AudioIONotBusyFlag(), wxT("Ctrl+W") )
      ),

      Section( "Save",
         Menu( wxT("Save"), XXO("&Save Project"),
            Command( wxT("Save"), XXO("&Save Project"), FN(OnSave),
               AudioIONotBusyFlag(), wxT("Ctrl+S") ),
            Command( wxT("SaveAs"), XXO("Save Project &As..."), FN(OnSaveAs),
               AudioIONotBusyFlag() ),
            Command( wxT("SaveCopy"), XXO("&Backup Project..."), FN(OnSaveCopy),
               AudioIONotBusyFlag() )
         )//,

         // Bug 2600: Compact has interactions with undo/history that are bound
         // to confuse some users.  We don't see a way to recover useful amounts 
         // of space and not confuse users using undo.
         // As additional space used by aup3 is 50% or so, perfectly valid
         // approach to this P1 bug is to not provide the 'Compact' menu item.
         //Command( wxT("Compact"), XXO("Co&mpact Project"), FN(OnCompact),
         //   AudioIONotBusyFlag(), wxT("Shift+A") )
      ),

      Section( "Import-Export",
         Menu( wxT("Export"), XXO("&Export") ),
         Menu( wxT("Import"), XXO("&Import") )
      ),

      Section( "Print" ),

      Section( "Exit",
         // On the Mac, the Exit item doesn't actually go here...wxMac will
         // pull it out
         // and put it in the Audacity menu for us based on its ID.
         /* i18n-hint: (verb) It's item on a menu. */
         Command( wxT("Exit"), XXO("E&xit"), FN(OnExit),
            AlwaysEnabledFlag, wxT("Ctrl+Q") )
      )
   ) ) };
   return menu;
}

AttachedItem sAttachment1{
   wxT(""),
   Shared( FileMenu() )
};

BaseItemSharedPtr HiddenFileMenu()
{
   static BaseItemSharedPtr menu
   {
      ConditionalItems( wxT("HiddenFileItems"),
         []()
         {
            // Ensures that these items never appear in a menu, but
            // are still available to scripting
            return false;
         },
         Menu( wxT("HiddenFileMenu"), XXO("Hidden File Menu") )
      )
   };
   return menu;
}

AttachedItem sAttachment2{
   wxT(""),
   Shared( HiddenFileMenu() )
};

}

#undef FN
