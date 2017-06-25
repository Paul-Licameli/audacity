/**********************************************************************

Audacity: A Digital Audio Editor

TrackSelectHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "TrackSelectHandle.h"
#include "TrackControls.h"
#include "../../Experimental.h"
#include "../../HitTestResult.h"
#include "../../MixerBoard.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../WaveTrack.h"

#include "../../MemoryX.h"
#include <wx/cursor.h>
#include <wx/translation.h>

#include "../../../images/Cursors.h"

TrackSelectHandle::TrackSelectHandle()
{
}

TrackSelectHandle &TrackSelectHandle::Instance()
{
   static TrackSelectHandle instance;
   return instance;
}

#if defined(__WXMAC__)
/* i18n-hint: Command names a modifier key on Macintosh keyboards */
#define CTRL_CLICK _("Command-Click")
#else
/* i18n-hint: Ctrl names a modifier key on Windows or Linux keyboards */
#define CTRL_CLICK _("Ctrl-Click")
#endif

namespace {
   wxString Message(unsigned trackCount) {
      if (trackCount > 1)
         // i18n-hint: %s is replaced by (translation of) 'Ctrl-Click' on windows, 'Command-Click' on Mac
         return wxString::Format(
            _("%s to select or deselect track. Drag up or down to change track order."),
            CTRL_CLICK );
      else
         // i18n-hint: %s is replaced by (translation of) 'Ctrl-Click' on windows, 'Command-Click' on Mac
         return wxString::Format(
            _("%s to select or deselect track."),
            CTRL_CLICK );
   }
}

HitTestPreview TrackSelectHandle::HitPreview(unsigned trackCount)
{
   static wxCursor arrowCursor{ wxCURSOR_ARROW };
   return {
      Message(trackCount),
       &arrowCursor
   };
}

HitTestResult TrackSelectHandle::HitAnywhere(unsigned trackCount)
{
   return {
      HitPreview(trackCount),
      &Instance()
   };
}

TrackSelectHandle::~TrackSelectHandle()
{
}

UIHandle::Result TrackSelectHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   Result result = RefreshNone;

   const wxMouseEvent &event = evt.event;

   // AS: If not a click, ignore the mouse event.
   if (!event.ButtonDown() && !event.ButtonDClick())
      return Cancelled;
   if (!event.Button(wxMOUSE_BTN_LEFT))
      return Cancelled;

   TrackControls *const pControls = static_cast<TrackControls*>(evt.pCell);
   Track *const pTrack = pControls->GetTrack();
   TrackPanel *const trackPanel = pProject->GetTrackPanel();
   const bool unsafe = pProject->IsAudioActive();

   // DM: If they weren't clicking on a particular part of a track label,
   //  deselect other tracks and select this one.

   // JH: also, capture the current track for rearranging, so the user
   //  can drag the track up or down to swap it with others
   if (unsafe)
      result |= Cancelled;
   else {
      mRearrangeCount = 0;
      mpTrack = pTrack;
      CalculateRearrangingThresholds(event);
   }

   pProject->HandleListSelection
      (pTrack, event.ShiftDown(), event.ControlDown(), !unsafe);

   return result;
}

UIHandle::Result TrackSelectHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   Result result = RefreshNone;

   const wxMouseEvent &event = evt.event;

   TrackList *const tracks = pProject->GetTracks();

   // probably harmless during play?  However, we do disallow the click, so check this too.
   bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return result;

   MixerBoard* pMixerBoard = pProject->GetMixerBoard(); // Update mixer board, too.
   if (event.m_y < mMoveUpThreshold || event.m_y < 0) {
      { TrackList::Locker locker{ tracks->mLock };
         tracks->MoveUp(&locker, mpTrack);
      }
      --mRearrangeCount;
      if (pMixerBoard)
         if(auto pPlayable = dynamic_cast< const PlayableTrack* >( mpTrack ))
            pMixerBoard->MoveTrackCluster(pPlayable, true /* up */);
   }
   else if ( event.m_y > mMoveDownThreshold
      || event.m_y > evt.whole.GetHeight() ) {
      { TrackList::Locker locker{ tracks->mLock };
         tracks->MoveDown(&locker, mpTrack);
      }
      ++mRearrangeCount;
      if (pMixerBoard)
         if(auto pPlayable = dynamic_cast< const PlayableTrack* >( mpTrack ))
            pMixerBoard->MoveTrackCluster(pPlayable, false /* down */);
   }
   else
      return result;

   // JH: if we moved up or down, recalculate the thresholds and make sure the
   // track is fully on-screen.
   CalculateRearrangingThresholds(event);

   result |= EnsureVisible;
   return result;
}

HitTestPreview TrackSelectHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *project)
{
   // Note that this differs from HitPreview.

   static auto disabledCursor =
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16);
   static wxCursor rearrangeCursor{ wxCURSOR_HAND };

   const bool unsafe = GetActiveProject()->IsAudioActive();
   return {
      Message(project->GetTrackPanel()->GetTrackCount()),
      (unsafe
         ? &*disabledCursor
         : &rearrangeCursor)
   };
}

UIHandle::Result TrackSelectHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *, wxWindow *)
{
   if (mRearrangeCount != 0) {
      AudacityProject *const project = ::GetActiveProject();
      wxString dir;
      /* i18n-hint: a direction as in up or down.*/
      dir = mRearrangeCount < 0 ? _("up") : _("down");
/* i18n-hint: will substitute name of track for first %s, "up" or "down" for the other.*/
      project->PushState(wxString::Format(_("Moved '%s' %s"),
         mpTrack->GetName().c_str(),
         dir.c_str()),
         _("Move Track"));
   }
   // No need to redraw, that was done when drag moved the track
   return RefreshCode::RefreshNone;
}

UIHandle::Result TrackSelectHandle::Cancel(AudacityProject *pProject)
{
   pProject->RollbackState();
   return RefreshCode::RefreshAll;
}

/// Figure out how far the user must drag the mouse up or down
/// before the track will swap with the one above or below
void TrackSelectHandle::CalculateRearrangingThresholds(const wxMouseEvent & event)
{
   // JH: this will probably need to be tweaked a bit, I'm just
   //   not sure what formula will have the best feel for the
   //   user.

   AudacityProject *const project = ::GetActiveProject();
   TrackList *const tracks = project->GetTracks();

   if (tracks->CanMoveUp(mpTrack))
      mMoveUpThreshold =
      event.m_y - tracks->GetGroupHeight(tracks->GetPrev(mpTrack, true));
   else
      mMoveUpThreshold = INT_MIN;

   if (tracks->CanMoveDown(mpTrack))
      mMoveDownThreshold =
      event.m_y + tracks->GetGroupHeight(tracks->GetNext(mpTrack, true));
   else
      mMoveDownThreshold = INT_MAX;
}
