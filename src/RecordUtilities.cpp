/**********************************************************************

  Audacity: A Digital Audio Editor

  @file RecordUtilties.cpp

  Paul Licameli split from ProjectAudioManager.cpp and TransportUtilities.cpp

**********************************************************************/
#include "RecordUtilities.h"

#include "widgets/AudacityMessageBox.h"
#include "AudioIO.h"
#include "BasicUI.h"
#include "CommonCommandFlags.h"
#include "Menus.h"
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectAudioManager.h"
#include "ProjectSettings.h"
#include "ProjectWindows.h"
#include "TrackPanelAx.h"
#include "tracks/ui/TrackView.h"
#include "ViewInfo.h"
#include "WaveTrack.h"

#include <wx/frame.h>

namespace RecordUtilities {
bool UseDuplex()
{
   bool duplex;
   gPrefs->Read(wxT("/AudioIO/Duplex"), &duplex,
#ifdef EXPERIMENTAL_DA
      false
#else
      true
#endif
      );
   return duplex;
}

WaveTrackArray ChooseExistingRecordingTracks(
   AudacityProject &proj, bool selectedOnly, double targetRate)
{
   auto p = &proj;
   size_t recordingChannels = std::max(0, AudioIORecordChannels.Read());
   bool strictRules = (recordingChannels <= 2);

   // Iterate over all wave tracks, or over selected wave tracks only.
   // If target rate was specified, ignore all tracks with other rates.
   //
   // In the usual cases of one or two recording channels, seek a first-fit
   // unbroken sub-sequence for which the total number of channels matches the
   // required number exactly.  Never drop inputs or fill only some channels
   // of a track.
   //
   // In case of more than two recording channels, choose tracks only among the
   // selected.  Simply take the earliest wave tracks, until the number of
   // channels is enough.  If there are fewer channels than inputs, but at least
   // one channel, then some of the input channels will be dropped.
   //
   // Resulting tracks may be non-consecutive within the list of all tracks
   // (there may be non-wave tracks between, or non-selected tracks when
   // considering selected tracks only.)

   if (!strictRules && !selectedOnly)
      return {};

   auto &trackList = TrackList::Get( *p );
   std::vector<unsigned> channelCounts;
   WaveTrackArray candidates;
   const auto range = trackList.Leaders<WaveTrack>();
   for ( auto candidate : selectedOnly ? range + &Track::IsSelected : range ) {
      if (targetRate != RATE_NOT_SELECTED && candidate->GetRate() != targetRate)
         continue;

      // count channels in this track
      const auto channels = TrackList::Channels( candidate );
      unsigned nChannels = channels.size();

      if (strictRules && nChannels > recordingChannels) {
         // The recording would under-fill this track's channels
         // Can't use any partial accumulated results
         // either.  Keep looking.
         candidates.clear();
         channelCounts.clear();
         continue;
      }
      else {
         // Might use this but may have to discard some of the accumulated
         while(strictRules &&
               nChannels + candidates.size() > recordingChannels) {
            auto nOldChannels = channelCounts[0];
            wxASSERT(nOldChannels > 0);
            channelCounts.erase(channelCounts.begin());
            candidates.erase(candidates.begin(),
                             candidates.begin() + nOldChannels);
         }
         channelCounts.push_back(nChannels);
         for ( auto channel : channels ) {
            candidates.push_back(channel->SharedPointer<WaveTrack>());
            if(candidates.size() == recordingChannels)
               // Done!
               return candidates;
         }
      }
   }

   if (!strictRules && !candidates.empty())
      // good enough
      return candidates;

   // If the loop didn't exit early, we could not find enough channels
   return {};
}

bool DoRecord(AudacityProject &project,
   const TransportTracks &tracks,
   double t0, double t1,
   bool altAppearance,
   const AudioIOStartStreamOptions &options)
{
   auto &projectAudioManager = ProjectAudioManager::Get(project);

   CommandFlag flags = AlwaysEnabledFlag; // 0 means recalc flags.

   // NB: The call may have the side effect of changing flags.
   bool allowed = MenuManager::Get(project).TryToMakeActionAllowed(
      flags,
      AudioIONotBusyFlag() | CanStopAudioStreamFlag());

   if (!allowed)
      return false;
   // ...end of code from CommandHandler.

   auto gAudioIO = AudioIO::Get();
   if (gAudioIO->IsBusy())
      return false;

   projectAudioManager.SetAppending( !altAppearance );

   bool success = false;

   auto transportTracks = tracks;

   // Will replace any given capture tracks with temporaries
   transportTracks.captureTracks.clear();

   const auto p = &project;

   bool appendRecord = !tracks.captureTracks.empty();

   {
      if (appendRecord) {
         // Append recording:
         // Pad selected/all wave tracks to make them all the same length
         for (const auto &wt : tracks.captureTracks)
         {
            auto endTime = wt->GetEndTime();

            // If the track was chosen for recording and playback both,
            // remember the original in preroll tracks, before making the
            // pending replacement.
            bool prerollTrack = make_iterator_range(transportTracks.playbackTracks).contains(wt);
            if (prerollTrack)
                  transportTracks.prerollTracks.push_back(wt);

            // A function that copies all the non-sample data between
            // wave tracks; in case the track recorded to changes scale
            // type (for instance), during the recording.
            auto updater = [](Track &d, const Track &s){
               auto &dst = static_cast<WaveTrack&>(d);
               auto &src = static_cast<const WaveTrack&>(s);
               dst.Reinit(src);
            };

            // Get a copy of the track to be appended, to be pushed into
            // undo history only later.
            auto pending = std::static_pointer_cast<WaveTrack>(
               TrackList::Get( *p ).RegisterPendingChangedTrack(
                  updater, wt.get() ) );

            // End of current track is before or at recording start time.
            // Less than or equal, not just less than, to ensure a clip boundary.
            // when append recording.
            if (endTime <= t0) {
               pending->CreateClip(t0);
            }
            transportTracks.captureTracks.push_back(pending);
         }
         TrackList::Get( *p ).UpdatePendingTracks();
      }

      if( transportTracks.captureTracks.empty() )
      {   // recording to NEW track(s).
         bool recordingNameCustom, useTrackNumber, useDateStamp, useTimeStamp;
         wxString defaultTrackName, defaultRecordingTrackName;

         // Count the tracks.
         auto &trackList = TrackList::Get( *p );
         auto numTracks = trackList.Leaders< const WaveTrack >().size();

         auto recordingChannels = std::max(1, AudioIORecordChannels.Read());

         gPrefs->Read(wxT("/GUI/TrackNames/RecordingNameCustom"), &recordingNameCustom, false);
         gPrefs->Read(wxT("/GUI/TrackNames/TrackNumber"), &useTrackNumber, false);
         gPrefs->Read(wxT("/GUI/TrackNames/DateStamp"), &useDateStamp, false);
         gPrefs->Read(wxT("/GUI/TrackNames/TimeStamp"), &useTimeStamp, false);
         defaultTrackName = WaveTrack::GetDefaultAudioTrackNamePreference();
         gPrefs->Read(wxT("/GUI/TrackNames/RecodingTrackName"), &defaultRecordingTrackName, defaultTrackName);

         wxString baseTrackName = recordingNameCustom? defaultRecordingTrackName : defaultTrackName;

         Track *first {};
         for (int c = 0; c < recordingChannels; c++) {
            auto newTrack = WaveTrackFactory::Get( *p ).NewWaveTrack();
            if (!first)
               first = newTrack.get();

            // Quantize bounds to the rate of the new track.
            if (c == 0) {
               if (t0 < DBL_MAX)
                  t0 = newTrack->LongSamplesToTime(newTrack->TimeToLongSamples(t0));
               if (t1 < DBL_MAX)
                  t1 = newTrack->LongSamplesToTime(newTrack->TimeToLongSamples(t1));
            }

            newTrack->SetOffset(t0);
            wxString nameSuffix = wxString(wxT(""));

            if (useTrackNumber) {
               nameSuffix += wxString::Format(wxT("%d"), 1 + (int) numTracks + c);
            }

            if (useDateStamp) {
               if (!nameSuffix.empty()) {
                  nameSuffix += wxT("_");
               }
               nameSuffix += wxDateTime::Now().FormatISODate();
            }

            if (useTimeStamp) {
               if (!nameSuffix.empty()) {
                  nameSuffix += wxT("_");
               }
               nameSuffix += wxDateTime::Now().FormatISOTime();
            }

            // ISO standard would be nice, but ":" is unsafe for file name.
            nameSuffix.Replace(wxT(":"), wxT("-"));

            if (baseTrackName.empty()) {
               newTrack->SetName(nameSuffix);
            }
            else if (nameSuffix.empty()) {
               newTrack->SetName(baseTrackName);
            }
            else {
               newTrack->SetName(baseTrackName + wxT("_") + nameSuffix);
            }

            TrackList::Get( *p ).RegisterPendingNewTrack( newTrack );

            if ((recordingChannels > 2) &&
                !(ProjectSettings::Get(*p).GetTracksFitVerticallyZoomed())) {
               TrackView::Get( *newTrack ).SetMinimized(true);
            }

            transportTracks.captureTracks.push_back(newTrack);
         }
         TrackList::Get( *p ).GroupChannels(*first, recordingChannels);
         // Bug 1548.  First of new tracks needs the focus.
         TrackFocus::Get(*p).Set(first);
         if (TrackList::Get(*p).back())
            TrackList::Get(*p).back()->EnsureVisible();
      }

      //Automated Input Level Adjustment Initialization
      #ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
         gAudioIO->AILAInitialize();
      #endif

      int token = gAudioIO->StartStream(transportTracks, t0, t1, options);

      success = (token != 0);

      if (success) {
         ProjectAudioIO::Get( *p ).SetAudioIOToken(token);
      }
      else {
         TrackList::Get( project ).ClearPendingTracks();

         // Show error message if stream could not be opened
         auto msg = XO("Error opening recording device.\nError code: %s")
            .Format( gAudioIO->LastPaErrorString() );
         using namespace BasicUI;
         ShowErrorDialog( *ProjectFramePlacement(&project),
            XO("Error"), msg, wxT("Error_opening_sound_device"),
            ErrorDialogOptions{ ErrorDialogType::ModalErrorReport } );
      }
   }

   return success;
}

/*! @excsafety{Strong} -- For state of current project's tracks */
void OnRecord(AudacityProject &project, bool altAppearance)
{
   bool bPreferNewTrack;
   gPrefs->Read("/GUI/PreferNewTrackRecord", &bPreferNewTrack, false);
   const bool appendRecord = (altAppearance == bPreferNewTrack);

   // Code from CommandHandler start...
   AudacityProject *p = &project;

   if (p) {
      const auto &selectedRegion = ViewInfo::Get( *p ).selectedRegion;
      double t0 = selectedRegion.t0();
      double t1 = selectedRegion.t1();
      // When no time selection, recording duration is 'unlimited'.
      if (t1 == t0)
         t1 = DBL_MAX;

      auto options = DefaultPlayOptions(*p);
      WaveTrackArray existingTracks;

      // Checking the selected tracks: counting them and
      // making sure they all have the same rate
      const auto selectedTracks{ GetPropertiesOfSelected(*p) };
      const int rateOfSelected{ selectedTracks.rateOfSelected };
      const int numberOfSelected{ selectedTracks.numberOfSelected };
      const bool allSameRate{ selectedTracks.allSameRate };

      if (!allSameRate) {
         AudacityMessageBox(XO("The tracks selected "
            "for recording must all have the same sampling rate"),
            XO("Mismatched Sampling Rates"),
            wxICON_ERROR | wxCENTRE);

         return;
      }

      if (appendRecord) {
         const auto trackRange = TrackList::Get( *p ).Any< const WaveTrack >();

         // Try to find wave tracks to record into.  (If any are selected,
         // try to choose only from them; else if wave tracks exist, may record into any.)
         existingTracks = ChooseExistingRecordingTracks(*p, true, rateOfSelected);
         if (!existingTracks.empty()) {
            t0 = std::max(t0,
               (trackRange + &Track::IsSelected).max(&Track::GetEndTime));
         }
         else {
            if (numberOfSelected > 0 && rateOfSelected != options.rate) {
               AudacityMessageBox(XO(
                  "Too few tracks are selected for recording at this sample rate.\n"
                  "(Audacity requires two channels at the same sample rate for\n"
                  "each stereo track)"),
                  XO("Too Few Compatible Tracks Selected"),
                  wxICON_ERROR | wxCENTRE);

               return;
            }

            existingTracks = ChooseExistingRecordingTracks(*p, false, options.rate);
            if(!existingTracks.empty())
                t0 = std::max( t0, trackRange.max( &Track::GetEndTime ) );
            // If suitable tracks still not found, will record into NEW ones,
            // starting with t0
         }
         
         // Whether we decided on NEW tracks or not:
         if (t1 <= selectedRegion.t0() && selectedRegion.t1() > selectedRegion.t0()) {
            t1 = selectedRegion.t1();   // record within the selection
         }
         else {
            t1 = DBL_MAX;        // record for a long, long time
         }
      }

      TransportTracks transportTracks;
      if (UseDuplex()) {
         // Remove recording tracks from the list of tracks for duplex ("overdub")
         // playback.
         /* TODO: set up stereo tracks if that is how the user has set up
          * their preferences, and choose sample format based on prefs */
         transportTracks = ProjectAudioManager::GetAllPlaybackTracks(TrackList::Get( *p ), false, true);
         for (const auto &wt : existingTracks) {
            auto end = transportTracks.playbackTracks.end();
            auto it = std::find(transportTracks.playbackTracks.begin(), end, wt);
            if (it != end)
               transportTracks.playbackTracks.erase(it);
         }
      }

      transportTracks.captureTracks = existingTracks;

      if (rateOfSelected != RATE_NOT_SELECTED)
         options.rate = rateOfSelected;

      DoRecord(*p, transportTracks, t0, t1, altAppearance, options);
   }
}

// GetSelectedProperties collects information about
// currently selected audio tracks
PropertiesOfSelected
GetPropertiesOfSelected(const AudacityProject &proj)
{
   double rateOfSelection{ RATE_NOT_SELECTED };

   PropertiesOfSelected result;
   result.allSameRate = true;

   const auto selectedTracks{
      TrackList::Get(proj).Selected< const WaveTrack >() };

   for (const auto & track : selectedTracks)
   {
      if (rateOfSelection != RATE_NOT_SELECTED &&
         track->GetRate() != rateOfSelection)
         result.allSameRate = false;
      else if (rateOfSelection == RATE_NOT_SELECTED)
         rateOfSelection = track->GetRate();
   }

   result.numberOfSelected = selectedTracks.size();
   result.rateOfSelected = rateOfSelection;

   return  result;
}

DoubleSetting RecordPreRollDuration{ L"/AudioIO/PreRoll", 5.0 };
DoubleSetting RecordCrossfadeDuration{ L"/AudioIO/Crossfade", 10.0 };

}
