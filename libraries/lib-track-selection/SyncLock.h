/**********************************************************************

Audacity: A Digital Audio Editor

@file SyncLock.cpp
@brief Defines groupings of tracks that should keep contents synchronized

Paul Licameli split from Track.cpp

**********************************************************************/
#ifndef __AUDACITY_SYNC_LOCK__
#define __AUDACITY_SYNC_LOCK__

#include "Track.h" // for TrackIterRange

//! Event emitted by the project when sync lock state changes
struct SyncLockChangeEvent : wxEvent{
   explicit SyncLockChangeEvent(bool on);
   ~SyncLockChangeEvent() override;
   wxEvent *Clone() const override;
   bool mIsOn; //!< state sync lock has after the change
};

// Sent to the project when certain settings change
wxDECLARE_EXPORTED_EVENT(TRACK_SELECTION_API,
   EVT_SYNC_LOCK_CHANGE, SyncLockChangeEvent);

class TRACK_SELECTION_API SyncLockState final
   : public AttachedProjectObject
{
public:
   static SyncLockState &Get( AudacityProject &project );
   static const SyncLockState &Get( const AudacityProject &project );
   explicit SyncLockState( AudacityProject &project );
   SyncLockState( const SyncLockState & ) = delete;
   SyncLockState &operator=( const SyncLockState & ) = delete;

   bool IsSyncLocked() const;
   void SetSyncLock(bool flag);

private:
   AudacityProject &mProject;
   bool mIsSyncLocked{ false };
};

class TRACK_SELECTION_API SyncLock {
public:
   //! Checks if sync-lock is on and any track in its sync-lock group is selected.
   static bool IsSyncLockSelected( const Track *pTrack );

   //! Checks if sync-lock is on and any track in its sync-lock group is selected.
   static bool IsSelectedOrSyncLockSelected( const Track *pTrack );

   /*! @pre `pTrack->GetOwner() != nullptr` */
   static TrackIterRange< Track > Group( Track *pTrack );

   /*! @copydoc Group */
   static TrackIterRange< const Track > Group( const Track *pTrack )
   {
      return Group(const_cast<Track*>(pTrack)).Filter<const Track>();
   }
};

#endif
