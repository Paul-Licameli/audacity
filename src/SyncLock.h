/**********************************************************************

Audacity: A Digital Audio Editor

@file SyncLock.cpp
@brief Defines groupings of tracks that should keep contents synchronized

Paul Licameli split from Track.cpp

**********************************************************************/
#ifndef __AUDACITY_SYNC_LOCK__
#define __AUDACITY_SYNC_LOCK__

#include "Track.h" // for TrackIterRange

class AUDACITY_DLL_API SyncLock {
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
