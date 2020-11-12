/**********************************************************************

  Audacity: A Digital Audio Editor

  Import.cpp

  Dominic Mazzoni

*******************************************************************//**

\file Import.cpp

  This file contains a general function which will import almost
  any type of sampled audio file (including MIDI)
  and return the wave tracks that were imported.  This function just
  figures out which one to call; among actual importers are
  ImportPCM, ImportMP3, ImportOGG, ImportLOF,
  ImportQT, ImportFLAC and ImportAUP.

*//***************************************************************//**

\class Format
\brief Abstract base class used in importing a file.

It's defined in Import.h

*//***************************************************************//**

\class Importer
\brief Class which actually imports the audio, using functions defined
in ImportPCM.cpp, ImportMP3.cpp, ImportOGG.cpp,
ImportLOF.cpp, and ImportAUP.cpp.

*//******************************************************************/




#include "Import.h"

#include "ImportPlugin.h"

#include <algorithm>

#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/log.h>
#include <wx/sizer.h>         //for wxBoxSizer
#include "BasicUI.h"
#include "FileNames.h"
#include "ShuttleGui.h"
#include "Project.h"
#include "ProjectFileIO.h"
#include "ProjectHistory.h"
#include "ProjectSettings.h"
#include "SelectUtilities.h"
#include "Tags.h"
#include "WaveTrack.h"
#include "toolbars/SelectionBar.h"
#include "widgets/FileHistory.h"

#include "Prefs.h"

#include "widgets/ProgressDialog.h"

// ============================================================================
//
// Return reference to singleton
//
// (Thread-safe...no active threading during construction or after destruction)
// ============================================================================
Importer Importer::mInstance;
Importer & Importer::Get()
{
   return mInstance;
}

Importer::Importer()
{
}

Importer::~Importer()
{
}

static ImportPluginList &sImportPluginList()
{
   static ImportPluginList theList;
   return theList;
}

namespace {
static const auto PathStart = wxT("Importers");


}

Registry::GroupItem &Importer::ImporterItem::Registry()
{
   static Registry::TransparentGroupItem<> registry{ PathStart };
   return registry;
}

Importer::ImporterItem::ImporterItem( const Identifier &id, std::unique_ptr<ImportPlugin> pPlugin )
   : SingleItem{ id }
   , mpPlugin{ std::move( pPlugin ) }
{}

Importer::ImporterItem::~ImporterItem() = default;

Importer::RegisteredImportPlugin::RegisteredImportPlugin(
   const Identifier &id,
   std::unique_ptr<ImportPlugin> pPlugin,
   const Registry::Placement &placement )
   : RegisteredItem{
      pPlugin
         ? std::make_unique< ImporterItem >( id, std::move( pPlugin ) )
         : nullptr,
      placement
   }
{
}

UnusableImportPluginList &Importer::sUnusableImportPluginList()
{
   static UnusableImportPluginList theList;
   return theList;
}

Importer::RegisteredUnusableImportPlugin::RegisteredUnusableImportPlugin(
   std::unique_ptr<UnusableImportPlugin> pPlugin )
   : mpPlugin( pPlugin.get() )
{
   if ( mpPlugin ) {
      sUnusableImportPluginList().emplace_back( std::move( pPlugin ) );
   }
}

Importer::RegisteredUnusableImportPlugin::~RegisteredUnusableImportPlugin()
{
   if ( mpPlugin ) {
      auto &list = sUnusableImportPluginList();
      auto end = list.end();
      auto iter = std::find_if( list.begin(), end,
         [&](auto &entry){ return entry.get() == mpPlugin; } );
      if ( iter != end )
         list.erase( iter );
      else
         wxASSERT( false );
   }
}

bool Importer::Initialize()
{
   // build the list of import plugin and/or unusableImporters.
   // order is significant.  If none match, they will all be tried
   // in the order defined here.

   using namespace Registry;
   static OrderingPreferenceInitializer init{
      PathStart,
      { {wxT(""), wxT("AUP,PCM,OGG,FLAC,MP3,LOF,portsmf,FFmpeg") } }
      // QT and GStreamer are only conditionally compiled and would get
      // placed at the end if present
   };
   
   static struct MyVisitor final : Visitor {
      MyVisitor()
      {
         // Once only, visit the registry to collect the plug-ins properly
         // sorted
         TransparentGroupItem<> top{ PathStart };
         Registry::Visit( *this, &top, &ImporterItem::Registry() );
      }

      void Visit( SingleItem &item, const Path &path ) override
      {
         sImportPluginList().push_back(
            static_cast<ImporterItem&>( item ).mpPlugin.get() );
      }
   } visitor;

   // Ordering of the unusable plugin list is not important.

   ExtImportItems{}.swap(mExtImportItems);

   ReadImportItems();

   return true;
}

bool Importer::Terminate()
{
   WriteImportItems();

   return true;
}

static FileNames::RegisteredFileTypeLister sLister {
   []{
      FileNames::FileTypes list;
      for(const auto &importPlugin : sImportPluginList())
      {
         list.emplace_back(importPlugin->GetPluginFormatDescription(),
                                  importPlugin->GetSupportedExtensions());
      }
      return list;
   }
};

void Importer::StringToList(wxString &str, wxString &delims, wxArrayString &list, wxStringTokenizerMode mod)
{
   wxStringTokenizer toker;

   for (toker.SetString(str, delims, mod);
      toker.HasMoreTokens(); list.push_back(toker.GetNextToken()));
}

void Importer::ReadImportItems()
{
   int item_counter = 0;
   wxStringTokenizer toker;
   wxString item_name;
   wxString item_value;

   ExtImportItems{}.swap(mExtImportItems);
   /* Rule string format is:
    * extension1:extension2:extension3\mime_type1:mime_type2:mime_type3|filter1:filter2:filter3\unusedfilter1:unusedfilter2
    * backslashes are escaped and unescaped internally
    */
   for (item_counter = 0; true; item_counter++)
   {
      wxString condition, filters, used_filters, unused_filters, extensions, mime_types;
      item_name.Printf (wxT("/ExtImportItems/Item%d"), item_counter);
      /* Break at first non-existent item */
      if (!gPrefs->Read(item_name, &item_value))
        break;

      toker.SetString(item_value, wxT("|"), wxTOKEN_RET_EMPTY_ALL);
      /* Break at first broken item */
      if (toker.CountTokens() != 2)
        break;

      auto new_item = std::make_unique<ExtImportItem>();

      /* First token is the filtering condition, second - the filter list */
      condition = toker.GetNextToken();
      filters = toker.GetNextToken();

      /* Condition token consists of extension list and mime type list
       * mime type list can be omitted entirely (complete with '\' separator)*/
      toker.SetString(condition, wxT("\\"), wxTOKEN_RET_EMPTY_ALL);
      extensions = toker.GetNextToken();
      if (toker.HasMoreTokens())
        mime_types = toker.GetNextToken();

      wxString delims(wxT(":"));
      StringToList (extensions, delims, new_item->extensions);

      if (!mime_types.empty())
         StringToList (mime_types, delims, new_item->mime_types);

      /* Filter token consists of used and unused filter lists */
      toker.SetString(filters, wxT("\\"), wxTOKEN_RET_EMPTY_ALL);
      used_filters = toker.GetNextToken();
      if (toker.HasMoreTokens())
        unused_filters = toker.GetNextToken();

      StringToList (used_filters, delims, new_item->filters);

      if (!unused_filters.empty())
      {
         /* Filters are stored in one list, but the position at which
          * unused filters start is remembered
          */
         new_item->divider = new_item->filters.size();
         StringToList (unused_filters, delims, new_item->filters);
      }
      else
         new_item->divider = -1;

      /* Find corresponding filter object for each filter ID */
      for (size_t i = 0; i < new_item->filters.size(); i++)
      {
         bool found = false;
         for (const auto &importPlugin : sImportPluginList())
         {
            if (importPlugin->GetPluginStringID() == new_item->filters[i])
            {
               new_item->filter_objects.push_back(importPlugin);
               found = true;
               break;
            }
         }
         /* IDs that do not have corresponding filters, will be shown as-is */
         if (!found)
           new_item->filter_objects.push_back(nullptr);
      }
      /* Find all filter objects that are not present in the filter list */
      for (const auto &importPlugin : sImportPluginList())
      {
         bool found = false;
         for (size_t i = 0; i < new_item->filter_objects.size(); i++)
         {
            if (importPlugin == new_item->filter_objects[i])
            {
               found = true;
               break;
            }
         }
         /* Add these filters at the bottom of used filter list */
         if (!found)
         {
            int index = new_item->divider;
            if (new_item->divider < 0)
               index = new_item->filters.size();
            new_item->filters.insert(
               new_item->filters.begin() + index,
               importPlugin->GetPluginStringID());
            new_item->filter_objects.insert(
               new_item->filter_objects.begin() + index, importPlugin);
            if (new_item->divider >= 0)
               new_item->divider++;
         }
      }
      this->mExtImportItems.push_back( std::move(new_item) );
   }
}

void Importer::WriteImportItems()
{
   size_t i;
   wxString val, name;
   for (i = 0; i < this->mExtImportItems.size(); i++)
   {
      ExtImportItem *item = mExtImportItems[i].get();
      val.clear();

      for (size_t j = 0; j < item->extensions.size(); j++)
      {
         val.Append (item->extensions[j]);
         if (j < item->extensions.size() - 1)
            val.Append (wxT(":"));
      }
      val.Append (wxT("\\"));
      for (size_t j = 0; j < item->mime_types.size(); j++)
      {
         val.Append (item->mime_types[j]);
         if (j < item->mime_types.size() - 1)
            val.Append (wxT(":"));
      }
      val.Append (wxT("|"));
      for (size_t j = 0; j < item->filters.size() && ((int) j < item->divider || item->divider < 0); j++)
      {
         val.Append (item->filters[j]);
         if (j < item->filters.size() - 1 && ((int) j < item->divider - 1 || item->divider < 0))
            val.Append (wxT(":"));
      }
      if (item->divider >= 0)
      {
         val.Append (wxT("\\"));
         for (size_t j = item->divider; j < item->filters.size(); j++)
         {
            val.Append (item->filters[j]);
            if (j < item->filters.size() - 1)
               val.Append (wxT(":"));
         }
      }
      name.Printf (wxT("/ExtImportItems/Item%d"), (int)i);
      gPrefs->Write (name, val);
      gPrefs->Flush();
   }
   /* If we used to have more items than we have now, DELETE the excess items.
   We just keep deleting items and incrementing until we find there aren't any
   more to DELETE.*/
   i = this->mExtImportItems.size();
   do {
     name.Printf (wxT("/ExtImportItems/Item%d"), (int)i);
     // No item to DELETE?  Then it's time to finish.
     if (!gPrefs->Read(name, &val))
        break;
     // Failure to DELETE probably means a read-only config file.
     // no point continuing.
     // TODO: Possibly report (once).
     if( !gPrefs->DeleteEntry (name, false))
        break;
     i++;
   } while( true );
}

std::unique_ptr<ExtImportItem> Importer::CreateDefaultImportItem()
{
   auto new_item = std::make_unique<ExtImportItem>();
   new_item->extensions.push_back(wxT("*"));
   new_item->mime_types.push_back(wxT("*"));

   for (const auto &importPlugin : sImportPluginList())
   {
      new_item->filters.push_back(importPlugin->GetPluginStringID());
      new_item->filter_objects.push_back(importPlugin);
   }
   new_item->divider = -1;
   return new_item;
}

void
Importer::AddImportedTracks( AudacityProject &project,
   const FilePath &fileName, TrackHolders &&newTracks)
{
   auto &history = ProjectHistory::Get( project );
   auto &projectFileIO = ProjectFileIO::Get( project );
   auto &tracks = TrackList::Get( project );

   std::vector< std::shared_ptr< Track > > results;

   SelectUtilities::SelectNone( project );

   wxFileName fn(fileName);

   bool initiallyEmpty = tracks.empty();
   double newRate = 0;
   wxString trackNameBase = fn.GetName();
   int i = -1;
   
   // Fix the bug 2109.
   // In case the project had soloed tracks before importing,
   // all newly imported tracks are muted.
   const bool projectHasSolo =
      !(tracks.Any<PlayableTrack>() + &PlayableTrack::GetSolo).empty();
   if (projectHasSolo)
   {
      for (auto& track : newTracks)
         for (auto& channel : track)
            if (auto pChannel = track_cast<PlayableTrack*>(channel.get()))
               pChannel->SetMute(true);
   }

   // Must add all tracks first (before using Track::IsLeader)
   for (auto &group : newTracks) {
      if (group.empty()) {
         wxASSERT(false);
         continue;
      }
      auto first = group.begin()->get();
      auto nChannels = group.size();
      for (auto &uNewTrack : group) {
         auto newTrack = tracks.Add( uNewTrack );
         results.push_back(newTrack->SharedPointer());
      }
      tracks.GroupChannels(*first, nChannels);
   }
   newTracks.clear();
      
   // Now name them

   // Add numbers to track names only if there is more than one (mono or stereo)
   // track (not necessarily, more than one channel)
   const bool useSuffix =
      make_iterator_range( results.begin() + 1, results.end() )
         .any_of( []( decltype(*results.begin()) &pTrack )
            { return pTrack->IsLeader(); } );

   for (const auto &newTrack : results) {
      if ( newTrack->IsLeader() )
         // Count groups only
         ++i;

      newTrack->SetSelected(true);

      if ( useSuffix )
         newTrack->SetName(trackNameBase + wxString::Format(wxT(" %d" ), i + 1));
      else
         newTrack->SetName(trackNameBase);

      newTrack->TypeSwitch( [&](WaveTrack *wt) {
         if (newRate == 0)
            newRate = wt->GetRate();
      });
   }

   // Automatically assign rate of imported file to whole project,
   // if this is the first file that is imported
   if (initiallyEmpty && newRate > 0) {
      auto &settings = ProjectSettings::Get( project );
      settings.SetRate( newRate );
      SelectionBar::Get( project ).SetRate( newRate );
   }

   history.PushState(XO("Imported '%s'").Format( fileName ),
       XO("Import"));

#if defined(__WXGTK__)
   // See bug #1224
   // The track panel hasn't we been fully created, so the DoZoomFit() will not give
   // expected results due to a window width of zero.  Should be safe to yield here to
   // allow the creation to complete.  If this becomes a problem, it "might" be possible
   // to queue a dummy event to trigger the DoZoomFit().
   wxEventLoopBase::GetActive()->YieldFor(wxEVT_CATEGORY_UI | wxEVT_CATEGORY_USER_INPUT);
#endif

   // If the project was clean and temporary (not permanently saved), then set
   // the filename to the just imported path.
   if (initiallyEmpty && projectFileIO.IsTemporary()) {
      project.SetProjectName(fn.GetName());
      project.SetInitialImportPath(fn.GetPath());
      projectFileIO.SetProjectTitle();
   }

   // Moved this call to higher levels to prevent flicker redrawing everything on each file.
   //   HandleResize();
}

namespace {
bool ImportProject(AudacityProject &dest, const FilePath &fileName)
{
   InvisibleTemporaryProject temp;
   auto &project = temp.Project();

   auto &projectFileIO = ProjectFileIO::Get(project);
   if (!projectFileIO.LoadProject(fileName, false))
      return false;
   auto &srcTracks = TrackList::Get(project);
   auto &destTracks = TrackList::Get(dest);
   for (const Track *pTrack : srcTracks.Any()) {
      auto destTrack = pTrack->PasteInto(dest);
      Track::FinishCopy(pTrack, destTrack.get());
      if (destTrack.use_count() == 1)
         destTracks.Add(destTrack);
   }
   Tags::Get(dest).Merge(Tags::Get(project));

   return true;
}
}

// If pNewTrackList is passed in non-NULL, it gets filled with the pointers to NEW tracks.
bool Importer::Import(AudacityProject &project,
   const FilePath &fileName,
   bool addToHistory /* = true */)
{
   auto &projectFileIO = ProjectFileIO::Get(project);
   auto oldTags = Tags::Get( project ).shared_from_this();
   bool initiallyEmpty = TrackList::Get(project).empty();
   TrackHolders newTracks;
   TranslatableString errorMessage;

#ifdef EXPERIMENTAL_IMPORT_AUP3
   // Handle AUP3 ("project") files directly
   if (fileName.AfterLast('.').IsSameAs(wxT("aup3"), false)) {
      if (ImportProject(project, fileName)) {
         auto &history = ProjectHistory::Get(project);

         // If the project was clean and temporary (not permanently saved), then set
         // the filename to the just imported path.
         if (initiallyEmpty && projectFileIO.IsTemporary()) {
            wxFileName fn(fileName);
            project.SetProjectName(fn.GetName());
            project.SetInitialImportPath(fn.GetPath());
            projectFileIO.SetProjectTitle();
         }

         history.PushState(XO("Imported '%s'").Format(fileName), XO("Import"));

         if (addToHistory) {
            FileHistory::Global().Append(fileName);
         }
      }
      else {
         errorMessage = projectFileIO.GetLastError();
         if (errorMessage.empty()) {
            errorMessage = XO("Failed to import project");
         }

         // Additional help via a Help button links to the manual.
         BasicUI::ShowErrorDialog( *ProjectFramePlacement(&project),
            XO("Error Importing"), errorMessage, wxT("Importing_Audio"));
      }

      return false;
   }
#endif

   {
      // Backup Tags, before the import.  Be prepared to roll back changes.
      bool committed = false;
      auto cleanup = finally([&]{
         if ( !committed )
            Tags::Set( project, oldTags );
      });
      auto newTags = oldTags->Duplicate();
      Tags::Set( project, newTags );

#ifndef EXPERIMENTAL_IMPORT_AUP3
      // Handle AUP3 ("project") files specially
      if (fileName.AfterLast('.').IsSameAs(wxT("aup3"), false)) {
         BasicUI::ShowErrorDialog( *ProjectFramePlacement(&project),
            XO("Error Importing"),
            XO( "Cannot import AUP3 format.  Use File > Open instead"),
            wxT("File_Menu"));
         return false;
      }
#endif
      bool success = Importer::Get().Import(project, fileName,
                                            &WaveTrackFactory::Get( project ),
                                            newTracks,
                                            newTags.get(),
                                            errorMessage);

      if (!errorMessage.empty()) {
         // Error message derived from Importer::Import
         // Additional help via a Help button links to the manual.
         BasicUI::ShowErrorDialog( *ProjectFramePlacement(&project),
            XO("Error Importing"), errorMessage, wxT("Importing_Audio"));
      }
      if (!success)
         return false;

      if (addToHistory) {
         FileHistory::Global().Append(fileName);
      }

      // no more errors, commit
      committed = true;
   }

   // for LOF ("list of files") files, do not import the file as if it
   // were an audio file itself
   if (fileName.AfterLast('.').IsSameAs(wxT("lof"), false)) {
      // PRL: don't redundantly do the steps below, because we already
      // did it in case of LOF, because of some weird recursion back to this
      // same function.  I think this should be untangled.

      // So Undo history push is not bypassed, despite appearances.
      return false;
   }

   // Handle AUP ("legacy project") files directly
   if (fileName.AfterLast('.').IsSameAs(wxT("aup"), false)) {
      // If the project was clean and temporary (not permanently saved), then set
      // the filename to the just imported path.
      if (initiallyEmpty && projectFileIO.IsTemporary()) {
         wxFileName fn(fileName);
         project.SetProjectName(fn.GetName());
         project.SetInitialImportPath(fn.GetPath());
         projectFileIO.SetProjectTitle();
      }

      auto &history = ProjectHistory::Get( project );

      history.PushState(XO("Imported '%s'").Format( fileName ), XO("Import"));

      return false;
   }

   // PRL: Undo history is incremented inside this:
   AddImportedTracks(project, fileName, std::move(newTracks));

   return true;
}

// returns number of tracks imported
bool Importer::Import( AudacityProject &project,
                     const FilePath &fName,
                     WaveTrackFactory *trackFactory,
                     TrackHolders &tracks,
                     Tags *tags,
                     TranslatableString &errorMessage)
{
   AudacityProject *pProj = &project;
   auto cleanup = valueRestorer( pProj->mbBusyImporting, true );

   const FileExtension extension{ fName.AfterLast(wxT('.')) };

   // Bug #2647: Peter has a Word 2000 .doc file that is recognized and imported by FFmpeg.
   if (wxFileName(fName).GetExt() == wxT("doc")) {
      errorMessage =
         XO("\"%s\" \nis a not an audio file. \nAudacity cannot open this type of file.")
         .Format( fName );
      return false;
   }

   using ImportPluginPtrs = std::vector< ImportPlugin* >;

   // This list is used to call plugins in correct order
   ImportPluginPtrs importPlugins;

   // This list is used to remember plugins that should have been compatible with the file.
   ImportPluginPtrs compatiblePlugins;

   // Not implemented (yet?)
   wxString mime_type = wxT("*");

   // First, add user-selected filter
   bool usersSelectionOverrides;
   gPrefs->Read(wxT("/ExtendedImport/OverrideExtendedImportByOpenFileDialogChoice"), &usersSelectionOverrides, false);

   if (usersSelectionOverrides)
   {
      // If user explicitly selected a filter,
      // then we should try importing via corresponding plugin first
      wxString type = gPrefs->Read(wxT("/LastOpenType"),wxT(""));

      wxLogDebug(wxT("LastOpenType is %s"),type);
      wxLogDebug(wxT("OverrideExtendedImportByOpenFileDialogChoice is %i"),usersSelectionOverrides);

      for (const auto &plugin : sImportPluginList())
      {
         if (plugin->GetPluginFormatDescription().Translation() == type )
         {
            // This plugin corresponds to user-selected filter, try it first.
            wxLogDebug(wxT("Inserting %s"),plugin->GetPluginStringID());
            importPlugins.insert(importPlugins.begin(), plugin);
         }
      }
   }

   wxLogMessage(wxT("File name is %s"), fName);
   wxLogMessage(wxT("Mime type is %s"), mime_type.Lower());

   for (const auto &uItem : mExtImportItems)
   {
      ExtImportItem *item = uItem.get();
      bool matches_ext = false, matches_mime = false;
      wxLogDebug(wxT("Testing extensions"));
      for (size_t j = 0; j < item->extensions.size(); j++)
      {
         wxLogDebug(wxT("%s"), item->extensions[j].Lower());
         if (wxMatchWild (item->extensions[j].Lower(),fName.Lower(), false))
         {
            wxLogDebug(wxT("Match!"));
            matches_ext = true;
            break;
         }
      }
      if (item->extensions.size() == 0)
      {
         wxLogDebug(wxT("Match! (empty list)"));
         matches_ext = true;
      }
      if (matches_ext)
         wxLogDebug(wxT("Testing mime types"));
      else
         wxLogDebug(wxT("Not testing mime types"));
      for (size_t j = 0; matches_ext && j < item->mime_types.size(); j++)
      {
         if (wxMatchWild (item->mime_types[j].Lower(),mime_type.Lower(), false))
         {
            wxLogDebug(wxT("Match!"));
            matches_mime = true;
            break;
         }
      }
      if (item->mime_types.size() == 0)
      {
         wxLogDebug(wxT("Match! (empty list)"));
         matches_mime = true;
      }
      if (matches_ext && matches_mime)
      {
         wxLogDebug(wxT("Complete match!"));
         for (size_t j = 0; j < item->filter_objects.size() && (item->divider < 0 || (int) j < item->divider); j++)
         {
            // the filter_object can be NULL if a suitable importer was not found
            // this happens when we recompile with --without-ffmpeg and there
            // is still ffmpeg in prefs from previous --with-ffmpeg builds
            if (!(item->filter_objects[j]))
               continue;
            wxLogDebug(wxT("Inserting %s"),item->filter_objects[j]->GetPluginStringID());
            importPlugins.push_back(item->filter_objects[j]);
         }
      }
   }

   // Add all plugins that support the extension
   for (const auto &plugin : sImportPluginList())
   {
      // Make sure its not already in the list
      if (importPlugins.end() ==
          std::find(importPlugins.begin(), importPlugins.end(), plugin))
      {
         if (plugin->SupportsExtension(extension))
         {
            wxLogDebug(wxT("Appending %s"),plugin->GetPluginStringID());
            importPlugins.push_back(plugin);
         }
      }
   }

   // Add remaining plugins
   for (const auto &plugin : sImportPluginList())
   {
      // Make sure its not already in the list
      if (importPlugins.end() ==
            std::find(importPlugins.begin(), importPlugins.end(), plugin))
      {
         wxLogDebug(wxT("Appending %s"),plugin->GetPluginStringID());
         importPlugins.push_back(plugin);
      }
   }

   // Try the import plugins, in the permuted sequences just determined
   for (const auto plugin : importPlugins)
   {
      // Try to open the file with this plugin (probe it)
      wxLogMessage(wxT("Opening with %s"),plugin->GetPluginStringID());
      auto inFile = plugin->Open(fName, pProj);
      if ( (inFile != NULL) && (inFile->GetStreamCount() > 0) )
      {
         wxLogMessage(wxT("Open(%s) succeeded"), fName);
         // File has more than one stream - display stream selector
         if (inFile->GetStreamCount() > 1)
         {
            ImportStreamDialog ImportDlg(inFile.get(), NULL, -1, XO("Select stream(s) to import"));

            if (ImportDlg.ShowModal() == wxID_CANCEL)
            {
               return false;
            }
         }
         // One stream - import it by default
         else
            inFile->SetStreamUsage(0,TRUE);

         auto res = inFile->Import(trackFactory, tracks, tags);
         switch (res) {
         case ImportFileHandle::ImportResult::Success:
         {
            auto end = tracks.end();
            auto iter = std::remove_if( tracks.begin(), end,
               std::mem_fn( &std::vector< std::shared_ptr<Track> >::empty ) );
            if ( iter != end ) {
               // importer shouldn't give us empty groups of channels!
               wxASSERT(false);
               // But correct that and proceed anyway
               tracks.erase( iter, end );
            }
            // success!
            return true;
         }
         case ImportFileHandle::ImportResult::Failed:
            return false;
         case ImportFileHandle::ImportResult::Retry:
         default:
            break;
         }

         // We could exit here since we had a match on the file extension,
         // but there may be another plug-in that can import the file and
         // that may recognize the extension, so we allow the loop to
         // continue.
      }
   }
   wxLogError(wxT("Importer::Import: Opening failed."));

   // None of our plugins can handle this file.  It might be that
   // Audacity supports this format, but support was not compiled in.
   // If so, notify the user of this fact
   for (const auto &unusableImportPlugin : sUnusableImportPluginList())
   {
      if( unusableImportPlugin->SupportsExtension(extension) )
      {
         errorMessage = XO("This version of Audacity was not compiled with %s support.")
            .Format( unusableImportPlugin->GetPluginFormatDescription() );
         return false;
      }
   }

   /* warnings for unsupported data types */

   if (compatiblePlugins.empty())
   {
      // if someone has sent us a .cda file, send them away
      if (extension.IsSameAs(wxT("cda"), false)) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is an audio CD track. \nAudacity cannot open audio CDs directly. \nExtract (rip) the CD tracks to an audio format that \nAudacity can import, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      // playlist type files
      if ((extension.IsSameAs(wxT("m3u"), false))||(extension.IsSameAs(wxT("ram"), false))||(extension.IsSameAs(wxT("pls"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a playlist file. \nAudacity cannot open this file because it only contains links to other files. \nYou may be able to open it in a text editor and download the actual audio files.")
            .Format( fName );
         return false;
      }
      //WMA files of various forms
      if ((extension.IsSameAs(wxT("wma"), false))||(extension.IsSameAs(wxT("asf"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a Windows Media Audio file. \nAudacity cannot open this type of file due to patent restrictions. \nYou need to convert it to a supported audio format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }
      //AAC files of various forms (probably not encrypted)
      if ((extension.IsSameAs(wxT("aac"), false))||(extension.IsSameAs(wxT("m4a"), false))||(extension.IsSameAs(wxT("m4r"), false))||(extension.IsSameAs(wxT("mp4"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is an Advanced Audio Coding file.\nWithout the optional FFmpeg library, Audacity cannot open this type of file.\nOtherwise, you need to convert it to a supported audio format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }
      // encrypted itunes files
      if ((extension.IsSameAs(wxT("m4p"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is an encrypted audio file. \nThese typically are from an online music store. \nAudacity cannot open this type of file due to the encryption. \nTry recording the file into Audacity, or burn it to audio CD then \nextract the CD track to a supported audio format such as WAV or AIFF.")
            .Format( fName );
         return false;
      }
      // Real Inc. files of various sorts
      if ((extension.IsSameAs(wxT("ra"), false))||(extension.IsSameAs(wxT("rm"), false))||(extension.IsSameAs(wxT("rpm"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a RealPlayer media file. \nAudacity cannot open this proprietary format. \nYou need to convert it to a supported audio format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      // Other notes-based formats
      if ((extension.IsSameAs(wxT("kar"), false))||(extension.IsSameAs(wxT("mod"), false))||(extension.IsSameAs(wxT("rmi"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a notes-based file, not an audio file. \nAudacity cannot open this type of file. \nTry converting it to an audio file such as WAV or AIFF and \nthen import it, or record it into Audacity.")
            .Format( fName );
         return false;
      }

      // MusePack files
      if ((extension.IsSameAs(wxT("mp+"), false))||(extension.IsSameAs(wxT("mpc"), false))||(extension.IsSameAs(wxT("mpp"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a Musepack audio file. \nAudacity cannot open this type of file. \nIf you think it might be an mp3 file, rename it to end with \".mp3\" \nand try importing it again. Otherwise you need to convert it to a supported audio \nformat, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      // WavPack files
      if ((extension.IsSameAs(wxT("wv"), false))||(extension.IsSameAs(wxT("wvc"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a Wavpack audio file. \nAudacity cannot open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      // AC3 files
      if ((extension.IsSameAs(wxT("ac3"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a Dolby Digital audio file. \nAudacity cannot currently open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      // Speex files
      if ((extension.IsSameAs(wxT("spx"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is an Ogg Speex audio file. \nAudacity cannot currently open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      // Video files of various forms
      if ((extension.IsSameAs(wxT("mpg"), false))||(extension.IsSameAs(wxT("mpeg"), false))||(extension.IsSameAs(wxT("avi"), false))||(extension.IsSameAs(wxT("wmv"), false))||(extension.IsSameAs(wxT("rv"), false))) {
         errorMessage = XO(
/* i18n-hint: %s will be the filename */
"\"%s\" is a video file. \nAudacity cannot currently open this type of file. \nYou need to extract the audio to a supported format, such as WAV or AIFF.")
            .Format( fName );
         return false;
      }

      if( !wxFileExists(fName)){
         errorMessage = XO( "File \"%s\" not found.").Format( fName );
         return false;
      }

      // we were not able to recognize the file type
      TranslatableString extraMessages;
      for(const auto &importPlugin : sImportPluginList()) {
         auto message = importPlugin->FailureHint();
         if (!message.empty()) {
            extraMessages += message;
            extraMessages += Verbatim("\n");
         }
      }

      errorMessage = XO(
/* i18n-hint: %s will be the filename */
"Audacity did not recognize the type of the file '%s'.\n\n%sFor uncompressed files, also try File > Import > Raw Data.")
         .Format( fName, extraMessages );
   }
   else
   {
      // We DO have a plugin for this file, but import failed.
      TranslatableString pluglist;

      for (const auto &plugin : compatiblePlugins)
      {
         if (pluglist.empty())
           pluglist = plugin->GetPluginFormatDescription();
         else
           pluglist = XO("%s, %s")
               .Format( pluglist, plugin->GetPluginFormatDescription() );
      }

      errorMessage = XO(
/* i18n-hint: %s will be the filename */
"Audacity recognized the type of the file '%s'.\nImporters supposedly supporting such files are:\n%s,\nbut none of them understood this file format.")
         .Format( fName, pluglist );
   }

   return false;
}

//-------------------------------------------------------------------------
// ImportStreamDialog
//-------------------------------------------------------------------------

BEGIN_EVENT_TABLE( ImportStreamDialog, wxDialogWrapper )
   EVT_BUTTON( wxID_OK, ImportStreamDialog::OnOk )
   EVT_BUTTON( wxID_CANCEL, ImportStreamDialog::OnCancel )
END_EVENT_TABLE()

ImportStreamDialog::ImportStreamDialog( ImportFileHandle *_mFile, wxWindow *parent, wxWindowID id, const TranslatableString &title,
                                       const wxPoint &position, const wxSize& size, long style ):
wxDialogWrapper( parent, id, title, position, size, style | wxRESIZE_BORDER )
{
   SetName();

   mFile = _mFile;
   scount = mFile->GetStreamCount();
   for (wxInt32 i = 0; i < scount; i++)
      mFile->SetStreamUsage(i, FALSE);

   ShuttleGui S{ this, eIsCreating };
   {
      S.SetBorder( 5 );

      StreamList =
      S
         .Prop(1)
         .Position(wxEXPAND | wxALIGN_LEFT | wxALL)
         .Style(wxLB_EXTENDED | wxLB_ALWAYS_SB)
         .AddListBox(
            transform_container<wxArrayStringEx>(
               mFile->GetStreamInfo(),
               std::mem_fn( &TranslatableString::Translation ) ) );

      S.AddStandardButtons();
   }

   SetAutoLayout(true);
   GetSizer()->Fit( this );

   SetSize( 400, 200 );
}

ImportStreamDialog::~ImportStreamDialog()
{

}

void ImportStreamDialog::OnOk(wxCommandEvent & WXUNUSED(event))
{
   wxArrayInt selitems;
   int sels = StreamList->GetSelections(selitems);
   for (wxInt32 i = 0; i < sels; i++)
      mFile->SetStreamUsage(selitems[i],TRUE);
   EndModal( wxID_OK );
}

void ImportStreamDialog::OnCancel(wxCommandEvent & WXUNUSED(event))
{
   EndModal( wxID_CANCEL );
}

Importer::RegisteredImportPlugin::Init::Init()
{
   (void) ImporterItem::Registry();
}

Importer::RegisteredUnusableImportPlugin::Init::Init()
{
   void sUnusableImportPluginList();
}
