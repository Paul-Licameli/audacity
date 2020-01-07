#include "../AudioIOBase.h"
#include "../Clipboard.h"
#include "../CommonCommandFlags.h"
#include "../LabelTrack.h"
#include "../Menus.h"
#include "../Prefs.h"
#include "../ProjectAudioIO.h"
#include "../ProjectHistory.h"
#include "../ProjectSettings.h"
#include "../ProjectWindow.h"
#include "../TrackPanelAx.h"
#include "../TrackPanel.h"
#include "../ViewInfo.h"
#include "../WaveTrack.h"
#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../tracks/labeltrack/ui/LabelTrackView.h"

// private helper classes and functions
namespace {

//Adds label and returns index of label in labeltrack.
int DoAddLabel(
   AudacityProject &project, const SelectedRegion &region,
   bool preserveFocus = false)
{
   auto &tracks = TrackList::Get( project );
   auto &trackFocus = TrackFocus::Get( project );
   auto &trackPanel = TrackPanel::Get( project );
   auto &trackFactory = TrackFactory::Get( project );
   auto &window = ProjectWindow::Get( project );

   wxString title;      // of label

   bool useDialog;
   gPrefs->Read(wxT("/GUI/DialogForNameNewLabel"), &useDialog, false);
   if (useDialog) {
      if (LabelTrackView::DialogForLabelName(
         project, region, wxEmptyString, title) == wxID_CANCEL)
         return -1;     // index
   }

   // If the focused track is a label track, use that
   const auto pFocusedTrack = trackFocus.Get();

   // Look for a label track at or after the focused track
   auto iter = pFocusedTrack
      ? tracks.Find(pFocusedTrack)
      : tracks.Any().begin();
   auto lt = * iter.Filter< LabelTrack >();

   // If none found, start a NEW label track and use it
   if (!lt)
      lt = tracks.Add( trackFactory.NewLabelTrack() );

// LLL: Commented as it seemed a little forceful to remove users
//      selection when adding the label.  This does not happen if
//      you select several tracks and the last one of those is a
//      label track...typing a label will not clear the selections.
//
//   SelectNone();
   lt->SetSelected(true);

   int index;
   if (useDialog) {
      index = lt->AddLabel(region, title);
   }
   else {
      int focusTrackNumber = -1;
      if (pFocusedTrack && preserveFocus) {
         // Must remember the track to re-focus after finishing a label edit.
         // do NOT identify it by a pointer, which might dangle!  Identify
         // by position.
         focusTrackNumber = pFocusedTrack->GetIndex();
      }
      index =
         LabelTrackView::Get( *lt ).AddLabel(region, title, focusTrackNumber);
   }

   ProjectHistory::Get( project )
      .PushState(XO("Added label"), XO("Label"));

   if (!useDialog) {
      TrackFocus::Get(project).Set(lt);
      lt->EnsureVisible();
   }
   trackPanel.SetFocus();

   return index;
}

//get regions selected by selected labels
//removes unnecessary regions, overlapping regions are merged
void GetRegionsByLabel(
   const TrackList &tracks, const SelectedRegion &selectedRegion,
   Regions &regions )
{
   //determine labeled regions
   for (auto lt : tracks.Selected< const LabelTrack >()) {
      for (int i = 0; i < lt->GetNumLabels(); i++)
      {
         const LabelStruct *ls = lt->GetLabel(i);
         if (ls->selectedRegion.t0() >= selectedRegion.t0() &&
            ls->selectedRegion.t1() <= selectedRegion.t1())
            regions.push_back(Region(ls->getT0(), ls->getT1()));
      }
   }

   //anything to do ?
   if( regions.size() == 0 )
      return;

   //sort and remove unnecessary regions
   std::sort(regions.begin(), regions.end());
   unsigned int selected = 1;
   while( selected < regions.size() )
   {
      const Region &cur = regions.at( selected );
      Region &last = regions.at( selected - 1 );
      if( cur.start < last.end )
      {
         if( cur.end > last.end )
            last.end = cur.end;
         regions.erase( regions.begin() + selected );
      }
      else
         ++selected;
   }
}

using EditFunction = std::function< void( WaveTrack*, double, double ) >;
static EditFunction clearFn( double rate )
{
   return [rate]( WaveTrack *wt, double t0, double t1 ){
      return wt->Clear( t0, t1, rate );
   };
}

//Executes the edit function on all selected wave tracks with
//regions specified by selected labels
//If No tracks selected, function is applied on all tracks
//If the function replaces the selection with audio of a different length,
// bSyncLockedTracks should be set true to perform the same action on sync-lock
// selected tracks.
void EditByLabel(
   TrackList &tracks, const SelectedRegion &selectedRegion,
   EditFunction action, bool bSyncLockedTracks )
{
   Regions regions;

   GetRegionsByLabel( tracks, selectedRegion, regions );
   if( regions.size() == 0 )
      return;

   // if at least one wave track is selected
   // apply only on the selected track
   const bool allTracks = (tracks.Selected< WaveTrack >()).empty();

   //Apply action on wavetracks starting from
   //labeled regions in the end. This is to correctly perform
   //actions like 'Delete' which collapse the track area.
   for (auto wt : tracks.Any<WaveTrack>())
   {
      if (allTracks || wt->GetSelected() ||
          (bSyncLockedTracks && wt->IsSyncLockSelected()))
      {
         for (int i = (int)regions.size() - 1; i >= 0; i--) {
            const Region &region = regions.at(i);
            action( wt, region.start, region.end );
         }
      }
   }
}

using EditDestFunction = std::shared_ptr<Track> (WaveTrack::*)(double, double);

//Executes the edit function on all selected wave tracks with
//regions specified by selected labels
//If No tracks selected, function is applied on all tracks
//Functions copy the edited regions to clipboard, possibly in multiple tracks
//This probably should not be called if *action() changes the timeline, because
// the copy needs to happen by track, and the timeline change by group.
void EditClipboardByLabel( AudacityProject &project,
   TrackList &tracks, const SelectedRegion &selectedRegion,
   EditDestFunction action )
{
   auto &settings = ProjectSettings::Get( project );

   Regions regions;

   GetRegionsByLabel( tracks, selectedRegion, regions );
   if( regions.size() == 0 )
      return;

   // if at least one wave track is selected
   // apply only on the selected track
   const bool allTracks = (tracks.Selected< WaveTrack >()).empty();

   auto &clipboard = Clipboard::Get();
   clipboard.Clear();

   auto pNewClipboard = TrackList::Create( nullptr );
   auto &newClipboard = *pNewClipboard;

   //Apply action on wavetracks starting from
   //labeled regions in the end. This is to correctly perform
   //actions like 'Cut' which collapse the track area.

   for(auto wt :
         tracks.Any<WaveTrack>()
            + (allTracks ? &Track::Any : &Track::IsSelected)
   ) {
      // This track accumulates the needed clips, right to left:
      Track::Holder merged;
      for( int i = (int)regions.size() - 1; i >= 0; i-- )
      {
         const Region &region = regions.at(i);
         auto dest = ( wt->*action )( region.start, region.end );
         if( dest )
         {
            Track::FinishCopy( wt, dest.get() );
            if( !merged )
               merged = dest;
            else
            {
               // Paste to the beginning; unless this is the first region,
               // offset the track to account for time between the regions
               if (i < (int)regions.size() - 1)
                  merged->Offset(
                     regions.at(i + 1).start - region.end);

               // dest may have a placeholder clip at the end that is
               // removed when pasting, which is okay because we proceed
               // right to left.  Any placeholder already in merged is kept.
               // Only the rightmost placeholder is important in the final
               // result.
               merged->Paste( 0.0 , dest.get(), settings.GetRate() );
            }
         }
         else
            // nothing copied but there is a 'region', so the 'region' must
            // be a 'point label' so offset
            if (i < (int)regions.size() - 1)
               if (merged)
                  merged->Offset(
                     regions.at(i + 1).start - region.end);
      }
      if( merged )
         newClipboard.Add( merged );
   }

   // Survived possibility of exceptions.  Commit changes to the clipboard now.
   clipboard.Assign( std::move( newClipboard ),
      regions.front().start, regions.back().end, &project );
}

}

/// Namespace for functions for Edit Label submenu
namespace LabelEditActions {

// exported helper functions
// none

// Menu handler functions

struct Handler : CommandHandlerObject {

void OnEditLabels(const CommandContext &context)
{
   auto &project = context.project;
   LabelTrackView::DoEditLabels(project);
}

void OnAddLabel(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   DoAddLabel(project, selectedRegion);
}

void OnAddLabelPlaying(const CommandContext &context)
{
   auto &project = context.project;
   auto token = ProjectAudioIO::Get( project ).GetAudioIOToken();

   auto gAudioIO = AudioIOBase::Get();
   if (token > 0 &&
       gAudioIO->IsStreamActive(token)) {
      double indicator = gAudioIO->GetStreamTime();
      DoAddLabel(project, SelectedRegion(indicator, indicator), true);
   }
}

// Creates a NEW label in each selected label track with text from the system
// clipboard
void OnPasteNewLabel(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &trackFactory = TrackFactory::Get( project );
   auto &trackPanel = TrackPanel::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   bool bPastedSomething = false;

   {
      auto trackRange = tracks.Selected< const LabelTrack >();
      if (trackRange.empty())
      {
         // If there are no selected label tracks, try to choose the first label
         // track after some other selected track
         Track *t = *tracks.Selected().begin()
            .Filter( &Track::Any )
            .Filter<LabelTrack>();

         // If no match found, add one
         if (!t)
            t = tracks.Add( trackFactory.NewLabelTrack() );

         // Select this track so the loop picks it up
         t->SetSelected(true);
      }
   }

   LabelTrack *plt = NULL; // the previous track
   for ( auto lt : tracks.Selected< LabelTrack >() )
   {
      // Unselect the last label, so we'll have just one active label when
      // we're done
      if (plt)
         LabelTrackView::Get( *plt ).SetSelectedIndex( -1 );

      // Add a NEW label, paste into it
      // Paul L:  copy whatever defines the selected region, not just times
      auto &view = LabelTrackView::Get( *lt );
      view.AddLabel(selectedRegion);
      if (view.PasteSelectedText( context.project, selectedRegion.t0(),
                                selectedRegion.t1() ))
         bPastedSomething = true;

      // Set previous track
      plt = lt;
   }

   // plt should point to the last label track pasted to -- ensure it's visible
   // and set focus
   if (plt) {
      TrackFocus::Get(project).Set(plt);
      plt->EnsureVisible();
      trackPanel.SetFocus();
   }

   if (bPastedSomething) {
      ProjectHistory::Get( project ).PushState(
         XO("Pasted from the clipboard"), XO("Paste Text to New Label"));
   }
}

void OnToggleTypeToCreateLabel(const CommandContext &WXUNUSED(context) )
{
   bool typeToCreateLabel;
   gPrefs->Read(wxT("/GUI/TypeToCreateLabel"), &typeToCreateLabel, false);
   gPrefs->Write(wxT("/GUI/TypeToCreateLabel"), !typeToCreateLabel);
   gPrefs->Flush();
   MenuManager::ModifyAllProjectToolbarMenus();
}

void OnCutLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &settings = ProjectSettings::Get( project );
   auto &window = ProjectWindow::Get( project );

   auto rate = settings.GetRate();
   
   if( selectedRegion.isPoint() )
     return;

   // Because of grouping the copy may need to operate on different tracks than
   // the clear, so we do these actions separately.
   EditClipboardByLabel( project,
      tracks, selectedRegion, &WaveTrack::CopyNonconst );

   if( gPrefs->Read( wxT( "/GUI/EnableCutLines" ), ( long )0 ) )
      EditByLabel(
         tracks, selectedRegion, &WaveTrack::ClearAndAddCutLine, true );
   else
      EditByLabel( tracks, selectedRegion, clearFn( rate ), true );

   selectedRegion.collapseToT0();

   ProjectHistory::Get( project ).PushState(
   /* i18n-hint: (verb) past tense.  Audacity has just cut the labeled audio
      regions.*/
      XO( "Cut labeled audio regions to clipboard" ),
   /* i18n-hint: (verb)*/
      XO( "Cut Labeled Audio" ) );
}

void OnDeleteLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &settings = ProjectSettings::Get( project );
   auto &window = ProjectWindow::Get( project );

   auto rate = settings.GetRate();

   if( selectedRegion.isPoint() )
      return;

   EditByLabel( tracks, selectedRegion, clearFn( rate ), true );

   selectedRegion.collapseToT0();

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb) Audacity has just deleted the labeled audio regions*/
      XO( "Deleted labeled audio regions" ),
      /* i18n-hint: (verb)*/
      XO( "Delete Labeled Audio" ) );
}

void OnSplitCutLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   if( selectedRegion.isPoint() )
      return;

   EditClipboardByLabel( project,
      tracks, selectedRegion, &WaveTrack::SplitCut );

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb) Audacity has just split cut the labeled audio
         regions*/
      XO( "Split Cut labeled audio regions to clipboard" ),
      /* i18n-hint: (verb) Do a special kind of cut on the labels*/
      XO( "Split Cut Labeled Audio" ) );
}

void OnSplitDeleteLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   if( selectedRegion.isPoint() )
      return;

   EditByLabel( tracks, selectedRegion, &WaveTrack::SplitDelete, false );

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb) Audacity has just done a special kind of DELETE on
         the labeled audio regions */
      XO( "Split Deleted labeled audio regions" ),
      /* i18n-hint: (verb) Do a special kind of DELETE on labeled audio
         regions */
      XO( "Split Delete Labeled Audio" ) );
}

void OnSilenceLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   if( selectedRegion.isPoint() )
      return;

   EditByLabel( tracks, selectedRegion, &WaveTrack::Silence, false );

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb)*/
      XO( "Silenced labeled audio regions" ),
      /* i18n-hint: (verb)*/
      XO( "Silence Labeled Audio" ) );
}

void OnCopyLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   if( selectedRegion.isPoint() )
      return;

   EditClipboardByLabel( project,
      tracks, selectedRegion, &WaveTrack::CopyNonconst );

   ProjectHistory::Get( project ).PushState( XO( "Copied labeled audio regions to clipboard" ),
   /* i18n-hint: (verb)*/
      XO( "Copy Labeled Audio" ) );
}

void OnSplitLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   EditByLabel( tracks, selectedRegion, &WaveTrack::Split, false );

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb) past tense.  Audacity has just split the labeled
         audio (a point or a region)*/
      XO( "Split labeled audio (points or regions)" ),
      /* i18n-hint: (verb)*/
      XO( "Split Labeled Audio" ) );
}

void OnJoinLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   if( selectedRegion.isPoint() )
      return;

   EditByLabel( tracks, selectedRegion, &WaveTrack::Join, false );

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb) Audacity has just joined the labeled audio (points or
         regions) */
      XO( "Joined labeled audio (points or regions)" ),
      /* i18n-hint: (verb) */
      XO( "Join Labeled Audio" ) );
}

void OnDisjoinLabels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   if( selectedRegion.isPoint() )
      return;

   EditByLabel( tracks, selectedRegion, &WaveTrack::Disjoin, false );

   ProjectHistory::Get( project ).PushState(
      /* i18n-hint: (verb) Audacity has just detached the labeled audio regions.
      This message appears in history and tells you about something
      Audacity has done.*/
      XO( "Detached labeled audio regions" ),
      /* i18n-hint: (verb)*/
      XO( "Detach Labeled Audio" ) );
}

}; // struct Handler

} // namespace

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static LabelEditActions::Handler instance;
   return instance;
};

// Menu definitions

#define FN(X) findCommandHandler, \
   static_cast<CommandFunctorPointer>(& LabelEditActions::Handler :: X)

MenuTable::BaseItemPtr LabelEditMenus( AudacityProject & )
{
   using namespace MenuTable;
   using Options = CommandManager::Options;

   static const auto checkOff = Options{}.CheckState( false );

   static const auto NotBusyLabelsAndWaveFlags =
      AudioIONotBusyFlag |
      LabelsSelectedFlag | WaveTracksExistFlag | TimeSelectedFlag;

   // Returns TWO menus.
   
   return Items(

   Menu( XO("&Labels"),
      Command( wxT("EditLabels"), XXO("&Edit Labels..."), FN(OnEditLabels),
                 AudioIONotBusyFlag ),

      Separator(),

      Command( wxT("AddLabel"), XXO("Add Label at &Selection"),
         FN(OnAddLabel), AlwaysEnabledFlag, wxT("Ctrl+B") ),
      Command( wxT("AddLabelPlaying"),
         XXO("Add Label at &Playback Position"),
         FN(OnAddLabelPlaying), AudioIOBusyFlag,
#ifdef __WXMAC__
         wxT("Ctrl+.")
#else
         wxT("Ctrl+M")
#endif
      ),
      Command( wxT("PasteNewLabel"), XXO("Paste Te&xt to New Label"),
         FN(OnPasteNewLabel),
         AudioIONotBusyFlag, wxT("Ctrl+Alt+V") ),

      Separator(),

      Command( wxT("TypeToCreateLabel"),
         XXO("&Type to Create a Label (on/off)"),
         FN(OnToggleTypeToCreateLabel), AlwaysEnabledFlag, checkOff )
   ), // first menu

   /////////////////////////////////////////////////////////////////////////////

   Menu( XO("La&beled Audio"),
      /* i18n-hint: (verb)*/
      Command( wxT("CutLabels"), XXO("&Cut"), FN(OnCutLabels),
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag |
            TimeSelectedFlag | IsNotSyncLockedFlag,
            Options{ wxT("Alt+X"), XO("Label Cut") } ),
      Command( wxT("DeleteLabels"), XXO("&Delete"), FN(OnDeleteLabels),
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag |
            TimeSelectedFlag | IsNotSyncLockedFlag,
         Options{ wxT("Alt+K"), XO("Label Delete") } ),

      Separator(),

      /* i18n-hint: (verb) A special way to cut out a piece of audio*/
      Command( wxT("SplitCutLabels"), XXO("&Split Cut"),
         FN(OnSplitCutLabels), NotBusyLabelsAndWaveFlags,
         Options{ wxT("Alt+Shift+X"), XO("Label Split Cut") } ),
      Command( wxT("SplitDeleteLabels"), XXO("Sp&lit Delete"),
         FN(OnSplitDeleteLabels), NotBusyLabelsAndWaveFlags,
         Options{ wxT("Alt+Shift+K"), XO("Label Split Delete") } ),

      Separator(),

      Command( wxT("SilenceLabels"), XXO("Silence &Audio"),
         FN(OnSilenceLabels), NotBusyLabelsAndWaveFlags,
         Options{ wxT("Alt+L"), XO("Label Silence") } ),
      /* i18n-hint: (verb)*/
      Command( wxT("CopyLabels"), XXO("Co&py"), FN(OnCopyLabels),
         NotBusyLabelsAndWaveFlags,
         Options{ wxT("Alt+Shift+C"), XO("Label Copy") } ),

      Separator(),

      /* i18n-hint: (verb)*/
      Command( wxT("SplitLabels"), XXO("Spli&t"), FN(OnSplitLabels),
         AudioIONotBusyFlag | LabelsSelectedFlag | WaveTracksExistFlag,
         Options{ wxT("Alt+I"), XO("Label Split") } ),
      /* i18n-hint: (verb)*/
      Command( wxT("JoinLabels"), XXO("&Join"), FN(OnJoinLabels),
         NotBusyLabelsAndWaveFlags,
         Options{ wxT("Alt+J"), XO("Label Join") } ),
      Command( wxT("DisjoinLabels"), XXO("Detac&h at Silences"),
         FN(OnDisjoinLabels), NotBusyLabelsAndWaveFlags,
         wxT("Alt+Shift+J") )
   ) // second menu

   ); // two menus
}

#undef FN
