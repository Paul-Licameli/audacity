/**********************************************************************

Audacity: A Digital Audio Editor

ProjectAudioManager.cpp

Paul Licameli split from ProjectManager.cpp

**********************************************************************/

#include "ProjectAudioManager.h"

#include <wx/app.h>
#include <wx/frame.h>

#include "AudioIO.h"
#include "BasicUI.h"
#include "CommonCommandFlags.h"
#include "Menus.h"
#include "Mix.h"
#include "ProjectAudioIO.h"
#include "ProjectFileIO.h"
#include "ProjectHistory.h"
#include "ProjectRate.h"
#include "ProjectSettings.h"
#include "ProjectStatus.h"
#include "ProjectWindows.h"
#include "UndoManager.h"
#include "ViewInfo.h"
#include "WaveTrack.h"
#include "tracks/ui/Scrubbing.h"

wxDEFINE_EVENT(EVT_RECORDING_DROPOUT, RecordingDropoutEvent);

static AudacityProject::AttachedObjects::RegisteredFactory
sProjectAudioManagerKey {
   []( AudacityProject &project ) {
      return std::make_shared< ProjectAudioManager >( project );
   }
};

ProjectAudioManager &ProjectAudioManager::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< ProjectAudioManager >(
      sProjectAudioManagerKey );
}

const ProjectAudioManager &ProjectAudioManager::Get(
   const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

ProjectAudioManager::ProjectAudioManager( AudacityProject &project )
   : mProject{ project }
{
   static ProjectStatus::RegisteredStatusWidthFunction
      registerStatusWidthFunction{ StatusWidthFunction };
   project.Bind( EVT_CHECKPOINT_FAILURE,
      &ProjectAudioManager::OnCheckpointFailure, this );
}

ProjectAudioManager::~ProjectAudioManager() = default;

static TranslatableString FormatRate( int rate )
{
   if (rate > 0) {
      return XO("Actual Rate: %d").Format( rate );
   }
   else
      // clear the status field
      return {};
}

auto ProjectAudioManager::StatusWidthFunction(
   const AudacityProject &project, StatusBarField field )
   -> ProjectStatus::StatusWidthResult
{
   if ( field == rateStatusBarField ) {
      auto &audioManager = ProjectAudioManager::Get( project );
      int rate = audioManager.mDisplayedRate;
      return {
         { { FormatRate( rate ) } },
         50
      };
   }
   return {};
}

/*! @excsafety{Strong} -- For state of mCutPreviewTracks */
int ProjectAudioManager::PlayPlayRegion(const SelectedRegion &selectedRegion,
                                   const AudioIOStartStreamOptions &options,
                                   PlayMode mode,
                                   bool backwards, /* = false */
                                   bool playWhiteSpace /* = false */)
{
   auto &projectAudioManager = *this;
   bool canStop = projectAudioManager.CanStopAudioStream();

   if ( !canStop )
      return -1;

   bool nonWaveToo = options.playNonWaveTracks;

   // Uncomment this for laughs!
   // backwards = true;

   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();
   // SelectedRegion guarantees t0 <= t1, so we need another boolean argument
   // to indicate backwards play.
   const bool looped = options.playLooped;

   if (backwards)
      std::swap(t0, t1);

   projectAudioManager.SetLooping( mode == PlayMode::loopedPlay );
   projectAudioManager.SetCutting( mode == PlayMode::cutPreviewPlay );

   bool success = false;

   auto gAudioIO = AudioIO::Get();
   if (gAudioIO->IsBusy())
      return -1;

   const bool cutpreview = mode == PlayMode::cutPreviewPlay;
   if (cutpreview && t0==t1)
      return -1; /* msmeyer: makes no sense */

   AudacityProject *p = &mProject;

   auto &tracks = TrackList::Get( *p );

   mLastPlayMode = mode;

   bool hasaudio;
   if (nonWaveToo)
      hasaudio = ! tracks.Any<PlayableTrack>().empty();
   else
      hasaudio = ! tracks.Any<WaveTrack>().empty();

   double latestEnd = (playWhiteSpace)? t1 : tracks.GetEndTime();

   if (!hasaudio)
      return -1;  // No need to continue without audio tracks

#if defined(EXPERIMENTAL_SEEK_BEHIND_CURSOR)
   double init_seek = 0.0;
#endif

   double loop_offset = 0.0;

   if (t1 == t0) {
      if (looped) {
         const auto &selectedRegion = ViewInfo::Get( *p ).selectedRegion;
         // play selection if there is one, otherwise
         // set start of play region to project start,
         // and loop the project from current play position.

         if ((t0 > selectedRegion.t0()) && (t0 < selectedRegion.t1())) {
            t0 = selectedRegion.t0();
            t1 = selectedRegion.t1();
         }
         else {
            // loop the entire project
            // Bug2347, loop playback from cursor position instead of project start
            loop_offset = t0 - tracks.GetStartTime();
            t0 = tracks.GetStartTime();
            t1 = tracks.GetEndTime();
         }
      } else {
         // move t0 to valid range
         if (t0 < 0) {
            t0 = tracks.GetStartTime();
         }
         else if (t0 > tracks.GetEndTime()) {
            t0 = tracks.GetEndTime();
         }
#if defined(EXPERIMENTAL_SEEK_BEHIND_CURSOR)
         else {
            init_seek = t0;         //AC: init_seek is where playback will 'start'
            t0 = tracks.GetStartTime();
         }
#endif
      }
      t1 = tracks.GetEndTime();
   }
   else {
      // maybe t1 < t0, with backwards scrubbing for instance
      if (backwards)
         std::swap(t0, t1);

      t0 = std::max(0.0, std::min(t0, latestEnd));
      t1 = std::max(0.0, std::min(t1, latestEnd));

      if (backwards)
         std::swap(t0, t1);
   }

   int token = -1;

   if (t1 != t0) {
      if (cutpreview) {
         const double tless = std::min(t0, t1);
         const double tgreater = std::max(t0, t1);
         double beforeLen, afterLen;
         gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);
         gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);
         double tcp0 = tless-beforeLen;
         double diff = tgreater - tless;
         double tcp1 = (tgreater+afterLen) - diff;
         SetupCutPreviewTracks(tcp0, tless, tgreater, tcp1);
         if (backwards)
            std::swap(tcp0, tcp1);
         if (mCutPreviewTracks)
         {
            AudioIOStartStreamOptions myOptions = options;
            myOptions.cutPreviewGapStart = t0;
            myOptions.cutPreviewGapLen = t1 - t0;
            token = gAudioIO->StartStream(
               GetAllPlaybackTracks(*mCutPreviewTracks, false, nonWaveToo),
               tcp0, tcp1, myOptions);
         }
         else
            // Cannot create cut preview tracks, clean up and exit
            return -1;
      }
      else {
         token = gAudioIO->StartStream(
            GetAllPlaybackTracks( tracks, false, nonWaveToo ),
            t0, t1, options);
      }
      if (token != 0) {
         success = true;
         ProjectAudioIO::Get(*p).SetAudioIOToken(token);
         if (loop_offset != 0.0) {
            // Bug 2347
            gAudioIO->SeekStream(loop_offset);
         }
#if defined(EXPERIMENTAL_SEEK_BEHIND_CURSOR )
         //AC: If init_seek was set, now's the time to make it happen.
         gAudioIO->SeekStream(init_seek);
#endif
      }
      else {
         // Bug1627 (part of it):
         // infinite error spew when trying to start scrub:
         // Problem was that the error dialog yields to events,
         // causing recursion to this function in the scrub timer
         // handler!  Easy fix, just delay the user alert instead.
         auto &window = GetProjectFrame( mProject );
         window.CallAfter( [&]{
            using namespace BasicUI;
            // Show error message if stream could not be opened
            ShowErrorDialog( *ProjectFramePlacement(&mProject),
               XO("Error"),
               XO("Error opening sound device.\nTry changing the audio host, playback device and the project sample rate."),
               wxT("Error_opening_sound_device"),
               ErrorDialogOptions{ ErrorDialogType::ModalErrorReport } );
         });
      }
   }

   if (!success)
      return -1;

   return token;
}

void ProjectAudioManager::PlayCurrentRegion(bool looped /* = false */,
                                       bool cutpreview /* = false */)
{
   auto &projectAudioManager = *this;
   bool canStop = projectAudioManager.CanStopAudioStream();

   if ( !canStop )
      return;

   AudacityProject *p = &mProject;

   {

      const auto &playRegion = ViewInfo::Get( *p ).playRegion;

      auto options = DefaultPlayOptions( *p );
      options.playLooped = looped;
      if (cutpreview)
         options.envelope = nullptr;
      auto mode =
         cutpreview ? PlayMode::cutPreviewPlay
         : options.playLooped ? PlayMode::loopedPlay
         : PlayMode::normalPlay;
      PlayPlayRegion(SelectedRegion(playRegion.GetStart(), playRegion.GetEnd()),
                     options,
                     mode);
   }
}

void ProjectAudioManager::Stop(bool stopStream /* = true*/)
{
   AudacityProject *project = &mProject;
   auto &projectAudioManager = *this;
   bool canStop = projectAudioManager.CanStopAudioStream();

   if ( !canStop )
      return;

   if(project) {
      // Let scrubbing code do some appearance change
      auto &scrubber = Scrubber::Get( *project );
      scrubber.StopScrubbing();
   }

   auto gAudioIO = AudioIO::Get();

   auto cleanup = finally( [&]{
      projectAudioManager.SetStopping( false );
   } );

   if (stopStream && gAudioIO->IsBusy()) {
      // flag that we are stopping
      projectAudioManager.SetStopping( true );
      // Allow UI to update for that
      while( wxTheApp->ProcessIdle() )
         ;
   }

   if(stopStream)
      gAudioIO->StopStream();

   projectAudioManager.SetLooping( false );
   projectAudioManager.SetCutting( false );

   #ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
      gAudioIO->AILADisable();
   #endif

   projectAudioManager.SetPaused( false );
   //Make sure you tell gAudioIO to unpause
   gAudioIO->SetPaused( false );

   ClearCutPreviewTracks();

   // So that we continue monitoring after playing or recording.
   // also clean the MeterQueues
   if( project ) {
      auto &projectAudioIO = ProjectAudioIO::Get( *project );
      auto meter = projectAudioIO.GetPlaybackMeter();
      if( meter ) {
         meter->Clear();
      }

      meter = projectAudioIO.GetCaptureMeter();
      if( meter ) {
         meter->Clear();
      }
   }
}

void ProjectAudioManager::Pause()
{
   auto &projectAudioManager = *this;
   bool canStop = projectAudioManager.CanStopAudioStream();

   if ( !canStop ) {
      auto gAudioIO = AudioIO::Get();
      gAudioIO->SetPaused(!gAudioIO->IsPaused());
   }
   else {
      OnPause();
   }
}

void ProjectAudioManager::OnPause()
{
   auto &projectAudioManager = *this;
   bool canStop = projectAudioManager.CanStopAudioStream();

   if ( !canStop ) {
      return;
   }

   bool paused = !projectAudioManager.Paused();
   projectAudioManager.SetPaused( paused );

   auto gAudioIO = AudioIO::Get();

#ifdef EXPERIMENTAL_SCRUBBING_SUPPORT

   auto project = &mProject;
   auto &scrubber = Scrubber::Get( *project );

   // Bug 1494 - Pausing a seek or scrub should just STOP as
   // it is confusing to be in a paused scrub state.
   bool bStopInstead = paused &&
      gAudioIO->IsScrubbing() &&
      !scrubber.IsSpeedPlaying() &&
      !scrubber.IsKeyboardScrubbing();

   if (bStopInstead) {
      Stop();
      return;
   }

   if (gAudioIO->IsScrubbing())
      scrubber.Pause(paused);
   else
#endif
   {
      gAudioIO->SetPaused(paused);
   }
}

/*! @excsafety{Strong} -- For state of mCutPreviewTracks*/
void ProjectAudioManager::SetupCutPreviewTracks(double WXUNUSED(playStart), double cutStart,
                                           double cutEnd, double  WXUNUSED(playEnd))

{
   ClearCutPreviewTracks();
   AudacityProject *p = &mProject;
   {
      auto trackRange = TrackList::Get( *p ).Selected< const PlayableTrack >();
      if( !trackRange.empty() ) {
         auto cutPreviewTracks = TrackList::Create( nullptr );
         for (const auto track1 : trackRange) {
            // Duplicate and change tracks
            // Clear has a very small chance of throwing

            auto newTrack = track1->Duplicate();
            newTrack->Clear(cutStart, cutEnd);
            cutPreviewTracks->Add( newTrack );
         }
         // use No-throw-guarantee:
         mCutPreviewTracks = cutPreviewTracks;
      }
   }
}

void ProjectAudioManager::ClearCutPreviewTracks()
{
   if (mCutPreviewTracks)
      mCutPreviewTracks->Clear();
   mCutPreviewTracks.reset();
}

void ProjectAudioManager::OnAudioIORate(int rate)
{
   auto &project = mProject;

   mDisplayedRate = rate;

   auto display = FormatRate( rate );

   ProjectStatus::Get( project ).Set( display, rateStatusBarField );
}

void ProjectAudioManager::OnAudioIOStartRecording()
{
   // Auto-save was done here before, but it is unnecessary, provided there
   // are sufficient autosaves when pushing or modifying undo states.
}

// This is called after recording has stopped and all tracks have flushed.
void ProjectAudioManager::OnAudioIOStopRecording()
{
   auto &project = mProject;
   auto &projectAudioIO = ProjectAudioIO::Get( project );
   auto &projectFileIO = ProjectFileIO::Get( project );

   // Only push state if we were capturing and not monitoring
   if (projectAudioIO.GetAudioIOToken() > 0)
   {
      auto &history = ProjectHistory::Get( project );

      if (IsTimerRecordCancelled()) {
         // discard recording
         history.RollbackState();
         // Reset timer record
         ResetTimerRecordCancelled();
      }
      else {
         // Add to history
         // We want this to have No-fail-guarantee if we get here from exception
         // handling of recording, and that means we rely on the last autosave
         // successfully committed to the database, not risking a failure
         history.PushState(XO("Recorded Audio"), XO("Record"),
            UndoPush::NOAUTOSAVE);

         // Now, we may add a label track to give information about
         // dropouts.  We allow failure of this.
         auto gAudioIO = AudioIO::Get();
         auto &intervals = gAudioIO->LostCaptureIntervals();
         if (intervals.size()) {
            RecordingDropoutEvent evt{ intervals };
            mProject.ProcessEvent(evt);
         }
      }
   }
}

void ProjectAudioManager::OnAudioIONewBlocks(
   const WritableSampleTrackArray *tracks)
{
   auto &project = mProject;
   auto &projectFileIO = ProjectFileIO::Get( project );
   projectFileIO.AutoSave(true);
}

void ProjectAudioManager::OnCommitRecording()
{
   const auto project = &mProject;
   TrackList::Get( *project ).ApplyPendingTracks();
}

void ProjectAudioManager::OnSoundActivationThreshold()
{
   auto &project = mProject;
   auto gAudioIO = AudioIO::Get();
   if ( gAudioIO && &project == gAudioIO->GetOwningProject() ) {
      wxTheApp->CallAfter( [this]{ Pause(); } );
   }
}

void ProjectAudioManager::OnCheckpointFailure(wxCommandEvent &evt)
{
   evt.Skip();
   Stop();
}

bool ProjectAudioManager::Playing() const
{
   auto gAudioIO = AudioIO::Get();
   return
      gAudioIO->IsBusy() &&
      CanStopAudioStream() &&
      // ... and not merely monitoring
      !gAudioIO->IsMonitoring() &&
      // ... and not punch-and-roll recording
      gAudioIO->GetNumCaptureChannels() == 0;
}

bool ProjectAudioManager::Recording() const
{
   auto gAudioIO = AudioIO::Get();
   return
      gAudioIO->IsBusy() &&
      CanStopAudioStream() &&
      gAudioIO->GetNumCaptureChannels() > 0;
}

bool ProjectAudioManager::CanStopAudioStream() const
{
   auto gAudioIO = AudioIO::Get();
   return (!gAudioIO->IsStreamActive() ||
           gAudioIO->IsMonitoring() ||
           gAudioIO->GetOwningProject() == &mProject );
}

const ReservedCommandFlag&
   CanStopAudioStreamFlag(){ static ReservedCommandFlag flag{
      [](const AudacityProject &project){
         auto &projectAudioManager = ProjectAudioManager::Get( project );
         bool canStop = projectAudioManager.CanStopAudioStream();
         return canStop;
      }
   }; return flag; }

AudioIOStartStreamOptions
DefaultPlayOptions( AudacityProject &project )
{
   auto &projectAudioIO = ProjectAudioIO::Get( project );
   AudioIOStartStreamOptions options { &project,
      ProjectRate::Get( project ).GetRate() };
   options.captureMeter = projectAudioIO.GetCaptureMeter();
   options.playbackMeter = projectAudioIO.GetPlaybackMeter();
   options.envelope = Mixer::WarpOptions::DefaultWarp(TrackList::Get(project));
   options.listener = ProjectAudioManager::Get( project ).shared_from_this();
   return options;
}

AudioIOStartStreamOptions
DefaultSpeedPlayOptions( AudacityProject &project )
{
   auto result = DefaultPlayOptions( project );
   auto gAudioIO = AudioIO::Get();
   auto PlayAtSpeedRate = gAudioIO->GetBestRate(
      false,     //not capturing
      true,      //is playing
      ProjectRate::Get( project ).GetRate()  //suggested rate
   );
   result.rate = PlayAtSpeedRate;
   return result;
}

TransportTracks ProjectAudioManager::GetAllPlaybackTracks(
   TrackList &trackList, bool selectedOnly, bool nonWaveToo)
{
   TransportTracks result;
   {
      auto range = trackList.Any< WaveTrack >()
         + (selectedOnly ? &Track::IsSelected : &Track::Any );
      for (auto pTrack: range)
         result.playbackTracks.push_back(
            pTrack->SharedPointer< WaveTrack >() );
   }
#ifdef EXPERIMENTAL_MIDI_OUT
   if (nonWaveToo) {
      auto range = trackList.Any< const PlayableTrack >() +
         (selectedOnly ? &Track::IsSelected : &Track::Any );
      for (auto pTrack: range)
         if (!track_cast<const WaveTrack *>(pTrack))
            result.otherPlayableTracks.push_back(
               pTrack->SharedPointer< const PlayableTrack >() );
   }
#else
   WXUNUSED(useMidi);
#endif
   return result;
}

// Stop playing or recording, if paused.
void ProjectAudioManager::StopIfPaused()
{
   if( AudioIOBase::Get()->IsPaused() )
      Stop();
}

bool ProjectAudioManager::DoPlayStopSelect( bool click, bool shift )
{
   auto &project = mProject;
   auto &scrubber = Scrubber::Get( project );
   auto token = ProjectAudioIO::Get( project ).GetAudioIOToken();
   auto &viewInfo = ViewInfo::Get( project );
   auto &selection = viewInfo.selectedRegion;
   auto gAudioIO = AudioIO::Get();

   //If busy, stop playing, make sure everything is unpaused.
   if (scrubber.HasMark() ||
       gAudioIO->IsStreamActive(token)) {
      // change the selection
      auto time = gAudioIO->GetStreamTime();
      // Test WasSpeedPlaying(), not IsSpeedPlaying()
      // as we could be stopped now. Similarly WasKeyboardScrubbing().
      if (click && (scrubber.WasSpeedPlaying() || scrubber.WasKeyboardScrubbing()))
      {
         ;// don't change the selection.
      }
      else if (shift && click) {
         // Change the region selection, as if by shift-click at the play head
         auto t0 = selection.t0(), t1 = selection.t1();
         if (time < t0)
            // Grow selection
            t0 = time;
         else if (time > t1)
            // Grow selection
            t1 = time;
         else {
            // Shrink selection, changing the nearer boundary
            if (fabs(t0 - time) < fabs(t1 - time))
               t0 = time;
            else
               t1 = time;
         }
         selection.setTimes(t0, t1);
      }
      else if (click){
         // avoid a point at negative time.
         time = wxMax( time, 0 );
         // Set a point selection, as if by a click at the play head
         selection.setTimes(time, time);
      } else
         // How stop and set cursor always worked
         // -- change t0, collapsing to point only if t1 was greater
         selection.setT0(time, false);

      ProjectHistory::Get( project ).ModifyState(false);           // without bWantsAutoSave
      return true;
   }
   return false;
}

// The code for "OnPlayStopSelect" is simply the code of "OnPlayStop" and
// "OnStopSelect" merged.
void ProjectAudioManager::DoPlayStopSelect()
{
   auto gAudioIO = AudioIO::Get();
   if (DoPlayStopSelect(false, false))
      Stop();
   else if (!gAudioIO->IsBusy()) {
      //Otherwise, start playing (assuming audio I/O isn't busy)

      // Will automatically set mLastPlayMode
      PlayCurrentRegion(false);
   }
}

static RegisteredMenuItemEnabler stopIfPaused{{
   PausedFlag,
   AudioIONotBusyFlag,
   []( const AudacityProject &project ){
      return MenuManager::Get( project ).mStopIfWasPaused; },
   []( AudacityProject &project, CommandFlag ){
      if ( MenuManager::Get( project ).mStopIfWasPaused )
         ProjectAudioManager::Get( project ).StopIfPaused();
   }
}};
