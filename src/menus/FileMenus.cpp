#include "../Audacity.h" // for USE_* macros

#include "../CommonCommandFlags.h"
#include "../FileNames.h"
#include "../LabelTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ProjectFileIO.h"
#include "../ProjectFileManager.h"
#include "../ProjectHistory.h"
#include "../ProjectManager.h"
#include "../ProjectWindow.h"
#include "../SelectUtilities.h"
#include "../UndoManager.h"
#include "../ViewInfo.h"
#include "../WaveTrack.h"
#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../export/ExportMultiple.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/FileHistory.h"
#include "../widgets/wxPanelWrapper.h"

#include <wx/menu.h>

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

void OnExportMp3(const CommandContext &context)
{
   auto &project = context.project;
   Exporter::DoExport(project, "MP3");
}

void OnExportWav(const CommandContext &context)
{
   auto &project = context.project;
   Exporter::DoExport(project, "WAV");
}

void OnExportOgg(const CommandContext &context)
{
   auto &project = context.project;
   Exporter::DoExport(project, "OGG");
}

void OnExportAudio(const CommandContext &context)
{
   auto &project = context.project;
   Exporter::DoExport(project, "");
}

void OnExportSelection(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   Exporter e{ project };

   e.SetFileDialogTitle( XO("Export Selected Audio") );
   e.Process(true, selectedRegion.t0(),
      selectedRegion.t1());
}

void OnExportLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &window = GetProjectFrame( project );

   /* i18n-hint: filename containing exported text from label tracks */
   wxString fName = _("labels.txt");
   auto trackRange = tracks.Any<const LabelTrack>();
   auto numLabelTracks = trackRange.size();

   if (numLabelTracks == 0) {
      AudacityMessageBox( XO("There are no label tracks to export.") );
      return;
   }
   else
      fName = (*trackRange.rbegin())->GetName();

   fName = FileNames::SelectFile(FileNames::Operation::Export,
      XO("Export Labels As:"),
      wxEmptyString,
      fName,
      wxT("txt"),
      { FileNames::TextFiles },
      wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxRESIZE_BORDER,
      &window);

   if (fName.empty())
      return;

   // Move existing files out of the way.  Otherwise wxTextFile will
   // append to (rather than replace) the current file.

   if (wxFileExists(fName)) {
#ifdef __WXGTK__
      wxString safetyFileName = fName + wxT("~");
#else
      wxString safetyFileName = fName + wxT(".bak");
#endif

      if (wxFileExists(safetyFileName))
         wxRemoveFile(safetyFileName);

      wxRename(fName, safetyFileName);
   }

   wxTextFile f(fName);
   f.Create();
   f.Open();
   if (!f.IsOpened()) {
      AudacityMessageBox(
         XO( "Couldn't write to file: %s" ).Format( fName ) );
      return;
   }

   for (auto lt : trackRange)
      lt->Export(f);

   f.Write();
   f.Close();
}

void OnExportMultiple(const CommandContext &context)
{
   auto &project = context.project;
   ExportMultipleDialog em(&project);

   em.ShowModal();
}

void OnImportLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &trackFactory = WaveTrackFactory::Get( project );
   auto &tracks = TrackList::Get( project );
   auto &window = ProjectWindow::Get( project );

   wxString fileName =
       FileNames::SelectFile(FileNames::Operation::Open,
         XO("Select a text file containing labels"),
         wxEmptyString,     // Path
         wxT(""),       // Name
         wxT("txt"),   // Extension
         { FileNames::TextFiles, FileNames::AllFiles },
         wxRESIZE_BORDER,        // Flags
         &window);    // Parent

   if (!fileName.empty()) {
      wxTextFile f;

      f.Open(fileName);
      if (!f.IsOpened()) {
         AudacityMessageBox(
            XO("Could not open file: %s").Format( fileName ) );
         return;
      }

      auto newTrack = std::make_shared<LabelTrack>();
      wxString sTrackName;
      wxFileName::SplitPath(fileName, NULL, NULL, &sTrackName, NULL);
      newTrack->SetName(sTrackName);

      newTrack->Import(f);

      SelectUtilities::SelectNone( project );
      newTrack->SetSelected(true);
      tracks.Add( newTrack );

      ProjectHistory::Get( project ).PushState(
         XO("Imported labels from '%s'").Format( fileName ),
            XO("Import Labels"));

      window.ZoomAfterImport(nullptr);
   }
}

void OnExit(const CommandContext &WXUNUSED(context) )
{
   // Simulate the application Exit menu item
   wxCommandEvent evt{ wxEVT_MENU, wxID_EXIT };
   wxTheApp->AddPendingEvent( evt );
}

void OnExportFLAC(const CommandContext &context)
{
   Exporter::DoExport(context.project, "FLAC");
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
   #ifdef __WXMAC__
            /* i18n-hint: This is the name of the menu item on Mac OS X only */
            XXO("Open Recent")
   #else
            /* i18n-hint: This is the name of the menu item on Windows and Linux */
            XXO("Recent &Files")
   #endif
            ,
            Special( wxT("PopulateRecentFilesStep"),
            [](AudacityProject &, wxMenu &theMenu){
               // Recent Files and Recent Projects menus
               auto &history = FileHistory::Global();
               history.UseMenu( &theMenu );

               wxWeakRef<wxMenu> recentFilesMenu{ &theMenu };
               wxTheApp->CallAfter( [=] {
                  // Bug 143 workaround.
                  // The bug is in wxWidgets.  For a menu that has scrollers,
                  // the scrollers have an ID of 0 (not wxID_NONE which is -3).
                  // Therefore wxWidgets attempts to find a help string. See
                  // wxFrameBase::ShowMenuHelp(int menuId)
                  // It finds a bogus automatic help string of "Recent &Files"
                  // from that submenu.
                  // So we set the help string for command with Id 0 to empty.
                  if ( recentFilesMenu )
                     recentFilesMenu->GetParent()->SetHelpString( 0, "" );
               } );
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
         Menu( wxT("Export"), XXO("&Export"),
            // Enable Export audio commands only when there are audio tracks.
            Command( wxT("ExportMp3"), XXO("Export as MP&3"), FN(OnExportMp3),
               AudioIONotBusyFlag() | WaveTracksExistFlag() ),

            Command( wxT("ExportWav"), XXO("Export as &WAV"), FN(OnExportWav),
               AudioIONotBusyFlag() | WaveTracksExistFlag() ),

            Command( wxT("ExportOgg"), XXO("Export as &OGG"), FN(OnExportOgg),
               AudioIONotBusyFlag() | WaveTracksExistFlag() ),

            Command( wxT("Export"), XXO("&Export Audio..."), FN(OnExportAudio),
               AudioIONotBusyFlag() | WaveTracksExistFlag(), wxT("Ctrl+Shift+E") ),

            // Enable Export Selection commands only when there's a selection.
            Command( wxT("ExportSel"), XXO("Expo&rt Selected Audio..."),
               FN(OnExportSelection),
               AudioIONotBusyFlag() | TimeSelectedFlag() | WaveTracksSelectedFlag() ),

            Command( wxT("ExportLabels"), XXO("Export &Labels..."),
               FN(OnExportLabels),
               AudioIONotBusyFlag() | LabelTracksExistFlag() ),
            // Enable Export audio commands only when there are audio tracks.
            Command( wxT("ExportMultiple"), XXO("Export &Multiple..."),
               FN(OnExportMultiple),
               AudioIONotBusyFlag() | WaveTracksExistFlag(), wxT("Ctrl+Shift+L") )
         ),

         Menu( wxT("Import"), XXO("&Import"),
            Command( wxT("ImportLabels"), XXO("&Labels..."), FN(OnImportLabels),
               AudioIONotBusyFlag() )
         )
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

AttachedItem sAttachment3{
   wxT("HiddenFileItems/HiddenFileMenu"),
   ( FinderScope{ findCommandHandler },
   Command( wxT("ExportFLAC"), XXO("Export as FLAC"),
      FN(OnExportFLAC),
      AudioIONotBusyFlag() )
   )
};

}

#undef FN
