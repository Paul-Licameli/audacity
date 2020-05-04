

#include "../AdornedRulerPanel.h"
#include "../AudioIO.h"
#include "../CommonCommandFlags.h"
#include "../SpectrumAnalyst.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ProjectAudioIO.h"
#include "../ProjectAudioManager.h"
#include "../ProjectHistory.h"
#include "../ProjectSelectionManager.h"
#include "../ProjectSettings.h"
#include "../ProjectWindow.h"
#include "../SelectUtilities.h"
#include "../TimeDialog.h"
#include "../TrackPanel.h"
#include "../WaveTrack.h"
#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../toolbars/ControlToolBar.h"
#include "../tracks/ui/SelectHandle.h"
#include "../tracks/playabletrack/wavetrack/ui/WaveTrackView.h"
#include "../tracks/playabletrack/wavetrack/ui/WaveTrackViewConstants.h"

// private helper classes and functions
namespace {

void DoNextPeakFrequency(AudacityProject &project, bool up)
{
   auto &tracks = TrackList::Get( project );
   auto &viewInfo = ViewInfo::Get( project );

   // Find the first selected wave track that is in a spectrogram view.
   const WaveTrack *pTrack {};
   for ( auto wt : tracks.Selected< const WaveTrack >() ) {
      const auto displays = WaveTrackView::Get( *wt ).GetDisplays();
      bool hasSpectrum = (displays.end() != std::find(
         displays.begin(), displays.end(),
         WaveTrackSubView::Type{ WaveTrackViewConstants::Spectrum, {} }
      ) );
      if ( hasSpectrum ) {
         pTrack = wt;
         break;
      }
   }

   if (pTrack) {
      SpectrumAnalyst analyst;
      SelectHandle::SnapCenterOnce(analyst, viewInfo, pTrack, up);
      ProjectHistory::Get( project ).ModifyState(false);
   }
}

double NearestZeroCrossing
(AudacityProject &project, double t0)
{
   const auto &settings = ProjectSettings::Get( project );
   auto rate = settings.GetRate();
   auto &tracks = TrackList::Get( project );

   // Window is 1/100th of a second.
   auto windowSize = size_t(std::max(1.0, rate / 100));
   Floats dist{ windowSize, true };

   int nTracks = 0;
   for (auto one : tracks.Selected< const WaveTrack >()) {
      auto oneWindowSize = size_t(std::max(1.0, one->GetRate() / 100));
      Floats oneDist{ oneWindowSize };
      auto s = one->TimeToLongSamples(t0);
      // fillTwo to ensure that missing values are treated as 2, and hence do
      // not get used as zero crossings.
      one->GetFloats(oneDist.get(),
               s - (int)oneWindowSize/2, oneWindowSize, fillTwo);


      // Looking for actual crossings.
      double prev = 2.0;
      for(size_t i=0; i<oneWindowSize; i++){
         float fDist = fabs( oneDist[i]); // score is absolute value
         if( prev * oneDist[i] > 0 ) // both same sign?  No good.
            fDist = fDist + 0.4; // No good if same sign.
         else if( prev > 0.0 )
            fDist = fDist + 0.1; // medium penalty for downward crossing.
         prev = oneDist[i];
         oneDist[i] = fDist;
      }

      // TODO: The mixed rate zero crossing code is broken,
      // if oneWindowSize > windowSize we'll miss out some
      // samples - so they will still be zero, so we'll use them.
      for(size_t i = 0; i < windowSize; i++) {
         size_t j;
         if (windowSize != oneWindowSize)
            j = i * (oneWindowSize-1) / (windowSize-1);
         else
            j = i;

         dist[i] += oneDist[j];
         // Apply a small penalty for distance from the original endpoint
         // We'll always prefer an upward  
         dist[i] +=
            0.1 * (abs(int(i) - int(windowSize/2))) / float(windowSize/2);
      }
      nTracks++;
   }

   // Find minimum
   int argmin = 0;
   float min = 3.0;
   for(size_t i=0; i<windowSize; i++) {
      if (dist[i] < min) {
         argmin = i;
         min = dist[i];
      }
   }

   // If we're worse than 0.2 on average, on one track, then no good.
   if(( nTracks == 1 ) && ( min > (0.2*nTracks) ))
      return t0;
   // If we're worse than 0.6 on average, on multi-track, then no good.
   if(( nTracks > 1 ) && ( min > (0.6*nTracks) ))
      return t0;

   return t0 + (argmin - (int)windowSize/2) / rate;
}

// If this returns true, then there was a key up, and nothing more to do,
// after this function has completed.
// (at most this function just does a ModifyState for the keyup)
bool OnlyHandleKeyUp( const CommandContext &context )
{
   auto &project = context.project;
   auto evt = context.pEvt;
   bool bKeyUp = (evt) && evt->GetEventType() == wxEVT_KEY_UP;

   if( ProjectAudioIO::Get( project ).IsAudioActive() )
      return bKeyUp;
   if( !bKeyUp )
      return false;

   ProjectHistory::Get( project ).ModifyState(false);
   return true;
}

enum CursorDirection {
   DIRECTION_LEFT = -1,
   DIRECTION_RIGHT = +1
};

enum SelectionOperation {
    SELECTION_EXTEND,
    SELECTION_CONTRACT,
    CURSOR_MOVE
};

enum TimeUnit {
    TIME_UNIT_SECONDS,
    TIME_UNIT_PIXELS
};

struct SeekInfo
{
   wxLongLong mLastSelectionAdjustment { ::wxGetUTCTimeMillis() };
   double mSeekShort{ 0.0 };
   double mSeekLong{ 0.0 };
};

void SeekWhenAudioActive(double seekStep, wxLongLong &lastSelectionAdjustment)
{
   auto gAudioIO = AudioIO::Get();
#ifdef EXPERIMENTAL_IMPROVED_SEEKING
   if (gAudioIO->GetLastPlaybackTime() < lastSelectionAdjustment) {
      // Allow time for the last seek to output a buffer before
      // discarding samples again
      // Do not advance mLastSelectionAdjustment
      return;
   }
#endif
   lastSelectionAdjustment = ::wxGetUTCTimeMillis();

   gAudioIO->SeekStream(seekStep);
}

// Handles moving a selection edge with the keyboard in snap-to-time mode;
// returns the moved value.
// Will move at least minPix pixels -- set minPix positive to move forward,
// negative to move backward.
// Helper for moving by keyboard with snap-to-grid enabled
double GridMove
(AudacityProject &project, double t, int minPix)
{
   const auto &settings = ProjectSettings::Get( project );
   auto rate = settings.GetRate();
   auto &viewInfo = ViewInfo::Get( project );
   auto format = settings.GetSelectionFormat();

   NumericConverter nc(NumericConverter::TIME, format, t, rate);

   // Try incrementing/decrementing the value; if we've moved far enough we're
   // done
   double result;
   minPix >= 0 ? nc.Increment() : nc.Decrement();
   result = nc.GetValue();
   if (std::abs(viewInfo.TimeToPosition(result) - viewInfo.TimeToPosition(t))
       >= abs(minPix))
       return result;

   // Otherwise, move minPix pixels, then snap to the time.
   result = viewInfo.OffsetTimeByPixels(t, minPix);
   nc.SetValue(result);
   result = nc.GetValue();
   return result;
}

double OffsetTime
(AudacityProject &project,
 double t, double offset, TimeUnit timeUnit, int snapToTime)
{
   auto &viewInfo = ViewInfo::Get( project );

    if (timeUnit == TIME_UNIT_SECONDS)
        return t + offset; // snapping is currently ignored for non-pixel moves

    if (snapToTime == SNAP_OFF)
        return viewInfo.OffsetTimeByPixels(t, (int)offset);

    return GridMove(project, t, (int)offset);
}

// Moving a cursor, and collapsed selection.
void MoveWhenAudioInactive
(AudacityProject &project, double seekStep, TimeUnit timeUnit)
{
   auto &viewInfo = ViewInfo::Get( project );
   auto &trackPanel = TrackPanel::Get( project );
   auto &tracks = TrackList::Get( project );
   auto &ruler = AdornedRulerPanel::Get( project );
   const auto &settings = ProjectSettings::Get( project );
   auto &window = ProjectWindow::Get( project );

   // If TIME_UNIT_SECONDS, snap-to will be off.
   int snapToTime = settings.GetSnapTo();
   const double t0 = viewInfo.selectedRegion.t0();
   const double end = std::max( 
      tracks.GetEndTime(),
      viewInfo.GetScreenEndTime());

   // Move the cursor
   // Already in cursor mode?
   if( viewInfo.selectedRegion.isPoint() )
   {
      double newT = OffsetTime(project,
         t0, seekStep, timeUnit, snapToTime);
      // constrain.
      newT = std::max(0.0, newT);
      newT = std::min(newT, end);
      // Move 
      viewInfo.selectedRegion.setT0(
         newT,
         false); // do not swap selection boundaries
      viewInfo.selectedRegion.collapseToT0();

      // Move the visual cursor, avoiding an unnecessary complete redraw
      trackPanel.DrawOverlays(false);
      ruler.DrawOverlays(false);
   }
   else
   {
      // Transition to cursor mode.
      if( seekStep < 0 )
         viewInfo.selectedRegion.collapseToT0();
      else
         viewInfo.selectedRegion.collapseToT1();
      trackPanel.Refresh(false);
   }

   // Make sure NEW position is in view
   window.ScrollIntoView(viewInfo.selectedRegion.t1());
   return;
}

void SeekWhenAudioInactive
(AudacityProject &project, double seekStep, TimeUnit timeUnit,
SelectionOperation operation)
{
   auto &viewInfo = ViewInfo::Get( project );
   auto &tracks = TrackList::Get( project );
   const auto &settings = ProjectSettings::Get( project );
   auto &window = ProjectWindow::Get( project );

   if( operation == CURSOR_MOVE )
   {
      MoveWhenAudioInactive( project, seekStep, timeUnit);
      return;
   }

   int snapToTime = settings.GetSnapTo();
   const double t0 = viewInfo.selectedRegion.t0();
   const double t1 = viewInfo.selectedRegion.t1();
   const double end = std::max( 
      tracks.GetEndTime(),
      viewInfo.GetScreenEndTime());

   // Is it t0 or t1 moving?
   bool bMoveT0 = (operation == SELECTION_CONTRACT && seekStep > 0) ||
	   (operation == SELECTION_EXTEND && seekStep < 0);
   // newT is where we want to move to
   double newT = OffsetTime( project,
      bMoveT0 ? t0 : t1, seekStep, timeUnit, snapToTime);
   // constrain to be in the track/screen limits.
   newT = std::max( 0.0, newT );
   newT = std::min( newT, end);
   // optionally constrain to be a contraction, i.e. so t0/t1 do not cross over
   if( operation == SELECTION_CONTRACT )
      newT = bMoveT0 ? std::min( t1, newT ) : std::max( t0, newT );

   // Actually move
   if( bMoveT0 )
      viewInfo.selectedRegion.setT0( newT );
   else 
      viewInfo.selectedRegion.setT1( newT );

   // Ensure it is visible
   window.ScrollIntoView(newT);
}

// Handle small cursor and play head movements
void SeekLeftOrRight
(AudacityProject &project, double direction, SelectionOperation operation,
 SeekInfo &info)
{
   // PRL:  What I found and preserved, strange though it be:
   // During playback:  jump depends on preferences and is independent of the
   // zoom and does not vary if the key is held
   // Else: jump depends on the zoom and gets bigger if the key is held

   if( ProjectAudioIO::Get( project ).IsAudioActive() )
   {
      if( operation == CURSOR_MOVE )
         SeekWhenAudioActive( info.mSeekShort * direction,
            info.mLastSelectionAdjustment );
      else if( operation == SELECTION_EXTEND )
         SeekWhenAudioActive( info.mSeekLong * direction,
            info.mLastSelectionAdjustment );
      // Note: no action for CURSOR_CONTRACT
      return;
   }

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   const wxLongLong curtime = ::wxGetUTCTimeMillis();
   enum { MIN_INTERVAL = 50 };
   const bool fast =
      (curtime - info.mLastSelectionAdjustment < MIN_INTERVAL);

   info.mLastSelectionAdjustment = curtime;

   // How much faster should the cursor move if shift is down?
   enum { LARGER_MULTIPLIER = 4 };
   const double seekStep = (fast ? LARGER_MULTIPLIER : 1.0) * direction;

   SeekWhenAudioInactive( project, seekStep, TIME_UNIT_PIXELS, operation);
}

// Move the cursor forward or backward, while paused or while playing.
void DoCursorMove(
   AudacityProject &project, double seekStep,
   wxLongLong &lastSelectionAdjustment)
{
   if (ProjectAudioIO::Get( project ).IsAudioActive()) {
      SeekWhenAudioActive(seekStep, lastSelectionAdjustment);
   }
   else
   {
      lastSelectionAdjustment = ::wxGetUTCTimeMillis();
      MoveWhenAudioInactive(project, seekStep, TIME_UNIT_SECONDS);
   }

   ProjectHistory::Get( project ).ModifyState(false);
}

void DoBoundaryMove(AudacityProject &project, int step, SeekInfo &info)
{
   auto &viewInfo = ViewInfo::Get( project );
   auto &tracks = TrackList::Get( project );
   auto &window = ProjectWindow::Get( project );

   // step is negative, then is moving left.  step positive, moving right.
   // Move the left/right selection boundary, to expand the selection

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetUTCTimeMillis();
   int pixels = step;
   if( curtime - info.mLastSelectionAdjustment < 50 )
   {
      pixels *= 4;
   }
   info.mLastSelectionAdjustment = curtime;

   // we used to have a parameter boundaryContract to say if expanding or
   // contracting.  it is no longer needed.
   bool bMoveT0 = (step < 0 );// ^ boundaryContract ;

   if( ProjectAudioIO::Get( project ).IsAudioActive() )
   {
      auto gAudioIO = AudioIO::Get();
      double indicator = gAudioIO->GetStreamTime();
      if( bMoveT0 )
         viewInfo.selectedRegion.setT0(indicator, false);
      else
         viewInfo.selectedRegion.setT1(indicator);

      ProjectHistory::Get( project ).ModifyState(false);
      return;
   }

   const double t0 = viewInfo.selectedRegion.t0();
   const double t1 = viewInfo.selectedRegion.t1();
   const double end = std::max( 
      tracks.GetEndTime(),
      viewInfo.GetScreenEndTime());

   double newT = viewInfo.OffsetTimeByPixels( bMoveT0 ? t0 : t1, pixels);
   // constrain to be in the track/screen limits.
   newT = std::max( 0.0, newT );
   newT = std::min( newT, end);
   // optionally constrain to be a contraction, i.e. so t0/t1 do not cross over
   //if( boundaryContract )
   //   newT = bMoveT0 ? std::min( t1, newT ) : std::max( t0, newT );

   // Actually move
   if( bMoveT0 )
      viewInfo.selectedRegion.setT0( newT );
   else 
      viewInfo.selectedRegion.setT1( newT );

   // Ensure it is visible
   window.ScrollIntoView(newT);

   ProjectHistory::Get( project ).ModifyState(false);
}

}

namespace SelectActions {


// Menu handler functions

struct Handler
   : CommandHandlerObject // MUST be the first base class!
   , ClientData::Base
   , PrefsListener
{

void OnSelectAll(const CommandContext &context)
{
   SelectUtilities::DoSelectAll( context.project );
}

void OnSelectNone(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   selectedRegion.collapseToT0();
   SelectUtilities::SelectNone( project );
   ProjectHistory::Get( project ).ModifyState(false);
}

void OnSelectAllTracks(const CommandContext &context)
{
   auto &project = context.project;
   SelectUtilities::DoSelectTimeAndTracks( project, false, true );
}

void OnSelectSyncLockSel(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );

   bool selected = false;
   for (auto t : tracks.Any() + &Track::SupportsBasicEditing
         + &Track::IsSyncLockSelected - &Track::IsSelected) {
      t->SetSelected(true);
      selected = true;
   }

   if (selected)
      ProjectHistory::Get( project ).ModifyState(false);
}

//this pops up a dialog which allows the left selection to be set.
//If playing/recording is happening, it sets the left selection at
//the current play position.
void OnSetLeftSelection(const CommandContext &context)
{
   auto &project = context.project;
   auto token = ProjectAudioIO::Get( project ).GetAudioIOToken();
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   const auto &settings = ProjectSettings::Get( project );
   auto &window = GetProjectFrame( project );

   bool bSelChanged = false;
   auto gAudioIO = AudioIO::Get();
   if ((token > 0) && gAudioIO->IsStreamActive(token))
   {
      double indicator = gAudioIO->GetStreamTime();
      selectedRegion.setT0(indicator, false);
      bSelChanged = true;
   }
   else
   {
      auto fmt = settings.GetSelectionFormat();
      auto rate = settings.GetRate();

      TimeDialog dlg(&window, XO("Set Left Selection Boundary"),
         fmt, rate, selectedRegion.t0(), XO("Position"));

      if (wxID_OK == dlg.ShowModal())
      {
         //Get the value from the dialog
         selectedRegion.setT0(
            std::max(0.0, dlg.GetTimeValue()), false);
         bSelChanged = true;
      }
   }

   if (bSelChanged)
   {
      ProjectHistory::Get( project ).ModifyState(false);
   }
}

void OnSetRightSelection(const CommandContext &context)
{
   auto &project = context.project;
   auto token = ProjectAudioIO::Get( project ).GetAudioIOToken();
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   const auto &settings = ProjectSettings::Get( project );
   auto &window = GetProjectFrame( project );

   bool bSelChanged = false;
   auto gAudioIO = AudioIO::Get();
   if ((token > 0) && gAudioIO->IsStreamActive(token))
   {
      double indicator = gAudioIO->GetStreamTime();
      selectedRegion.setT1(indicator, false);
      bSelChanged = true;
   }
   else
   {
      auto fmt = settings.GetSelectionFormat();
      auto rate = settings.GetRate();

      TimeDialog dlg(&window, XO("Set Right Selection Boundary"),
         fmt, rate, selectedRegion.t1(), XO("Position"));

      if (wxID_OK == dlg.ShowModal())
      {
         //Get the value from the dialog
         selectedRegion.setT1(
            std::max(0.0, dlg.GetTimeValue()), false);
         bSelChanged = true;
      }
   }

   if (bSelChanged)
   {
      ProjectHistory::Get( project ).ModifyState(false);
   }
}

void OnSelectStartCursor(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   double kWayOverToRight = std::numeric_limits<double>::max();

   auto range = tracks.Selected();
   if ( ! range )
      return;

   double minOffset = range.min( &Track::GetStartTime );

   if( minOffset >=
       (kWayOverToRight * (1 - std::numeric_limits<double>::epsilon()) ))
      return;

   selectedRegion.setT0(minOffset);

   ProjectHistory::Get( project ).ModifyState(false);
}

void OnSelectCursorEnd(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   double kWayOverToLeft = std::numeric_limits<double>::lowest();

   auto range = tracks.Selected();
   if ( ! range )
      return;

   double maxEndOffset = range.max( &Track::GetEndTime );

   if( maxEndOffset <=
       (kWayOverToLeft * (1 - std::numeric_limits<double>::epsilon()) ))
      return;

   selectedRegion.setT1(maxEndOffset);

   ProjectHistory::Get( project ).ModifyState(false);
}

void OnSelectTrackStartToEnd(const CommandContext &context)
{
   auto &project = context.project;
   auto &viewInfo = ViewInfo::Get( project );
   auto &tracks = TrackList::Get( project );

   auto range = tracks.Selected();
   double maxEndOffset = range.max( &Track::GetEndTime );
   double minOffset = range.min( &Track::GetStartTime );

   if( maxEndOffset < minOffset)
      return;

   viewInfo.selectedRegion.setTimes( minOffset, maxEndOffset );
   ProjectHistory::Get( project ).ModifyState(false);
}

// Handler state:
SelectedRegion mRegionSave{};

void OnSelectionSave(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   mRegionSave = selectedRegion;
}

void OnSelectionRestore(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get(project);

   if ((mRegionSave.t0() == 0.0) &&
       (mRegionSave.t1() == 0.0))
      return;

   selectedRegion = mRegionSave;
   window.ScrollIntoView(selectedRegion.t0());

   ProjectHistory::Get( project ).ModifyState(false);
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING

// Handler state:
double mLastF0{ SelectedRegion::UndefinedFrequency };
double mLastF1{ SelectedRegion::UndefinedFrequency };

void OnToggleSpectralSelection(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;

   const double f0 = selectedRegion.f0();
   const double f1 = selectedRegion.f1();
   const bool haveSpectralSelection =
   !(f0 == SelectedRegion::UndefinedFrequency &&
     f1 == SelectedRegion::UndefinedFrequency);
   if (haveSpectralSelection)
   {
      mLastF0 = f0;
      mLastF1 = f1;
      selectedRegion.setFrequencies
      (SelectedRegion::UndefinedFrequency, SelectedRegion::UndefinedFrequency);
   }
   else
      selectedRegion.setFrequencies(mLastF0, mLastF1);

   ProjectHistory::Get( project ).ModifyState(false);
}

void OnNextHigherPeakFrequency(const CommandContext &context)
{
   auto &project = context.project;
   DoNextPeakFrequency(project, true);
}

void OnNextLowerPeakFrequency(const CommandContext &context)
{
   auto &project = context.project;
   DoNextPeakFrequency(project, false);
}
#endif

// Handler state:
bool mCursorPositionHasBeenStored{ false };
double mCursorPositionStored{ 0.0 };

void OnSelectCursorStoredCursor(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto isAudioActive = ProjectAudioIO::Get( project ).IsAudioActive();

   if (mCursorPositionHasBeenStored) {
      auto gAudioIO = AudioIO::Get();
      double cursorPositionCurrent = isAudioActive
         ? gAudioIO->GetStreamTime()
         : selectedRegion.t0();
      selectedRegion.setTimes(
         std::min(cursorPositionCurrent, mCursorPositionStored),
         std::max(cursorPositionCurrent, mCursorPositionStored));

      ProjectHistory::Get( project ).ModifyState(false);
   }
}

void OnCursorPositionStore(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto isAudioActive = ProjectAudioIO::Get( project ).IsAudioActive();

   auto gAudioIO = AudioIO::Get();
   mCursorPositionStored =
      isAudioActive ? gAudioIO->GetStreamTime() : selectedRegion.t0();
   mCursorPositionHasBeenStored = true;
}

void OnZeroCrossing(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   const auto &settings = ProjectSettings::Get( project );

   const double t0 = NearestZeroCrossing(project, selectedRegion.t0());
   if (selectedRegion.isPoint())
      selectedRegion.setTimes(t0, t0);
   else {
      const double t1 = NearestZeroCrossing(project, selectedRegion.t1());
      // Empty selection is generally not much use, so do not make it if empty.
      if( fabs( t1 - t0 ) * settings.GetRate() > 1.5 )
         selectedRegion.setTimes(t0, t1);
   }

   ProjectHistory::Get( project ).ModifyState(false);
}

void OnSnapToOff(const CommandContext &context)
{
   auto &project = context.project;
   ProjectSelectionManager::Get( project ).AS_SetSnapTo(SNAP_OFF);
}

void OnSnapToNearest(const CommandContext &context)
{
   auto &project = context.project;
   ProjectSelectionManager::Get( project ).AS_SetSnapTo(SNAP_NEAREST);
}

void OnSnapToPrior(const CommandContext &context)
{
   auto &project = context.project;
   ProjectSelectionManager::Get( project ).AS_SetSnapTo(SNAP_PRIOR);
}

void OnSelToStart(const CommandContext &context)
{
   auto &project = context.project;
   auto &window = ProjectWindow::Get( project );
   window.Rewind(true);
   ProjectHistory::Get( project ).ModifyState(false);
}

void OnSelToEnd(const CommandContext &context)
{
   auto &project = context.project;
   auto &window = ProjectWindow::Get( project );
   window.SkipEnd(true);
   ProjectHistory::Get( project ).ModifyState(false);
}

// Handler state:
SeekInfo mSeekInfo;

void OnSelExtendLeft(const CommandContext &context)
{
   if( !OnlyHandleKeyUp( context ) )
      SeekLeftOrRight( context.project, DIRECTION_LEFT, SELECTION_EXTEND,
         mSeekInfo );
}

void OnSelExtendRight(const CommandContext &context)
{
   if( !OnlyHandleKeyUp( context ) )
      SeekLeftOrRight( context.project, DIRECTION_RIGHT, SELECTION_EXTEND,
         mSeekInfo );
}

void OnSelSetExtendLeft(const CommandContext &context)
{
   DoBoundaryMove( context.project, DIRECTION_LEFT, mSeekInfo);
}

void OnSelSetExtendRight(const CommandContext &context)
{
   DoBoundaryMove( context.project, DIRECTION_RIGHT, mSeekInfo);
}

void OnSelContractLeft(const CommandContext &context)
{
   if( !OnlyHandleKeyUp( context ) )
      SeekLeftOrRight( context.project, DIRECTION_RIGHT, SELECTION_CONTRACT,
         mSeekInfo );
}

void OnSelContractRight(const CommandContext &context)
{
   if( !OnlyHandleKeyUp( context ) )
      SeekLeftOrRight( context.project, DIRECTION_LEFT, SELECTION_CONTRACT,
         mSeekInfo );
}

void OnCursorSelStart(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   selectedRegion.collapseToT0();
   ProjectHistory::Get( project ).ModifyState(false);
   window.ScrollIntoView(selectedRegion.t0());
}

void OnCursorSelEnd(const CommandContext &context)
{
   auto &project = context.project;
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   selectedRegion.collapseToT1();
   ProjectHistory::Get( project ).ModifyState(false);
   window.ScrollIntoView(selectedRegion.t1());
}

void OnCursorTrackStart(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   double kWayOverToRight = std::numeric_limits<double>::max();

   auto trackRange = tracks.Selected() + &Track::SupportsBasicEditing;
   if (trackRange.empty())
      // This should have been prevented by command manager
      return;

   // Range is surely nonempty now
   auto minOffset = std::max( 0.0, trackRange.min( &Track::GetOffset ) );

   if( minOffset >=
       (kWayOverToRight * (1 - std::numeric_limits<double>::epsilon()) ))
      return;

   selectedRegion.setTimes(minOffset, minOffset);
   ProjectHistory::Get( project ).ModifyState(false);
   window.ScrollIntoView(selectedRegion.t0());
}

void OnCursorTrackEnd(const CommandContext &context)
{
   auto &project = context.project;
   auto &tracks = TrackList::Get( project );
   auto &selectedRegion = ViewInfo::Get( project ).selectedRegion;
   auto &window = ProjectWindow::Get( project );

   double kWayOverToLeft = std::numeric_limits<double>::lowest();

   auto trackRange = tracks.Selected() + &Track::SupportsBasicEditing;
   if (trackRange.empty())
      // This should have been prevented by command manager
      return;

   // Range is surely nonempty now
   auto maxEndOffset = trackRange.max( &Track::GetEndTime );

   if( maxEndOffset <
       (kWayOverToLeft * (1 - std::numeric_limits<double>::epsilon()) ))
      return;

   selectedRegion.setTimes(maxEndOffset, maxEndOffset);
   ProjectHistory::Get( project ).ModifyState(false);
   window.ScrollIntoView(selectedRegion.t1());
}

void OnSkipStart(const CommandContext &context)
{
   auto &project = context.project;

   auto &controlToolBar = ControlToolBar::Get( project );
   controlToolBar.OnRewind();
   ProjectHistory::Get( project ).ModifyState(false);
}

void OnSkipEnd(const CommandContext &context)
{
   auto &project = context.project;

   auto &controlToolBar = ControlToolBar::Get( project );
   controlToolBar.OnFF();
   ProjectHistory::Get( project ).ModifyState(false);
}

void OnCursorLeft(const CommandContext &context)
{
   if( !OnlyHandleKeyUp( context ) )
      SeekLeftOrRight( context.project, DIRECTION_LEFT, CURSOR_MOVE,
         mSeekInfo );
}

void OnCursorRight(const CommandContext &context)
{
   if( !OnlyHandleKeyUp( context ) )
      SeekLeftOrRight( context.project, DIRECTION_RIGHT, CURSOR_MOVE,
         mSeekInfo );
}

void OnCursorShortJumpLeft(const CommandContext &context)
{
   DoCursorMove( context.project,
      -mSeekInfo.mSeekShort, mSeekInfo.mLastSelectionAdjustment );
}

void OnCursorShortJumpRight(const CommandContext &context)
{
   DoCursorMove( context.project,
      mSeekInfo.mSeekShort, mSeekInfo.mLastSelectionAdjustment );
}

void OnCursorLongJumpLeft(const CommandContext &context)
{
   DoCursorMove( context.project,
      -mSeekInfo.mSeekLong, mSeekInfo.mLastSelectionAdjustment );
}

void OnCursorLongJumpRight(const CommandContext &context)
{
   DoCursorMove( context.project,
      mSeekInfo.mSeekLong, mSeekInfo.mLastSelectionAdjustment );
}

void OnSeekLeftShort(const CommandContext &context)
{
   auto &project = context.project;
   SeekLeftOrRight( project, DIRECTION_LEFT, CURSOR_MOVE, mSeekInfo );
}

void OnSeekRightShort(const CommandContext &context)
{
   auto &project = context.project;
   SeekLeftOrRight( project, DIRECTION_RIGHT, CURSOR_MOVE, mSeekInfo );
}

void OnSeekLeftLong(const CommandContext &context)
{
   auto &project = context.project;
   SeekLeftOrRight( project, DIRECTION_LEFT, SELECTION_EXTEND, mSeekInfo );
}

void OnSeekRightLong(const CommandContext &context)
{
   auto &project = context.project;
   SeekLeftOrRight( project, DIRECTION_RIGHT, SELECTION_EXTEND, mSeekInfo );
}

#if 1
// Legacy functions, not used as of version 2.3.0
void OnSelectAllTime(const CommandContext &context)
{
   auto &project = context.project;
   SelectUtilities::DoSelectTimeAndTracks( project, true, false );
}
#endif

void UpdatePrefs() override
{
   gPrefs->Read(L"/AudioIO/SeekShortPeriod", &mSeekInfo.mSeekShort, 1.0);
   gPrefs->Read(L"/AudioIO/SeekLongPeriod", &mSeekInfo.mSeekLong, 15.0);
}
Handler()
{
   UpdatePrefs();
}
Handler( const Handler & ) PROHIBITED;
Handler &operator=( const Handler & ) PROHIBITED;

}; // struct Handler

} // namespace

// Handler is stateful.  Needs a factory registered with
// AudacityProject.
static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject&) {
      return std::make_unique< SelectActions::Handler >(); } };

static CommandHandlerObject &findCommandHandler(AudacityProject &project) {
   return project.AttachedObjects::Get< SelectActions::Handler >( key );
};

// Menu definitions

#define FN(X) (& SelectActions::Handler :: X)

namespace {
using namespace MenuTable;
BaseItemSharedPtr SelectMenu()
{
   using Options = CommandManager::Options;
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   /* i18n-hint: (verb) It's an item on a menu. */
   Menu( L"Select", XXO("&Select"),
      Section( "Basic",
         Command( L"SelectAll", XXO("&All"), FN(OnSelectAll),
            TracksExistFlag(),
            Options{ L"Ctrl+A", XO("Select All") } ),
         Command( L"SelectNone", XXO("&None"), FN(OnSelectNone),
            TracksExistFlag(),
            Options{ L"Ctrl+Shift+A", XO("Select None") } ),

         //////////////////////////////////////////////////////////////////////////

         Menu( L"Tracks", XXO("&Tracks"),
            Command( L"SelAllTracks", XXO("In All &Tracks"),
               FN(OnSelectAllTracks),
               TracksExistFlag(),
               L"Ctrl+Shift+K" )

   #ifdef EXPERIMENTAL_SYNC_LOCK
            ,
            Command( L"SelSyncLockTracks", XXO("In All &Sync-Locked Tracks"),
               FN(OnSelectSyncLockSel),
               EditableTracksSelectedFlag() | IsSyncLockedFlag(),
               Options{ L"Ctrl+Shift+Y", XO("Select Sync-Locked") } )
   #endif
         ),

         //////////////////////////////////////////////////////////////////////////

         Menu( L"Region", XXO("R&egion"),
            Section( "",
               Command( L"SetLeftSelection", XXO("&Left at Playback Position"),
                  FN(OnSetLeftSelection), TracksExistFlag(),
                  Options{ L"[", XO("Set Selection Left at Play Position") } ),
               Command( L"SetRightSelection", XXO("&Right at Playback Position"),
                  FN(OnSetRightSelection), TracksExistFlag(),
                  Options{ L"]", XO("Set Selection Right at Play Position") } ),
               Command( L"SelTrackStartToCursor", XXO("Track &Start to Cursor"),
                  FN(OnSelectStartCursor), AlwaysEnabledFlag,
                  Options{ L"Shift+J", XO("Select Track Start to Cursor") } ),
               Command( L"SelCursorToTrackEnd", XXO("Cursor to Track &End"),
                  FN(OnSelectCursorEnd), AlwaysEnabledFlag,
                  Options{ L"Shift+K", XO("Select Cursor to Track End") } ),
               Command( L"SelTrackStartToEnd", XXO("Track Start to En&d"),
                  FN(OnSelectTrackStartToEnd), AlwaysEnabledFlag,
                  Options{}.LongName( XO("Select Track Start to End") ) )
            ),

            Section( "",
               // GA: Audacity had 'Store Re&gion' here previously. There is no
               // one-step way to restore the 'Saved Cursor Position' in Select Menu,
               // so arguably using the word 'Selection' to do duty for both saving
               // the region or the cursor is better. But it does not belong in a
               // 'Region' submenu.
               Command( L"SelSave", XXO("S&tore Selection"), FN(OnSelectionSave),
                  WaveTracksSelectedFlag() ),
               // Audacity had 'Retrieve Regio&n' here previously.
               Command( L"SelRestore", XXO("Retrieve Selectio&n"),
                  FN(OnSelectionRestore), TracksExistFlag() )
            )
         ),

         //////////////////////////////////////////////////////////////////////////

   #ifdef EXPERIMENTAL_SPECTRAL_EDITING
         Menu( L"Spectral", XXO("S&pectral"),
            Command( L"ToggleSpectralSelection",
               XXO("To&ggle Spectral Selection"), FN(OnToggleSpectralSelection),
               TracksExistFlag(), L"Q" ),
            Command( L"NextHigherPeakFrequency",
               XXO("Next &Higher Peak Frequency"), FN(OnNextHigherPeakFrequency),
               TracksExistFlag() ),
            Command( L"NextLowerPeakFrequency",
               XXO("Next &Lower Peak Frequency"), FN(OnNextLowerPeakFrequency),
               TracksExistFlag() )
         )
   #endif
      ),

      Section( "",
         Command( L"SelCursorStoredCursor",
            XXO("Cursor to Stored &Cursor Position"),
            FN(OnSelectCursorStoredCursor), TracksExistFlag(),
            Options{}.LongName( XO("Select Cursor to Stored") ) ),

         Command( L"StoreCursorPosition", XXO("Store Cursor Pos&ition"),
            FN(OnCursorPositionStore),
            WaveTracksExistFlag() )
         // Save cursor position is used in some selections.
         // Maybe there should be a restore for it?
      ),

      Section( "",
         Command( L"ZeroCross", XXO("At &Zero Crossings"),
            FN(OnZeroCrossing), EditableTracksSelectedFlag(),
            Options{ L"Z", XO("Select Zero Crossing") } )
      )
   ) ) };
   return menu;
}

AttachedItem sAttachment1{
   L"",
   Shared( SelectMenu() )
};

BaseItemSharedPtr ExtraSelectionMenu()
{
   using Options = CommandManager::Options;
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"Select", XXO("&Selection"),
      Command( L"SnapToOff", XXO("Snap-To &Off"), FN(OnSnapToOff),
         AlwaysEnabledFlag ),
      Command( L"SnapToNearest", XXO("Snap-To &Nearest"),
         FN(OnSnapToNearest), AlwaysEnabledFlag ),
      Command( L"SnapToPrior", XXO("Snap-To &Prior"), FN(OnSnapToPrior),
         AlwaysEnabledFlag ),
      Command( L"SelStart", XXO("Selection to &Start"), FN(OnSelToStart),
         AlwaysEnabledFlag, L"Shift+Home" ),
      Command( L"SelEnd", XXO("Selection to En&d"), FN(OnSelToEnd),
         AlwaysEnabledFlag, L"Shift+End" ),
      Command( L"SelExtLeft", XXO("Selection Extend &Left"),
         FN(OnSelExtendLeft),
         TracksExistFlag() | TrackPanelHasFocus(),
         Options{ L"Shift+Left" }.WantKeyUp().AllowDup() ),
      Command( L"SelExtRight", XXO("Selection Extend &Right"),
         FN(OnSelExtendRight),
         TracksExistFlag() | TrackPanelHasFocus(),
         Options{ L"Shift+Right" }.WantKeyUp().AllowDup() ),
      Command( L"SelSetExtLeft", XXO("Set (or Extend) Le&ft Selection"),
         FN(OnSelSetExtendLeft),
         TracksExistFlag() | TrackPanelHasFocus() ),
      Command( L"SelSetExtRight", XXO("Set (or Extend) Rig&ht Selection"),
         FN(OnSelSetExtendRight),
         TracksExistFlag() | TrackPanelHasFocus() ),
      Command( L"SelCntrLeft", XXO("Selection Contract L&eft"),
         FN(OnSelContractLeft),
         TracksExistFlag() | TrackPanelHasFocus(),
         Options{ L"Ctrl+Shift+Right" }.WantKeyUp() ),
      Command( L"SelCntrRight", XXO("Selection Contract R&ight"),
         FN(OnSelContractRight),
         TracksExistFlag() | TrackPanelHasFocus(),
         Options{ L"Ctrl+Shift+Left" }.WantKeyUp() )
   ) ) };
   return menu;
}

AttachedItem sAttachment2{
   L"Optional/Extra/Part1",
   Shared( ExtraSelectionMenu() )
};
}

namespace {
BaseItemSharedPtr CursorMenu()
{
   using Options = CommandManager::Options;
   static const auto CanStopFlags = AudioIONotBusyFlag() | CanStopAudioStreamFlag();

   // JKC: ANSWER-ME: How is 'cursor to' different to 'Skip To' and how is it
   // useful?
   // GA: 'Skip to' moves the viewpoint to center of the track and preserves the
   // selection. 'Cursor to' does neither. 'Center at' might describe it better
   // than 'Skip'.
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"Cursor", XXO("&Cursor to"),
      Command( L"CursSelStart", XXO("Selection Star&t"),
         FN(OnCursorSelStart),
         TimeSelectedFlag(),
         Options{}.LongName( XO("Cursor to Selection Start") ) ),
      Command( L"CursSelEnd", XXO("Selection En&d"),
         FN(OnCursorSelEnd),
         TimeSelectedFlag(),
         Options{}.LongName( XO("Cursor to Selection End") ) ),

      Command( L"CursTrackStart", XXO("Track &Start"),
         FN(OnCursorTrackStart),
         EditableTracksSelectedFlag(),
         Options{ L"J", XO("Cursor to Track Start") } ),
      Command( L"CursTrackEnd", XXO("Track &End"),
         FN(OnCursorTrackEnd),
         EditableTracksSelectedFlag(),
         Options{ L"K", XO("Cursor to Track End") } ),

      Command( L"CursProjectStart", XXO("&Project Start"),
         FN(OnSkipStart),
         CanStopFlags,
         Options{ L"Home", XO("Cursor to Project Start") } ),
      Command( L"CursProjectEnd", XXO("Project E&nd"), FN(OnSkipEnd),
         CanStopFlags,
         Options{ L"End", XO("Cursor to Project End") } )
   ) ) };
   return menu;
}

AttachedItem sAttachment0{
   L"Transport/Basic",
   Shared( CursorMenu() )
};

BaseItemSharedPtr ExtraCursorMenu()
{
   using Options = CommandManager::Options;
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"Cursor", XXO("&Cursor"),
      Command( L"CursorLeft", XXO("Cursor &Left"), FN(OnCursorLeft),
         TracksExistFlag() | TrackPanelHasFocus(),
         Options{ L"Left" }.WantKeyUp().AllowDup() ),
      Command( L"CursorRight", XXO("Cursor &Right"), FN(OnCursorRight),
         TracksExistFlag() | TrackPanelHasFocus(),
         Options{ L"Right" }.WantKeyUp().AllowDup() ),
      Command( L"CursorShortJumpLeft", XXO("Cursor Sh&ort Jump Left"),
         FN(OnCursorShortJumpLeft),
         TracksExistFlag() | TrackPanelHasFocus(), L"," ),
      Command( L"CursorShortJumpRight", XXO("Cursor Shor&t Jump Right"),
         FN(OnCursorShortJumpRight),
         TracksExistFlag() | TrackPanelHasFocus(), L"." ),
      Command( L"CursorLongJumpLeft", XXO("Cursor Long J&ump Left"),
         FN(OnCursorLongJumpLeft),
         TracksExistFlag() | TrackPanelHasFocus(), L"Shift+," ),
      Command( L"CursorLongJumpRight", XXO("Cursor Long Ju&mp Right"),
         FN(OnCursorLongJumpRight),
         TracksExistFlag() | TrackPanelHasFocus(), L"Shift+." )
   ) ) };
   return menu;
}

AttachedItem sAttachment4{
   L"Optional/Extra/Part2",
   Shared( ExtraCursorMenu() )
};

BaseItemSharedPtr ExtraSeekMenu()
{
   using Options = CommandManager::Options;
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"Seek", XXO("See&k"),
      Command( L"SeekLeftShort", XXO("Short Seek &Left During Playback"),
         FN(OnSeekLeftShort), AudioIOBusyFlag(),
         Options{ L"Left" }.AllowDup() ),
      Command( L"SeekRightShort",
         XXO("Short Seek &Right During Playback"), FN(OnSeekRightShort),
         AudioIOBusyFlag(),
         Options{ L"Right" }.AllowDup() ),
      Command( L"SeekLeftLong", XXO("Long Seek Le&ft During Playback"),
         FN(OnSeekLeftLong), AudioIOBusyFlag(),
         Options{ L"Shift+Left" }.AllowDup() ),
      Command( L"SeekRightLong", XXO("Long Seek Rig&ht During Playback"),
         FN(OnSeekRightLong), AudioIOBusyFlag(),
         Options{ L"Shift+Right" }.AllowDup() )
   ) ) };
   return menu;
}

AttachedItem sAttachment5{
   L"Optional/Extra/Part1",
   Shared( ExtraSeekMenu() )
};

}

#undef FN
