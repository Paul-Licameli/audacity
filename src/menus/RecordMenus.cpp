/**********************************************************************

  Audacity: A Digital Audio Editor

  @file RecordMenus.cpp
  @brief headerless file implements recording menu items

  Paul Licameli split from ProjectAudioManager.cpp and TransportUtilities.cpp

**********************************************************************/
#include "../widgets/AudacityMessageBox.h"
#include "../AudioIO.h"
#include "BasicUI.h"
#include "CommandContext.h"
#include "CommandManager.h"
#include "../CommonCommandFlags.h"
#include "../widgets/ProgressDialog.h"
#include "../ProjectAudioManager.h"
#include "../ProjectHistory.h"
#include "ProjectWindows.h"
#include "../RecordUtilities.h"
#include "ViewInfo.h"
#include "../WaveClip.h"
#include "../WaveTrack.h"

#include <wx/frame.h>

namespace {
void RecordAndWait(
   const CommandContext &context, bool altAppearance)
{
   using namespace RecordUtilities;

   auto &project = context.project;
   auto &projectAudioManager = ProjectAudioManager::Get(project);

   const auto &selectedRegion = ViewInfo::Get(project).selectedRegion;
   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();

   OnRecord(project, altAppearance);

   if (project.mBatchMode > 0 && t1 != t0) {
      wxYieldIfNeeded();

      /* i18n-hint: This title appears on a dialog that indicates the progress
         in doing something.*/
      ProgressDialog progress(XO("Progress"), XO("Recording"), pdlgHideCancelButton);
      auto gAudioIO = AudioIO::Get();

      while (projectAudioManager.Recording()) {
         ProgressResult result = progress.Update(gAudioIO->GetStreamTime() - t0, t1 - t0);
         if (result != ProgressResult::Success) {
            projectAudioManager.Stop();
            if (result != ProgressResult::Stopped) {
               context.Error(wxT("Recording interrupted"));
            }
            break;
         }

         wxMilliSleep(100);
         wxYieldIfNeeded();
      }

      projectAudioManager.Stop();
      wxYieldIfNeeded();
   }
}

struct Handler : CommandHandlerObject {
void OnRecord(const CommandContext &context)
{
   RecordAndWait(context, false);
}

// If first choice is record same track 2nd choice is record NEW track
// and vice versa.
void OnRecord2ndChoice(const CommandContext &context)
{
   RecordAndWait(context, true);
}

#ifdef EXPERIMENTAL_PUNCH_AND_ROLL
void OnPunchAndRoll(const CommandContext &context)
{
   using namespace RecordUtilities;

   AudacityProject &project = context.project;
   auto &viewInfo = ViewInfo::Get( project );

   static const auto url =
      wxT("Punch_and_Roll_Record#Using_Punch_and_Roll_Record");

   auto gAudioIO = AudioIO::Get();
   if (gAudioIO->IsBusy())
      return;

   // Ignore all but left edge of the selection.
   viewInfo.selectedRegion.collapseToT0();
   double t1 = std::max(0.0, viewInfo.selectedRegion.t1());

   // Checking the selected tracks: making sure they all have the same rate
   const auto selectedTracks{ GetPropertiesOfSelected(project) };
   const int rateOfSelected{ selectedTracks.rateOfSelected };
   const bool allSameRate{ selectedTracks.allSameRate };

   if (!allSameRate) {
      AudacityMessageBox(XO("The tracks selected "
         "for recording must all have the same sampling rate"),
         XO("Mismatched Sampling Rates"),
         wxICON_ERROR | wxCENTRE);

      return;
   }

   // Decide which tracks to record in.
   auto tracks =
      ChooseExistingRecordingTracks(project, true, rateOfSelected);
   if (tracks.empty()) {
      auto recordingChannels =
         std::max(0, AudioIORecordChannels.Read());
      auto message =
         (recordingChannels == 1)
         ? XO("Please select in a mono track.")
         : (recordingChannels == 2)
         ? XO("Please select in a stereo track or two mono tracks.")
         : XO("Please select at least %d channels.").Format( recordingChannels );
      BasicUI::ShowErrorDialog( *ProjectFramePlacement(&project),
         XO("Error"), message, url);
      return;
   }

   // Delete the portion of the target tracks right of the selection, but first,
   // remember a part of the deletion for crossfading with the new recording.
   // We may also adjust the starting point leftward if it is too close to the
   // end of the track, so that at least some nonzero crossfade data can be
   // taken.
   PRCrossfadeData crossfadeData;
   const double crossFadeDuration =
      std::max(0.0, RecordCrossfadeDuration.Read()) / 1000.0;

   // The test for t1 == 0.0 stops punch and roll deleting everything where the
   // selection is at zero.  There wouldn't be any cued audio to play in
   // that case, so a normal record, not a punch and roll, is called for.
   bool error = (t1 == 0.0);

   double newt1 = t1;
   for (const auto &wt : tracks) {
      auto rate = wt->GetRate();
      sampleCount testSample(floor(t1 * rate));
      auto intervals = wt->GetIntervals();
      auto pred = [rate](sampleCount testSample){ return
         [rate, testSample](const Track::Interval &interval){
            auto start = floor(interval.Start() * rate + 0.5);
            auto end = start + (interval.End() - start);
            auto ts = testSample.as_double();
            return ts >= start && ts < end;
         };
      };
      auto begin = intervals.begin(), end = intervals.end(),
         iter = std::find_if(begin, end, pred(testSample));
      if (iter == end)
         // Bug 1890 (an enhancement request)
         // Try again, a little to the left.
         // Subtract 10 to allow a selection exactly at or slightly after the
         // end time
         iter = std::find_if(begin, end, pred(testSample + 10));
      if (iter == end)
         error = true;
      else {
         // May adjust t1 left
         // Let's ignore the possibility of a clip even shorter than the
         // crossfade duration!
         newt1 = std::min(newt1, iter->End() - crossFadeDuration);
      }
   }

   if (error) {
      auto message = XO("Please select a time within a clip.");
      BasicUI::ShowErrorDialog( *ProjectFramePlacement(&project),
         XO("Error"), message, url);
      return;
   }

   t1 = newt1;
   for (const auto &wt : tracks) {
      const auto endTime = wt->GetEndTime();
      const auto duration =
         std::max(0.0, std::min(crossFadeDuration, endTime - t1));
      const size_t getLen = floor(duration * wt->GetRate());
      std::vector<float> data(getLen);
      if (getLen > 0) {
         float *const samples = data.data();
         const sampleCount pos = wt->TimeToLongSamples(t1);
         wt->GetFloats(samples, pos, getLen);
      }
      crossfadeData.push_back(std::move(data));
   }

   // Change tracks only after passing the error checks above
   for (const auto &wt : tracks) {
      wt->Clear(t1, wt->GetEndTime());
   }

   // Choose the tracks for playback.
   TransportTracks transportTracks;
   const auto duplex = UseDuplex();
   if (duplex)
      // play all
      transportTracks =
         ProjectAudioManager::GetAllPlaybackTracks(
            TrackList::Get( project ), false, true);
   else
      // play recording tracks only
      std::copy(tracks.begin(), tracks.end(),
         std::back_inserter(transportTracks.playbackTracks));
      
   // Unlike with the usual recording, a track may be chosen both for playback
   // and recording.
   transportTracks.captureTracks = std::move(tracks);

   // Try to start recording
   auto options = DefaultPlayOptions( project );
   options.rate = rateOfSelected;
   options.preRoll = std::max(0.0, RecordPreRollDuration.Read());
   options.pCrossfadeData = &crossfadeData;
   bool success = DoRecord(project,
      transportTracks,
      t1, DBL_MAX,
      false, // altAppearance
      options);

   if (success)
      // Undo state will get pushed elsewhere, when record finishes
      ;
   else
      // Roll back the deletions
      ProjectHistory::Get( project ).RollbackState();
}

void OnPause(const CommandContext &context)
{
   ProjectAudioManager::Get( context.project ).OnPause();
}
};

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static Handler instance;
   return instance;
};

#define FN(X) (& Handler :: X)
using namespace MenuTable;
BaseItemSharedPtr RecordMenu()
{
   static const auto CanStopFlags = AudioIONotBusyFlag() | CanStopAudioStreamFlag();
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( wxT("Record"), XXO("&Recording"),
      /* i18n-hint: (verb)*/
      Command( wxT("Record1stChoice"), XXO("&Record"), FN(OnRecord),
         CanStopFlags, wxT("R") ),

      // The OnRecord2ndChoice function is: if normal record records beside,
      // it records below, if normal record records below, it records beside.
      // TODO: Do 'the right thing' with other options like TimerRecord.
      // Delayed evaluation in case gPrefs is not yet defined
      [](const AudacityProject&)
      { return Command( wxT("Record2ndChoice"),
         // Our first choice is bound to R (by default)
         // and gets the prime position.
         // We supply the name for the 'other one' here.
         // It should be bound to Shift+R
         (gPrefs->ReadBool("/GUI/PreferNewTrackRecord", false)
          ? XXO("&Append Record") : XXO("Record &New Track")),
         FN(OnRecord2ndChoice), CanStopFlags,
         wxT("Shift+R"),
         findCommandHandler
      ); },

#ifdef EXPERIMENTAL_PUNCH_AND_ROLL
      Command( wxT("PunchAndRoll"), XXO("Punch and Rol&l Record"),
         FN(OnPunchAndRoll),
         WaveTracksExistFlag() | AudioIONotBusyFlag(), wxT("Shift+D") ),
#endif

      // JKC: I decided to duplicate this between play and record,
      // rather than put it at the top level.
      // CommandManger::AddItem can now cope with simple duplicated items.
      // PRL:  caution, this is a duplicated command name!
      Command( wxT("Pause"), XXO("&Pause"), FN(OnPause),
         CanStopAudioStreamFlag(), wxT("P") )
   ) ) };
   return menu;
}

AttachedItem sAttachment1{
   Placement{ L"Transport/Basic", { OrderingHint::After, L"Play" } },
   Shared( RecordMenu() )
};

#undef FN

}
#endif
