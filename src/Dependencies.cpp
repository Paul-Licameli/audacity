/**********************************************************************

   Audacity: A Digital Audio Editor
   Audacity(R) is copyright (c) 1999-2008 Audacity Team.
   License: GPL v2.  See License.txt.

   Dependencies.cpp

   Dominic Mazzoni
   Leland Lucius
   Markus Meyer
   LRN
   Michael Chinen
   Vaughan Johnson

   The primary function provided in this source file is
   ShowDependencyDialogIfNeeded.  It checks a project to see if
   any of its WaveTracks contain AliasBlockFiles; if so it
   presents a dialog to the user and lets them copy those block
   files into the project, making it self-contained.

********************************************************************//**

\class AliasedFile
\brief An audio file that is referenced (pointed into) directly from
an Audacity .aup file rather than Audacity having its own copies of the
data.

*//*****************************************************************//**

\class DependencyDialog
\brief DependencyDialog shows dependencies of an AudacityProject on 
AliasedFile s.

*//********************************************************************/

#include "Audacity.h"
#include "Dependencies.h"

#include <wx/defs.h>
#include <wx/filename.h>
#include <wx/listctrl.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/frame.h>
#include <wx/stattext.h>

#include "blockfile/SimpleBlockFile.h"
#include "DirManager.h"
#include "FileFormats.h"
#include "Prefs.h"
#include "Project.h"
#include "Sequence.h"
#include "ShuttleGui.h"
#include "WaveTrack.h"
#include "WaveClip.h"
#include "prefs/QualityPrefs.h"
#include "widgets/AudacityMessageBox.h"
#include "widgets/ProgressDialog.h"
#include "widgets/MenuHandle.h"

#include <unordered_map>

using AliasedFileHash = std::unordered_map<wxString, AliasedFile*>;

// These two hash types are used only inside short scopes
// so it is safe to key them by plain pointers.
using ReplacedBlockFileHash = std::unordered_map<BlockFile *, BlockFilePtr>;
using BoolBlockFileHash = std::unordered_map<BlockFile *, bool>;

// Given a project, returns a single array of all SeqBlocks
// in the current set of tracks.  Enumerating that array allows
// you to process all block files in the current set.
static void GetAllSeqBlocks(AudacityProject *project,
                            BlockPtrArray *outBlocks)
{
   for (auto waveTrack : TrackList::Get( *project ).Any< WaveTrack >()) {
      for(const auto &clip : waveTrack->GetAllClips()) {
         Sequence *sequence = clip->GetSequence();
         BlockArray &blocks = sequence->GetBlockArray();
         for (size_t i = 0; i < blocks.size(); i++)
            outBlocks->push_back(&blocks[i]);
      }
   }
}

// Given an Audacity project and a hash mapping aliased block
// files to un-aliased block files, walk through all of the
// tracks and replace each aliased block file with its replacement.
// Note that this code respects reference-counting and thus the
// process of making a project self-contained is actually undoable.
static void ReplaceBlockFiles(BlockPtrArray &blocks,
                              ReplacedBlockFileHash &hash)
// NOFAIL-GUARANTEE
{
   for (const auto &pBlock : blocks) {
      auto &f = pBlock->f;
      const auto src = &*f;
      if (hash.count( src ) > 0) {
         const auto &dst = hash[src];
         f = dst;
      }
   }
}

void FindDependencies(AudacityProject *project,
                      AliasedFileArray &outAliasedFiles)
{
   sampleFormat format = QualityPrefs::SampleFormatChoice();

   BlockPtrArray blocks;
   GetAllSeqBlocks(project, &blocks);

   AliasedFileHash aliasedFileHash;
   BoolBlockFileHash blockFileHash;

   for (const auto &blockFile : blocks) {
      const auto &f = blockFile->f;
      if (f->IsAlias() && (blockFileHash.count( &*f ) == 0))
      {
         // f is an alias block we have not yet counted.
         blockFileHash[ &*f ] = true; // Don't count the same blockfile twice.
         auto aliasBlockFile = static_cast<AliasBlockFile*>( &*f );
         const wxFileName &fileName = aliasBlockFile->GetAliasedFileName();

         // In ProjectFSCK(), if the user has chosen to
         // "Replace missing audio with silence", the code there puts in an empty wxFileName.
         // Don't count those in dependencies.
         if (!fileName.IsOk())
            continue;

         const wxString &fileNameStr = fileName.GetFullPath();
         auto blockBytes = (SAMPLE_SIZE(format) *
                           aliasBlockFile->GetLength());
         if (aliasedFileHash.count(fileNameStr) > 0)
            // Already put this AliasBlockFile in aliasedFileHash.
            // Update block count.
            aliasedFileHash[fileNameStr]->mByteCount += blockBytes;
         else
         {
            // Haven't counted this AliasBlockFile yet.
            // Add to return array and internal hash.

            // PRL: do this in two steps so that we move instead of copying.
            // We don't have a moving push_back in all compilers.
            outAliasedFiles.push_back(AliasedFile{});
            outAliasedFiles.back() =
               AliasedFile {
                  wxFileNameWrapper { fileName },
                  wxLongLong(blockBytes), fileName.FileExists()
               };
            aliasedFileHash[fileNameStr] = &outAliasedFiles.back();
         }
      }
   }
}

// Given a project and a list of aliased files that should no
// longer be external dependencies (selected by the user), replace
// all of those alias block files with disk block files.
static void RemoveDependencies(AudacityProject *project,
                               AliasedFileArray &aliasedFiles)
// STRONG-GUARANTEE
{
   auto &dirManager = DirManager::Get( *project );

   ProgressDialog progress(
      XO("Removing Dependencies"),
      XO("Copying audio data into project..."));
   auto updateResult = ProgressResult::Success;

   // Hash aliasedFiles based on their full paths and
   // count total number of bytes to process.
   AliasedFileHash aliasedFileHash;
   wxLongLong totalBytesToProcess = 0;
   for (auto &aliasedFile : aliasedFiles) {
      totalBytesToProcess += aliasedFile.mByteCount;
      const wxString &fileNameStr = aliasedFile.mFileName.GetFullPath();
      aliasedFileHash[fileNameStr] = &aliasedFile;
   }

   BlockPtrArray blocks;
   GetAllSeqBlocks(project, &blocks);

   const sampleFormat format = QualityPrefs::SampleFormatChoice();
   ReplacedBlockFileHash blockFileHash;
   wxLongLong completedBytes = 0;
   for (const auto blockFile : blocks) {
      const auto &f = blockFile->f;
      if (f->IsAlias() && (blockFileHash.count( &*f ) == 0))
      {
         // f is an alias block we have not yet processed.
         auto aliasBlockFile = static_cast<AliasBlockFile*>( &*f );
         const wxFileName &fileName = aliasBlockFile->GetAliasedFileName();
         const wxString &fileNameStr = fileName.GetFullPath();

         if (aliasedFileHash.count(fileNameStr) == 0)
            // This aliased file was not selected to be replaced. Skip it.
            continue;

         // Convert it from an aliased file to an actual file in the project.
         auto len = aliasBlockFile->GetLength();
         BlockFilePtr newBlockFile;
         {
            SampleBuffer buffer(len, format);
            // We tolerate exceptions from NewBlockFile
            // and so we can allow exceptions from ReadData too
            f->ReadData(buffer.ptr(), format, 0, len);
            newBlockFile =
               dirManager.NewBlockFile( [&]( wxFileNameWrapper filePath ) {
                  return make_blockfile<SimpleBlockFile>(
                     std::move(filePath), buffer.ptr(), len, format);
               } );
         }

         // Update our hash so we know what block files we've done
         blockFileHash[ &*f ] = newBlockFile;

         // Update the progress bar
         completedBytes += SAMPLE_SIZE(format) * len;
         updateResult = progress.Update(completedBytes, totalBytesToProcess);
         if (updateResult != ProgressResult::Success)
            // leave the project unchanged
            return;
      }
   }

   // COMMIT OPERATIONS needing NOFAIL-GUARANTEE:

   // Above, we created a SimpleBlockFile contained in our project
   // to go with each AliasBlockFile that we wanted to migrate.
   // However, that didn't actually change any references to these
   // blockfiles in the Sequences, so we do that next...
   ReplaceBlockFiles(blocks, blockFileHash);
}

//
// DependencyDialog
//

class DependencyDialog final : public wxDialogWrapper
{
public:
   DependencyDialog(wxWindow *parent,
                    wxWindowID id,
                    AudacityProject *project,
                    AliasedFileArray &aliasedFiles,
                    bool isSaving);

private:
   void PopulateList();
   void PopulateOrExchange(ShuttleGui & S);

   // event handlers
   void OnCancel();
   void OnCopySelectedFiles();
   void OnList(wxListEvent &evt);
   void OnSize(wxSizeEvent &evt);
   void OnNo();
   void OnYes();
   void OnRightClick(wxListEvent& evt);
   void OnCopyToClipboard();


   void SaveFutureActionChoice();


   AudacityProject  *mProject;
   AliasedFileArray &mAliasedFiles;
   const bool        mIsSaving;
   bool              mHasMissingFiles;
   bool              mHasNonMissingFiles;
   int               mChoice = 0;

   wxStaticText     *mMessageStaticText;
   wxListCtrl       *mFileListCtrl = nullptr;

public:
   DECLARE_EVENT_TABLE()
};

enum {
   FileListID = 6000,
};

BEGIN_EVENT_TABLE(DependencyDialog, wxDialogWrapper)
   EVT_LIST_ITEM_SELECTED(FileListID, DependencyDialog::OnList)
   EVT_LIST_ITEM_DESELECTED(FileListID, DependencyDialog::OnList)
   EVT_LIST_ITEM_RIGHT_CLICK(FileListID, DependencyDialog::OnRightClick )
   EVT_SIZE(DependencyDialog::OnSize)
END_EVENT_TABLE()

DependencyDialog::DependencyDialog(wxWindow *parent,
                                   wxWindowID id,
                                   AudacityProject *project,
                                   AliasedFileArray &aliasedFiles,
                                   bool isSaving)
: wxDialogWrapper(parent, id, XO("Project Depends on Other Audio Files"),
            wxDefaultPosition, wxDefaultSize,
            (isSaving ?
                  (wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX) : // no close box when saving
                  wxDEFAULT_DIALOG_STYLE) |
               wxRESIZE_BORDER),
   mProject(project),
   mAliasedFiles(aliasedFiles),
   mIsSaving(isSaving),
   mHasMissingFiles(false),
   mHasNonMissingFiles(false),
   mMessageStaticText(NULL)
{
   SetName();
   ShuttleGui S(this);
   PopulateOrExchange(S);
}

static const TranslatableString kStdMsg()
{
   return
XO("Copying these files into your project will remove this dependency.\
\nThis is safer, but needs more disk space.");
}

static const TranslatableString kExtraMsgForMissingFiles()
{
   return
XO("\n\nFiles shown as MISSING have been moved or deleted and cannot be copied.\
\nRestore them to their original location to be able to copy into project.");
}

void DependencyDialog::PopulateOrExchange(ShuttleGui& S)
{
   using namespace DialogDefinition;
   S.StartVerticalLay();
   {
      S
         .AddVariableText(kStdMsg(), false)
         .Assign(mMessageStaticText);

      S.StartStatic(XO("Project Dependencies"),1);
      {
         S
            .Id(FileListID)
            .AddListControlReportMode({
               { XO("Audio File"), wxLIST_FORMAT_LEFT, 220 },
               { XO("Disk Space"), wxLIST_FORMAT_LEFT, 120 }
            })
            .Initialize([this](wxWindow*){ PopulateList(); })
            .Assign(mFileListCtrl);

         S
            .Focus()
            .Enable( [this]{ return mFileListCtrl &&
               mFileListCtrl->GetSelectedItemCount() > 0; })
            .Action( [this]{ OnCopySelectedFiles(); } )
            .AddButton(
               XXO("Copy Selected Files"),
               wxALIGN_LEFT, true);
      }
      S.EndStatic();

      S.StartHorizontalLay(wxALIGN_CENTRE,0);
      {
         if (mIsSaving) {
            S
               .Action( [this]{ OnCancel(); } )
               .AddButton(XXO("Cancel Save"));

            S
               .Action( [this]{ OnNo(); } )
               .AddButton(XXO("Save Without Copying"));
         }
         else
            S
               .Action( [this]{ OnNo(); } )
               .AddButton(XXO("Do Not Copy"));

         S
            // Enabling mCopyAllFilesButton is also done in PopulateList,
            // but at its call above, mCopyAllFilesButton does not yet exist.
            .Disable(mHasMissingFiles)
            .Enable( [this]{ return !mHasMissingFiles; } )
            .Action( [this]{ OnYes(); } )
            .AddButton(XXO("Copy All Files (Safer)"));
      }
      S.EndHorizontalLay();

      if (mIsSaving)
      {
         S.StartHorizontalLay(wxALIGN_LEFT,0);
         {
            S
               .Target( NumberChoice( mChoice,
                  TranslatableStrings{
                     /*i18n-hint: One of the choices of what you want Audacity to do when
                     * Audacity finds a project depends on another file.*/
                     XO("Ask me") ,
                     /*i18n-hint: One of the choices of what you want Audacity to do when
                     * Audacity finds a project depends on another file.*/
                     XO("Always copy all files (safest)") ,
                     /*i18n-hint: One of the choices of what you want Audacity to do when
                     * Audacity finds a project depends on another file.*/
                     XO("Never copy any files") ,
                  }
               ) )
               .AddChoice(XXO("Whenever a project depends on other files:") );
         }
         S.EndHorizontalLay();
      }
   }
   S.EndVerticalLay();
   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();
}

void DependencyDialog::PopulateList()
{
   mFileListCtrl->DeleteAllItems();

   mHasMissingFiles = false;
   mHasNonMissingFiles = false;
   long i = 0;
   for (const auto &aliasedFile : mAliasedFiles) {
      const wxFileName &fileName = aliasedFile.mFileName;
      wxLongLong byteCount = (aliasedFile.mByteCount * 124) / 100;
      bool bOriginalExists = aliasedFile.mbOriginalExists;

      if (bOriginalExists)
      {
         mFileListCtrl->InsertItem(i, fileName.GetFullPath());
         mHasNonMissingFiles = true;
         mFileListCtrl->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      }
      else
      {
         mFileListCtrl->InsertItem(i,
            wxString::Format( _("MISSING %s"), fileName.GetFullPath() ) );
         mHasMissingFiles = true;
         mFileListCtrl->SetItemState(i, 0, wxLIST_STATE_SELECTED); // Deselect.
         mFileListCtrl->SetItemTextColour(i, *wxRED);
      }
      mFileListCtrl->SetItem(i, 1, Internat::FormatSize(byteCount).Translation());
      mFileListCtrl->SetItemData(i, long(bOriginalExists));

      ++i;
   }

   auto msg = kStdMsg();
   if (mHasMissingFiles)
      msg += kExtraMsgForMissingFiles();
   mMessageStaticText->SetLabel(msg.Translation());
}

void DependencyDialog::OnList(wxListEvent &evt)
{
   if (!mFileListCtrl)
      return;

   if (evt.GetData() == 0)
      // This list item is one of mAliasedFiles for which
      // the original is missing, i.e., moved or deleted.
      // wxListCtrl does not provide for items that are not
      // allowed to be selected, so always deselect these items.
      mFileListCtrl->SetItemState(evt.GetIndex(), 0, wxLIST_STATE_SELECTED); // Deselect.
}

void DependencyDialog::OnSize(wxSizeEvent &evt)
{
   int fileListCtrlWidth, fileListCtrlHeight;
   mFileListCtrl->GetSize(&fileListCtrlWidth, &fileListCtrlHeight);

   // File path is column 0. File size is column 1.
   // File size column is always 120 px wide.
   // Also subtract 8 from file path column width for borders.
   mFileListCtrl->SetColumnWidth(0, fileListCtrlWidth - 120 - 8);
   mFileListCtrl->SetColumnWidth(1, 120);
   wxDialogWrapper::OnSize(evt);
}

void DependencyDialog::OnNo()
{
   SaveFutureActionChoice();
   EndModal(wxID_NO);
}

void DependencyDialog::OnYes()
{
   SaveFutureActionChoice();
   EndModal(wxID_YES);
}

void DependencyDialog::OnCopySelectedFiles()
{
   AliasedFileArray aliasedFilesToDelete, remainingAliasedFiles;

   long i = 0;
   for( const auto &file : mAliasedFiles ) {
      if (mFileListCtrl->GetItemState(i, wxLIST_STATE_SELECTED))
         aliasedFilesToDelete.push_back( file );
      else
         remainingAliasedFiles.push_back( file );
      ++i;
   }

   // provides STRONG-GUARANTEE
   RemoveDependencies(mProject, aliasedFilesToDelete);

   // COMMIT OPERATIONS needing NOFAIL-GUARANTEE:
   mAliasedFiles.swap( remainingAliasedFiles );
   PopulateList();

   if (mAliasedFiles.empty() || !mHasNonMissingFiles)
   {
      SaveFutureActionChoice();
      EndModal(wxID_NO);  // Don't need to remove dependencies
   }
}

void DependencyDialog::OnRightClick( wxListEvent& event)
{
   static_cast<void>(event);
   Widgets::MenuHandle menu;
   menu.Append(XXO("&Copy Names to Clipboard"), [this]{ OnCopyToClipboard(); } );
   menu.Popup(*this);
}

void DependencyDialog::OnCopyToClipboard()
{
   TranslatableString Files;
   for (const auto &aliasedFile : mAliasedFiles) {
      const wxFileName & fileName = aliasedFile.mFileName;
      wxLongLong byteCount = (aliasedFile.mByteCount * 124) / 100;
      bool bOriginalExists = aliasedFile.mbOriginalExists;
      // All fields quoted, as e.g. size may contain a comma in the number.
      Files += XO( "\"%s\", \"%s\", \"%s\"\n").Format(
         fileName.GetFullPath(), 
         Internat::FormatSize( byteCount),
         bOriginalExists ? XO("OK") : XO("Missing") );
   }

   // copy data onto clipboard
   if (wxTheClipboard->Open()) {
      // Clipboard owns the data you give it
      wxTheClipboard->SetData(safenew wxTextDataObject(Files.Translation()));
      wxTheClipboard->Close();
   }
}

void DependencyDialog::OnCancel()
{
   if (mIsSaving)
   {
      int ret = AudacityMessageBox(
         XO(
"If you proceed, your project will not be saved to disk. Is this what you want?"),
         XO("Cancel Save"),
         wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT, this);
      if (ret != wxYES)
         return;
   }

   EndModal(wxID_CANCEL);
}

void DependencyDialog::SaveFutureActionChoice()
{
   if (mIsSaving)
   {
      wxString savePref;
      switch ( mChoice )
      {
      case 1: savePref = L"copy"; break;
      case 2: savePref = L"never"; break;
      default: savePref = L"ask";
      }
      FileFormatsSaveWithDependenciesSetting.Write( savePref );
      gPrefs->Flush();
   }
}

// Checks for alias block files, modifies the project if the
// user requests it, and returns true if the user continues.
// Returns false only if the user clicks Cancel.
bool ShowDependencyDialogIfNeeded(AudacityProject *project,
                                  bool isSaving)
{
   auto pWindow = FindProjectFrame( project );
   AliasedFileArray aliasedFiles;
   FindDependencies(project, aliasedFiles);

   if (aliasedFiles.empty()) {
      if (!isSaving)
      {
         auto msg =
XO("Your project is self-contained; it does not depend on any external audio files. \
\n\nSome older Audacity projects may not be self-contained, and care \n\
is needed to keep their external dependencies in the right place.\n\
New projects will be self-contained and are less risky.");
         AudacityMessageBox(
            msg,
            XO("Dependency Check"),
            wxOK | wxICON_INFORMATION,
            pWindow);
      }
      return true; // Nothing to do.
   }

   if (isSaving)
   {
      RemoveDependencies(project, aliasedFiles);
      return true;
   }

   DependencyDialog dlog(pWindow, -1, project, aliasedFiles, isSaving);
   int returnCode = dlog.ShowModal();
   if (returnCode == wxID_CANCEL)
      return false;
   else if (returnCode == wxID_YES)
      RemoveDependencies(project, aliasedFiles);

   return true;
}

