/**********************************************************************

Audacity: A Digital Audio Editor

@file SyncLock.cpp
@brief implements sync-lock logic

Paul Licameli split from Track.cpp

**********************************************************************/

#include "SyncLock.h"

#include "Track.h"

wxDEFINE_EVENT(EVT_SYNC_LOCK_CHANGE, SyncLockChangeEvent);

SyncLockChangeEvent::SyncLockChangeEvent(bool on)
   : wxEvent{ -1, EVT_SYNC_LOCK_CHANGE }
   , mIsOn{ on }
{}
SyncLockChangeEvent::~SyncLockChangeEvent() = default;
wxEvent *SyncLockChangeEvent::Clone() const
{
   return safenew SyncLockChangeEvent{*this};
}

namespace {
   void Notify( AudacityProject &project, bool on )
   {
      SyncLockChangeEvent e{ on };
      project.ProcessEvent( e );
   }
}

static const AudacityProject::AttachedObjects::RegisteredFactory
sSyncLockStateKey{
  []( AudacityProject &project ){
     auto result = std::make_shared< SyncLockState >( project );
     return result;
   }
};

SyncLockState &SyncLockState::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< SyncLockState >(
      sSyncLockStateKey );
}

const SyncLockState &SyncLockState::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

SyncLockState::SyncLockState(AudacityProject &project)
   : mProject{project}
{
   gPrefs->Read(wxT("/GUI/SyncLockTracks"), &mIsSyncLocked, false);
}

bool SyncLockState::IsSyncLocked() const
{
#ifdef EXPERIMENTAL_SYNC_LOCK
   return mIsSyncLocked;
#else
   return false;
#endif
}

void SyncLockState::SetSyncLock(bool flag)
{
   if (flag != mIsSyncLocked) {
      mIsSyncLocked = flag;
      Notify( mProject, flag );
   }
}

namespace {
inline bool IsSyncLockableNonSeparatorTrack( const Track *pTrack )
{
   return pTrack && pTrack->GetSyncLockPolicy() == Track::Grouped;
}

inline bool IsSeparatorTrack( const Track *pTrack )
{
   return pTrack && pTrack->GetSyncLockPolicy() == Track::EndSeparator;
}

bool IsGoodNextSyncLockTrack(const Track *t, bool inSeparatorSection)
{
   if (!t)
      return false;
   const bool isSeparator = IsSeparatorTrack(t);
   if (inSeparatorSection)
      return isSeparator;
   else if (isSeparator)
      return true;
   else
      return IsSyncLockableNonSeparatorTrack( t );
}

std::pair<Track *, Track *> FindSyncLockGroup(Track *pMember)
{
   if (!pMember)
      return { nullptr, nullptr };

   // A non-trivial sync-locked group is a maximal sub-sequence of the tracks
   // consisting of any positive number of audio tracks followed by zero or
   // more label tracks.

   // Step back through any label tracks.
   auto pList = pMember->GetOwner();
   auto member = pList->Find(pMember);
   while (*member && IsSeparatorTrack(*member) )
      --member;

   // Step back through the wave and note tracks before the label tracks.
   Track *first = nullptr;
   while (*member && IsSyncLockableNonSeparatorTrack(*member)) {
      first = *member;
      --member;
   }

   if (!first)
      // Can't meet the criteria described above.  In that case,
      // consider the track to be the sole member of a group.
      return { pMember, pMember };

   auto last = pList->Find(first);
   auto next = last;
   bool inLabels = false;

   while (*++next) {
      if ( ! IsGoodNextSyncLockTrack(*next, inLabels) )
         break;
      last = next;
      inLabels = IsSeparatorTrack(*last);
   }

   return { first, *last };
}

}

bool SyncLock::IsSyncLockSelected( const Track *pTrack )
{
   if (!pTrack)
      return false;
   auto pList = pTrack->GetOwner();
   if (!pList)
      return false;

   auto p = pList->GetOwner();
   if (!p || !SyncLockState::Get( *p ).IsSyncLocked())
      return false;

   auto shTrack = pTrack->SubstituteOriginalTrack();
   if (!shTrack)
      return false;

   const auto pOrigTrack = shTrack.get();
   auto trackRange = Group( pOrigTrack );

   if (trackRange.size() <= 1) {
      // Not in a sync-locked group.
      // Return true iff selected and of a sync-lockable type.
      return (IsSyncLockableNonSeparatorTrack( pOrigTrack ) ||
              IsSeparatorTrack( pOrigTrack )) && pTrack->GetSelected();
   }

   // Return true iff any track in the group is selected.
   return *(trackRange + &Track::IsSelected).begin();
}

bool SyncLock::IsSelectedOrSyncLockSelected( const Track *pTrack )
{
   return pTrack && (pTrack->IsSelected() || IsSyncLockSelected(pTrack));
}

TrackIterRange< Track > SyncLock::Group( Track *pTrack )
{
   auto pList = pTrack->GetOwner();
   auto tracks = FindSyncLockGroup(const_cast<Track*>( pTrack ) );
   return pList->Any().StartingWith(tracks.first).EndingAfter(tracks.second);
}
