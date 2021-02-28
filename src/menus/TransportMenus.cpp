

#include "../AudioIO.h"
#include "../CommonCommandFlags.h"
#include "../DeviceManager.h"
#include "Menus.h"
#include "Prefs.h"
#include "Project.h"
#include "../ProjectAudioIO.h"
#include "../ProjectAudioManager.h"
#include "../ProjectFileIO.h"
#include "../ProjectHistory.h"
#include "../ProjectManager.h"
#include "ProjectWindows.h"
#include "../ProjectSettings.h"
#include "../ProjectWindow.h"
#include "../SoundActivatedRecord.h"
#include "../TrackPanelAx.h"
#include "../TransportUtilities.h"
#include "../UndoManager.h"
#include "../WaveClip.h"
#include "../WaveTrack.h"
#include "ViewInfo.h"
#include "CommandContext.h"
#include "../ProjectCommandManager.h"
#include "../widgets/AudacityMessageBox.h"
#include "BasicUI.h"
#include "../widgets/ProgressDialog.h"

#include <float.h>
#include <wx/app.h>

// private helper classes and functions
namespace {

// TODO: Should all these functions which involve
// the toolbar actually move into ControlToolBar?

/// MakeReadyToPlay stops whatever is currently playing
/// and pops the play button up.  Then, if nothing is now
/// playing, it pushes the play button down and enables
/// the stop button.
bool MakeReadyToPlay(AudacityProject &project)
{
   wxCommandEvent evt;

   // If this project is playing, stop playing
   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO->IsStreamActive(
      ProjectAudioIO::Get( project ).GetAudioIOToken()
   )) {
      ProjectAudioManager::Get( project ).Stop();
      ::wxMilliSleep(100);
   }

   // If it didn't stop playing quickly, or if some other
   // project is playing, return
   if (gAudioIO->IsBusy())
      return false;

   return true;
}

}

// Menu handler functions

namespace TransportActions {

struct Handler : CommandHandlerObject {

// This Plays OR Stops audio.  It's a toggle.
// It is usually bound to the SPACE key.
void OnPlayStop(const CommandContext &context)
{
   if (TransportUtilities::DoStopPlaying(context))
      return;
   TransportUtilities::DoStartPlaying(context);
}

void OnPlayStopSelect(const CommandContext &context)
{
   ProjectAudioManager::Get( context.project ).DoPlayStopSelect();
}

void OnPlayLooped(const CommandContext &context)
{
   auto &project = context.project;

   if( !MakeReadyToPlay(project) )
      return;

   // Now play in a loop
   // Will automatically set mLastPlayMode
   TransportUtilities::PlayCurrentRegionAndWait(context, true);
}

void OnPause(const CommandContext &context)
{
   ProjectAudioManager::Get( context.project ).OnPause();
}

void OnRescanDevices(const CommandContext &WXUNUSED(context) )
{
   DeviceManager::Instance()->Rescan();
}

void OnSoundActivated(const CommandContext &context)
{
   AudacityProject &project = context.project;

   SoundActivatedRecordDialog dialog( &GetProjectFrame( project ) /* parent */ );
   dialog.ShowModal();
}

void OnToggleSoundActivated(const CommandContext & )
{
   bool pause;
   gPrefs->Read(wxT("/AudioIO/SoundActivatedRecord"), &pause, false);
   gPrefs->Write(wxT("/AudioIO/SoundActivatedRecord"), !pause);
   gPrefs->Flush();
   ProjectCommandManager::UpdateCheckmarksInAllProjects();
}

void OnTogglePlayRecording(const CommandContext & )
{
   bool Duplex;
#ifdef EXPERIMENTAL_DA
   gPrefs->Read(wxT("/AudioIO/Duplex"), &Duplex, false);
#else
   gPrefs->Read(wxT("/AudioIO/Duplex"), &Duplex, true);
#endif
   gPrefs->Write(wxT("/AudioIO/Duplex"), !Duplex);
   gPrefs->Flush();
   ProjectCommandManager::UpdateCheckmarksInAllProjects();
}

void OnToggleSWPlaythrough(const CommandContext & )
{
   bool SWPlaythrough;
   gPrefs->Read(wxT("/AudioIO/SWPlaythrough"), &SWPlaythrough, false);
   gPrefs->Write(wxT("/AudioIO/SWPlaythrough"), !SWPlaythrough);
   gPrefs->Flush();
   ProjectCommandManager::UpdateCheckmarksInAllProjects();
}

#ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
void OnToggleAutomatedInputLevelAdjustment(
   const CommandContext &WXUNUSED(context) )
{
   bool AVEnabled;
   gPrefs->Read(
      wxT("/AudioIO/AutomatedInputLevelAdjustment"), &AVEnabled, false);
   gPrefs->Write(wxT("/AudioIO/AutomatedInputLevelAdjustment"), !AVEnabled);
   gPrefs->Flush();
   ProjectCommandManager::UpdateCheckmarksInAllProjects();
}
#endif

void OnStop(const CommandContext &context)
{
   ProjectAudioManager::Get( context.project ).Stop();
}

double GetMostRecentXPos( const CellularPanel &panel, const ViewInfo &viewInfo )
{
   return viewInfo.PositionToTime(
      panel.MostRecentXCoord(), viewInfo.GetLabelWidth());
}

void OnPlayOneSecond(const CommandContext &context)
{
   auto &project = context.project;
   if( !MakeReadyToPlay(project) )
      return;

   auto &viewInfo = ViewInfo::Get( project );
   auto &trackPanel = GetProjectPanel( project );
   auto options = DefaultPlayOptions( project );

   double pos = GetMostRecentXPos( trackPanel, viewInfo );
   TransportUtilities::PlayPlayRegionAndWait(
      context, SelectedRegion(pos - 0.5, pos + 0.5),
      options, PlayMode::oneSecondPlay);
}

/// The idea for this function (and first implementation)
/// was from Juhana Sadeharju.  The function plays the
/// sound between the current mouse position and the
/// nearest selection boundary.  This gives four possible
/// play regions depending on where the current mouse
/// position is relative to the left and right boundaries
/// of the selection region.
void OnPlayToSelection(const CommandContext &context)
{
   auto &project = context.project;

   if( !MakeReadyToPlay(project) )
      return;

   auto &trackPanel = GetProjectPanel( project );
   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double pos = GetMostRecentXPos( trackPanel, viewInfo );

   double t0,t1;
   // check region between pointer and the nearest selection edge
   if (fabs(pos - selectedRegion.t0()) <
       fabs(pos - selectedRegion.t1())) {
      t0 = t1 = selectedRegion.t0();
   } else {
      t0 = t1 = selectedRegion.t1();
   }
   if( pos < t1)
      t0=pos;
   else
      t1=pos;

   // JKC: oneSecondPlay mode disables auto scrolling
   // On balance I think we should always do this in this function
   // since you are typically interested in the sound EXACTLY
   // where the cursor is.
   // TODO: have 'playing attributes' such as 'with_autoscroll'
   // rather than modes, since that's how we're now using the modes.

   // An alternative, commented out below, is to disable autoscroll
   // only when playing a short region, less than or equal to a second.
//   mLastPlayMode = ((t1-t0) > 1.0) ? normalPlay : oneSecondPlay;

   auto playOptions = DefaultPlayOptions( project );

   TransportUtilities::PlayPlayRegionAndWait(
      context, SelectedRegion(t0, t1),
      playOptions, PlayMode::oneSecondPlay);
}

// The next 4 functions provide a limited version of the
// functionality of OnPlayToSelection() for keyboard users

void OnPlayBeforeSelectionStart(const CommandContext &context)
{
   auto &project = context.project;

   if( !MakeReadyToPlay(project) )
      return;

   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double t0 = selectedRegion.t0();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);

   auto playOptions = DefaultPlayOptions( project );

   TransportUtilities::PlayPlayRegionAndWait(
      context, SelectedRegion(t0 - beforeLen, t0),
      playOptions, PlayMode::oneSecondPlay);
}

void OnPlayAfterSelectionStart(const CommandContext &context)
{
   auto &project = context.project;

   if( !MakeReadyToPlay(project) )
      return;

   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   auto playOptions = DefaultPlayOptions( project );

   if ( t1 - t0 > 0.0 && t1 - t0 < afterLen )
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t0, t1),
         playOptions, PlayMode::oneSecondPlay);
   else
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t0, t0 + afterLen),
         playOptions, PlayMode::oneSecondPlay);
}

void OnPlayBeforeSelectionEnd(const CommandContext &context)
{
   auto &project = context.project;

   if( !MakeReadyToPlay(project) )
      return;

   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);

   auto playOptions = DefaultPlayOptions( project );

   if ( t1 - t0 > 0.0 && t1 - t0 < beforeLen )
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t0, t1),
         playOptions, PlayMode::oneSecondPlay);
   else
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t1 - beforeLen, t1),
         playOptions, PlayMode::oneSecondPlay);
}

void OnPlayAfterSelectionEnd(const CommandContext &context)
{
   auto &project = context.project;

   if( !MakeReadyToPlay(project) )
      return;

   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double t1 = selectedRegion.t1();
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   auto playOptions = DefaultPlayOptions( project );

   TransportUtilities::PlayPlayRegionAndWait(
      context, SelectedRegion(t1, t1 + afterLen),
      playOptions, PlayMode::oneSecondPlay);
}

void OnPlayBeforeAndAfterSelectionStart
(const CommandContext &context)
{
   auto &project = context.project;

   if (!MakeReadyToPlay(project))
      return;

   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   auto playOptions = DefaultPlayOptions( project );

   if ( t1 - t0 > 0.0 && t1 - t0 < afterLen )
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t0 - beforeLen, t1),
         playOptions, PlayMode::oneSecondPlay);
   else
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t0 - beforeLen, t0 + afterLen),
         playOptions, PlayMode::oneSecondPlay);
}

void OnPlayBeforeAndAfterSelectionEnd
(const CommandContext &context)
{
   auto &project = context.project;

   if (!MakeReadyToPlay(project))
      return;

   auto &viewInfo = ViewInfo::Get( project );
   const auto &selectedRegion = viewInfo.selectedRegion;

   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();
   double beforeLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);
   double afterLen;
   gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);

   auto playOptions = DefaultPlayOptions( project );

   if ( t1 - t0 > 0.0 && t1 - t0 < beforeLen )
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t0, t1 + afterLen),
         playOptions, PlayMode::oneSecondPlay);
   else
      TransportUtilities::PlayPlayRegionAndWait(
         context, SelectedRegion(t1 - beforeLen, t1 + afterLen),
         playOptions, PlayMode::oneSecondPlay);
}

void OnPlayCutPreview(const CommandContext &context)
{
   auto &project = context.project;

   if ( !MakeReadyToPlay(project) )
      return;

   // Play with cut preview
   TransportUtilities::PlayCurrentRegionAndWait(context, false, true);
}

#if 0
// Legacy handlers, not used as of version 2.3.0
void OnStopSelect(const CommandContext &context)
{
   auto &project = context.project;
   auto &history = ProjectHistory::Get( project );
   auto &viewInfo = project.GetViewInfo();
   auto &selectedRegion = viewInfo.selectedRegion;

   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO->IsStreamActive()) {
      selectedRegion.setT0(gAudioIO->GetStreamTime(), false);
      ProjectAudioManager::Get( project ).Stop();
      history.ModifyState(false);           // without bWantsAutoSave
   }
}
#endif

}; // struct Handler

} // namespace

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static TransportActions::Handler instance;
   return instance;
};

// Menu definitions

#define FN(X) (& TransportActions::Handler :: X)

// Under /MenuBar
namespace {
using namespace MenuTable;
BaseItemSharedPtr TransportMenu()
{
   using Options = CommandManager::Options;

   static const auto CanStopFlags = AudioIONotBusyFlag() | CanStopAudioStreamFlag();

   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   /* i18n-hint: 'Transport' is the name given to the set of controls that
      play, record, pause etc. */
   Menu( wxT("Transport"), XXO("Tra&nsport"),
      Section( "Basic",
         Menu( wxT("Play"), XXO("Pl&aying"),
            /* i18n-hint: (verb) Start or Stop audio playback*/
            Command( wxT("PlayStop"), XXO("Pl&ay/Stop"), FN(OnPlayStop),
               CanStopAudioStreamFlag(), wxT("Space") ),
            Command( wxT("PlayStopSelect"), XXO("Play/Stop and &Set Cursor"),
               FN(OnPlayStopSelect), CanStopAudioStreamFlag(), wxT("X") ),
            Command( wxT("PlayLooped"), XXO("&Loop Play"), FN(OnPlayLooped),
               CanStopAudioStreamFlag(), wxT("Shift+Space") ),
            Command( wxT("Pause"), XXO("&Pause"), FN(OnPause),
               CanStopAudioStreamFlag(), wxT("P") )
         )
      ),

      Section( "Other",
         Command( wxT("RescanDevices"), XXO("R&escan Audio Devices"),
            FN(OnRescanDevices), AudioIONotBusyFlag() | CanStopAudioStreamFlag() ),

         Menu( wxT("Options"), XXO("Transport &Options"),
            Section( "Part1",
               // Sound Activated recording options
               Command( wxT("SoundActivationLevel"),
                  XXO("Sound Activation Le&vel..."), FN(OnSoundActivated),
                  AudioIONotBusyFlag() | CanStopAudioStreamFlag() ),
               Command( wxT("SoundActivation"),
                  XXO("Sound A&ctivated Recording (on/off)"),
                  FN(OnToggleSoundActivated),
                  AudioIONotBusyFlag() | CanStopAudioStreamFlag(),
                  Options{}.CheckTest(wxT("/AudioIO/SoundActivatedRecord"), false) )
            ),

            Section( "Part2",
               Command( wxT("Overdub"), XXO("&Overdub (on/off)"),
                  FN(OnTogglePlayRecording),
                  AudioIONotBusyFlag() | CanStopAudioStreamFlag(),
                  Options{}.CheckTest( wxT("/AudioIO/Duplex"),
#ifdef EXPERIMENTAL_DA
                     false
#else
                     true
#endif
                  ) ),
               Command( wxT("SWPlaythrough"), XXO("So&ftware Playthrough (on/off)"),
                  FN(OnToggleSWPlaythrough),
                  AudioIONotBusyFlag() | CanStopAudioStreamFlag(),
                  Options{}.CheckTest( wxT("/AudioIO/SWPlaythrough"), false ) )


      #ifdef EXPERIMENTAL_AUTOMATED_INPUT_LEVEL_ADJUSTMENT
               ,
               Command( wxT("AutomatedInputLevelAdjustmentOnOff"),
                  XXO("A&utomated Recording Level Adjustment (on/off)"),
                  FN(OnToggleAutomatedInputLevelAdjustment),
                  AudioIONotBusyFlag() | CanStopAudioStreamFlag(),
                  Options{}.CheckTest(
                     wxT("/AudioIO/AutomatedInputLevelAdjustment"), false ) )
      #endif
            )
         )
      )
   ) ) };
   return menu;
}

AttachedItem sAttachment1{
   wxT(""),
   Shared( TransportMenu() )
};

BaseItemSharedPtr ExtraTransportMenu()
{
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( wxT("Transport"), XXO("T&ransport"),
      // PlayStop is already in the menus.
      /* i18n-hint: (verb) Start playing audio*/
      Command( wxT("Play"), XXO("Pl&ay"), FN(OnPlayStop),
         WaveTracksExistFlag() | AudioIONotBusyFlag() ),
      /* i18n-hint: (verb) Stop playing audio*/
      Command( wxT("Stop"), XXO("Sto&p"), FN(OnStop),
         AudioIOBusyFlag() | CanStopAudioStreamFlag() ),
      Command( wxT("PlayOneSec"), XXO("Play &One Second"), FN(OnPlayOneSecond),
         CaptureNotBusyFlag(), wxT("1") ),
      Command( wxT("PlayToSelection"), XXO("Play to &Selection"),
         FN(OnPlayToSelection),
         CaptureNotBusyFlag(), wxT("B") ),
      Command( wxT("PlayBeforeSelectionStart"),
         XXO("Play &Before Selection Start"), FN(OnPlayBeforeSelectionStart),
         CaptureNotBusyFlag(), wxT("Shift+F5") ),
      Command( wxT("PlayAfterSelectionStart"),
         XXO("Play Af&ter Selection Start"), FN(OnPlayAfterSelectionStart),
         CaptureNotBusyFlag(), wxT("Shift+F6") ),
      Command( wxT("PlayBeforeSelectionEnd"),
         XXO("Play Be&fore Selection End"), FN(OnPlayBeforeSelectionEnd),
         CaptureNotBusyFlag(), wxT("Shift+F7") ),
      Command( wxT("PlayAfterSelectionEnd"),
         XXO("Play Aft&er Selection End"), FN(OnPlayAfterSelectionEnd),
         CaptureNotBusyFlag(), wxT("Shift+F8") ),
      Command( wxT("PlayBeforeAndAfterSelectionStart"),
         XXO("Play Before a&nd After Selection Start"),
         FN(OnPlayBeforeAndAfterSelectionStart), CaptureNotBusyFlag(),
         wxT("Ctrl+Shift+F5") ),
      Command( wxT("PlayBeforeAndAfterSelectionEnd"),
         XXO("Play Before an&d After Selection End"),
         FN(OnPlayBeforeAndAfterSelectionEnd), CaptureNotBusyFlag(),
         wxT("Ctrl+Shift+F7") ),
      Command( wxT("PlayCutPreview"), XXO("Play C&ut Preview"),
         FN(OnPlayCutPreview),
         CaptureNotBusyFlag(), wxT("C") )
   ) ) };
   return menu;
}

AttachedItem sAttachment2{
   wxT("Optional/Extra/Part1"),
   Shared( ExtraTransportMenu() )
};

BaseItemSharedPtr ExtraPlayAtSpeedMenu()
{
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( wxT("PlayAtSpeed"), XXO("&Play-at-Speed")
   ) ) };
   return menu;
}

AttachedItem sAttachment3{
   wxT("Optional/Extra/Part1"),
   Shared( ExtraPlayAtSpeedMenu() )
};

}

#undef FN
