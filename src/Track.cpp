/**********************************************************************

  Audacity: A Digital Audio Editor

  Track.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Track
\brief Fundamental data object of Audacity, placed in the TrackPanel.
Classes derived form it include the WaveTrack, NoteTrack, LabelTrack
and TimeTrack.

\class AudioTrack
\brief A Track that can load/save audio data to/from XML.

\class PlayableTrack
\brief An AudioTrack that can be played and stopped.

*//*******************************************************************/

#include "Audacity.h" // for USE_* macros

#include "Track.h"

#include "Experimental.h"

#include <algorithm>
#include <numeric>

#include <float.h>
#include <wx/file.h>
#include <wx/textfile.h>
#include <wx/log.h>

#include "TimeTrack.h"
#include "WaveTrack.h"
#include "NoteTrack.h"
#include "LabelTrack.h"
#include "Project.h"
#include "DirManager.h"

#include "InconsistencyException.h"

#include "TrackPanel.h" // for TrackInfo

#ifdef _MSC_VER
//Disable truncation warnings
#pragma warning( disable : 4786 )
#endif

Track::Track(const std::shared_ptr<DirManager> &projDirManager)
:  vrulerSize(36,0),
   mDirManager(projDirManager)
{
   mSelected  = false;
   mLinked    = false;

   mY = 0;
   mHeight = DefaultHeight;
   mIndex = 0;

   mMinimized = false;

   mOffset = 0.0;
}

Track::Track(const Track &orig)
: vrulerSize( orig.vrulerSize )
{
   mY = 0;
   mIndex = 0;
   Init(orig);
   mOffset = orig.mOffset;
}

// Copy all the track properties except the actual contents
void Track::Init(const Track &orig)
{
   mId = orig.mId;

   mDefaultName = orig.mDefaultName;
   mName = orig.mName;

   mDirManager = orig.mDirManager;

   mSelected = orig.mSelected;
   mLinked = orig.mLinked;
   mHeight = orig.mHeight;
   mMinimized = orig.mMinimized;
}

void Track::SetName( const wxString &n )
{
   if ( mName != n ) {
      mName = n;
      Notify();
   }
}

void Track::SetSelected(bool s)
{
   if (mSelected != s) {
      mSelected = s;
      auto pList = mList.lock();
      if (pList)
         pList->SelectionEvent( SharedPointer() );
   }
}

void Track::Merge(const Track &orig)
{
   mSelected = orig.mSelected;
}

Track::Holder Track::Duplicate() const
{
   // invoke "virtual constructor" to copy track object proper:
   auto result = Clone();

   // Always must do shallow copy of certain attached, shared objects here
   // (the delayed cloning of them may happen later when the duplicate track
   // is added to a TrackList, or else avoided when the track replaces its
   // original in the same TrackList)

   return result;
}

Track::~Track()
{
}


TrackNodePointer Track::GetNode() const
{
   wxASSERT(mList.lock() == NULL || this == mNode.first->get());
   return mNode;
}

void Track::SetOwner
(const std::weak_ptr<TrackList> &list, TrackNodePointer node)
{
   // BUG: When using this function to clear an owner, we may need to clear 
   // focussed track too.  Otherwise focus could remain on an invisible (or deleted) track.
   mList = list;
   mNode = node;
}

int Track::GetMinimizedHeight() const
{
   auto height = TrackInfo::MinimumTrackHeight();
   auto channels = TrackList::Channels(this->SubstituteOriginalTrack().get());
   auto nChannels = channels.size();
   auto begin = channels.begin();
   auto index = std::distance(begin, std::find(begin, channels.end(), this));
   return (height * (index + 1) / nChannels) - (height * index / nChannels);
}

int Track::GetIndex() const
{
   return mIndex;
}

void Track::SetIndex(int index)
{
   mIndex = index;
}

int Track::GetY() const
{
   return mY;
}

void Track::SetY(int y)
{
   auto pList = mList.lock();
   if (pList && !pList->mPendingUpdates.empty()) {
      auto orig = pList->FindById( GetId() );
      if (orig && orig != this) {
         // delegate, and rely on the update to copy back
         orig->SetY(y);
         pList->UpdatePendingTracks();
         return;
      }
   }

   DoSetY(y);
}

void Track::DoSetY(int y)
{
   mY = y;
}

int Track::GetHeight() const
{
   if (mMinimized) {
      return GetMinimizedHeight();
   }

   return mHeight;
}

void Track::SetHeight(int h)
{
   auto pList = mList.lock();
   if (pList && !pList->mPendingUpdates.empty()) {
      auto orig = pList->FindById( GetId() );
      if (orig && orig != this) {
         // delegate, and rely on RecalcPositions to copy back
         orig->SetHeight(h);
         return;
      }
   }

   DoSetHeight(h);

   if (pList) {
      pList->RecalcPositions(mNode);
      pList->ResizingEvent(mNode);
   }
}

void Track::DoSetHeight(int h)
{
   mHeight = h;
}

bool Track::GetMinimized() const
{
   return mMinimized;
}

void Track::SetMinimized(bool isMinimized)
{
   auto pList = mList.lock();
   if (pList && !pList->mPendingUpdates.empty()) {
      auto orig = pList->FindById( GetId() );
      if (orig && orig != this) {
         // delegate, and rely on RecalcPositions to copy back
         orig->SetMinimized(isMinimized);
         return;
      }
   }

   DoSetMinimized(isMinimized);

   if (pList) {
      pList->RecalcPositions(mNode);
      pList->ResizingEvent(mNode);
   }
}

void Track::DoSetMinimized(bool isMinimized)
{
   mMinimized = isMinimized;
}

void Track::SetLinked(bool l)
{
   auto pList = mList.lock();
   if (pList && !pList->mPendingUpdates.empty()) {
      auto orig = pList->FindById( GetId() );
      if (orig && orig != this) {
         // delegate, and rely on RecalcPositions to copy back
         orig->SetLinked(l);
         return;
      }
   }

   DoSetLinked(l);

   if (pList) {
      pList->RecalcPositions(mNode);
      pList->ResizingEvent(mNode);
   }
}

void Track::DoSetLinked(bool l)
{
   mLinked = l;
}

Track *Track::GetLink() const
{
   auto pList = mList.lock();
   if (!pList)
      return nullptr;

   if (!pList->isNull(mNode)) {
      if (mLinked) {
         auto next = pList->getNext( mNode );
         if ( !pList->isNull( next ) )
            return next.first->get();
      }

      if (mNode.first != mNode.second->begin()) {
         auto prev = pList->getPrev( mNode );
         if ( !pList->isNull( prev ) ) {
            auto track = prev.first->get();
            if (track && track->GetLinked())
               return track;
         }
      }
   }

   return nullptr;
}

namespace {
   inline bool IsSyncLockableNonLabelTrack( const Track *pTrack )
   {
      return nullptr != track_cast< const AudioTrack * >( pTrack );
   }

   bool IsGoodNextSyncLockTrack(const Track *t, bool inLabelSection)
   {
      if (!t)
         return false;
      const bool isLabel = ( nullptr != track_cast<const LabelTrack*>(t) );
      if (inLabelSection)
         return isLabel;
      else if (isLabel)
         return true;
      else
         return IsSyncLockableNonLabelTrack( t );
   }
}

bool Track::IsSyncLockSelected() const
{
#ifdef EXPERIMENTAL_SYNC_LOCK
   AudacityProject *p = GetActiveProject();
   if (!p || !p->IsSyncLocked())
      return false;

   auto pList = mList.lock();
   if (!pList)
      return false;

   auto shTrack = this->SubstituteOriginalTrack();
   if (!shTrack)
      return false;
   
   const auto pTrack = shTrack.get();
   auto trackRange = TrackList::SyncLockGroup( pTrack );

   if (trackRange.size() <= 1) {
      // Not in a sync-locked group.
      // Return true iff selected and of a sync-lockable type.
      return (IsSyncLockableNonLabelTrack( pTrack ) ||
              track_cast<const LabelTrack*>( pTrack )) && GetSelected();
   }

   // Return true iff any track in the group is selected.
   return *(trackRange + &Track::IsSelected).begin();
#endif

   return false;
}

void Track::Notify( int code )
{
   auto pList = mList.lock();
   if (pList)
      pList->DataEvent( SharedPointer(), code );
}

void Track::SyncLockAdjust(double oldT1, double newT1)
{
   if (newT1 > oldT1) {
      // Insert space within the track

      if (oldT1 > GetEndTime())
         return;

      auto tmp = Cut(oldT1, GetEndTime());

      Paste(newT1, tmp.get());
   }
   else if (newT1 < oldT1) {
      // Remove from the track
      Clear(newT1, oldT1);
   }
}

std::shared_ptr<Track> Track::DoFindTrack()
{
   return SharedPointer();
}

void PlayableTrack::Init( const PlayableTrack &orig )
{
   mMute = orig.mMute;
   mSolo = orig.mSolo;
   AudioTrack::Init( orig );
}

void PlayableTrack::Merge( const Track &orig )
{
   auto pOrig = dynamic_cast<const PlayableTrack *>(&orig);
   wxASSERT( pOrig );
   mMute = pOrig->mMute;
   mSolo = pOrig->mSolo;
   AudioTrack::Merge( *pOrig );
}

void PlayableTrack::SetMute( bool m )
{
   if ( mMute != m ) {
      mMute = m;
      Notify();
   }
}

void PlayableTrack::SetSolo( bool s  )
{
   if ( mSolo != s ) {
      mSolo = s;
      Notify();
   }
}

// Serialize, not with tags of its own, but as attributes within a tag.
void PlayableTrack::WriteXMLAttributes(XMLWriter &xmlFile) const
{
   xmlFile.WriteAttr(wxT("mute"), mMute);
   xmlFile.WriteAttr(wxT("solo"), mSolo);
   AudioTrack::WriteXMLAttributes(xmlFile);
}

// Return true iff the attribute is recognized.
bool PlayableTrack::HandleXMLAttribute(const wxChar *attr, const wxChar *value)
{
   const wxString strValue{ value };
   long nValue;
   if (!wxStrcmp(attr, wxT("mute")) &&
            XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      mMute = (nValue != 0);
      return true;
   }
   else if (!wxStrcmp(attr, wxT("solo")) &&
            XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      mSolo = (nValue != 0);
      return true;
   }

   return AudioTrack::HandleXMLAttribute(attr, value);
}

bool Track::Any() const
   { return true; }

bool Track::IsSelected() const
   { return GetSelected(); }

bool Track::IsSelectedOrSyncLockSelected() const
   { return GetSelected() || IsSyncLockSelected(); }

bool Track::IsLeader() const
   { return !GetLink() || GetLinked(); }

bool Track::IsSelectedLeader() const
   { return IsSelected() && IsLeader(); }

void Track::FinishCopy
(const Track *n, Track *dest)
{
   if (dest) {
      dest->SetLinked(n->GetLinked());
      dest->SetName(n->GetName());
   }
}

std::pair<Track *, Track *> TrackList::FindSyncLockGroup(Track *pMember)
{
   if (!pMember)
      return { nullptr, nullptr };

   // A non-trivial sync-locked group is a maximal sub-sequence of the tracks
   // consisting of any positive number of audio tracks followed by zero or
   // more label tracks.

   // Get an iterator over non-const tracks
   auto member = Find( pMember );

   // Step back through any label tracks.
   while ( nullptr != track_cast< const LabelTrack* >( *member ) )
      --member;

   // Step back through the wave and note tracks before the label tracks.
   Track *first = nullptr;
   while ( IsSyncLockableNonLabelTrack( *member ) ) {
      first = *member;
      --member;
   }

   if (!first)
      // Can't meet the criteria described above.  In that case,
      // consider the track to be the sole member of a group.
      return { pMember, pMember };

   Track *last = first;
   bool inLabels = false;

   auto next = Find( last );
   while ( * ++ next ) {
      if ( ! IsGoodNextSyncLockTrack( * next, inLabels ) )
         break;
      last = *next;
      inLabels = ( nullptr != track_cast< const LabelTrack* >( last ) );
   }

   return { first, last };
}


// TrackList
//
// The TrackList sends events whenever certain updates occur to the list it
// is managing.  Any other classes that may be interested in get these updates
// should use TrackList::Connect() or TrackList::Bind().
//
wxDEFINE_EVENT(EVT_TRACKLIST_TRACK_DATA_CHANGE, TrackListEvent);
wxDEFINE_EVENT(EVT_TRACKLIST_SELECTION_CHANGE, TrackListEvent);
wxDEFINE_EVENT(EVT_TRACKLIST_PERMUTED, TrackListEvent);
wxDEFINE_EVENT(EVT_TRACKLIST_RESIZING, TrackListEvent);
wxDEFINE_EVENT(EVT_TRACKLIST_ADDITION, TrackListEvent);
wxDEFINE_EVENT(EVT_TRACKLIST_DELETION, TrackListEvent);

// same value as in the default constructed TrackId:
long TrackList::sCounter = -1;

static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject&) { return TrackList::Create(); }
};

TrackList &TrackList::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< TrackList >( key );
}

const TrackList &TrackList::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

TrackList::TrackList()
:  wxEvtHandler()
{
}

// Factory function
std::shared_ptr<TrackList> TrackList::Create()
{
   return std::make_shared<TrackList>();
}

TrackList &TrackList::operator= (TrackList &&that)
{
   if (this != &that) {
      this->Clear();
      Swap(that);
   }
   return *this;
}

void TrackList::Swap(TrackList &that)
{
   auto SwapLOTs = [](
      ListOfTracks &a, const std::weak_ptr< TrackList > &aSelf,
      ListOfTracks &b, const std::weak_ptr< TrackList > &bSelf )
   {
      a.swap(b);
      for (auto it = a.begin(), last = a.end(); it != last; ++it)
         (*it)->SetOwner(aSelf, {it, &a});
      for (auto it = b.begin(), last = b.end(); it != last; ++it)
         (*it)->SetOwner(bSelf, {it, &b});
   };

   const auto self = shared_from_this();
   const auto otherSelf = that.shared_from_this();
   SwapLOTs( *this, self, that, otherSelf );
   SwapLOTs( this->mPendingUpdates, self, that.mPendingUpdates, otherSelf );
   mUpdaters.swap(that.mUpdaters);
}

TrackList::~TrackList()
{
   Clear(false);
}

void TrackList::RecalcPositions(TrackNodePointer node)
{
   if ( isNull( node ) )
      return;

   Track *t;
   int i = 0;
   int y = 0;

   auto prev = getPrev( node );
   if ( !isNull( prev ) ) {
      t = prev.first->get();
      i = t->GetIndex() + 1;
      y = t->GetY() + t->GetHeight();
   }

   const auto theEnd = end();
   for (auto n = Find( node.first->get() ); n != theEnd; ++n) {
      t = *n;
      t->SetIndex(i++);
      t->DoSetY(y);
      y += t->GetHeight();
   }

   UpdatePendingTracks();
}

void TrackList::SelectionEvent( const std::shared_ptr<Track> &pTrack )
{
   // wxWidgets will own the event object
   QueueEvent(
      safenew TrackListEvent{ EVT_TRACKLIST_SELECTION_CHANGE, pTrack } );
}

void TrackList::DataEvent( const std::shared_ptr<Track> &pTrack, int code )
{
   // wxWidgets will own the event object
   QueueEvent(
      safenew TrackListEvent{ EVT_TRACKLIST_TRACK_DATA_CHANGE, pTrack, code } );
}

void TrackList::PermutationEvent()
{
   // wxWidgets will own the event object
   QueueEvent( safenew TrackListEvent{ EVT_TRACKLIST_PERMUTED } );
}

void TrackList::DeletionEvent()
{
   // wxWidgets will own the event object
   QueueEvent( safenew TrackListEvent{ EVT_TRACKLIST_DELETION } );
}

void TrackList::AdditionEvent(TrackNodePointer node)
{
   // wxWidgets will own the event object
   QueueEvent( safenew TrackListEvent{ EVT_TRACKLIST_ADDITION, *node.first } );
}

void TrackList::ResizingEvent(TrackNodePointer node)
{
   // wxWidgets will own the event object
   QueueEvent( safenew TrackListEvent{ EVT_TRACKLIST_RESIZING, *node.first } );
}

auto TrackList::EmptyRange() const
   -> TrackIterRange< Track >
{
   auto it = const_cast<TrackList*>(this)->getEnd();
   return {
      { it, it, it, &Track::Any },
      { it, it, it, &Track::Any }
   };
}

auto TrackList::SyncLockGroup( Track *pTrack )
   -> TrackIterRange< Track >
{
   auto pList = pTrack->GetOwner();
   auto tracks =
      pList->FindSyncLockGroup( pTrack );
   return pList->Any().StartingWith(tracks.first).EndingAfter(tracks.second);
}

auto TrackList::FindLeader( Track *pTrack )
   -> TrackIter< Track >
{
   auto iter = Find(pTrack);
   while( *iter && ! ( *iter )->IsLeader() )
      --iter;
   return iter.Filter( &Track::IsLeader );
}

void TrackList::Permute(const std::vector<TrackNodePointer> &permutation)
{
   for (const auto iter : permutation) {
      ListOfTracks::value_type track = *iter.first;
      erase(iter.first);
      Track *pTrack = track.get();
      pTrack->SetOwner(shared_from_this(),
         { insert(ListOfTracks::end(), track), this });
   }
   auto n = getBegin();
   RecalcPositions(n);
   PermutationEvent();
}

Track *TrackList::FindById( TrackId id )
{
   // Linear search.  Tracks in a project are usually very few.
   // Search only the non-pending tracks.
   auto it = std::find_if( ListOfTracks::begin(), ListOfTracks::end(),
      [=](const ListOfTracks::value_type &ptr){ return ptr->GetId() == id; } );
   if (it == ListOfTracks::end())
      return {};
   return it->get();
}

Track *TrackList::DoAddToHead(const std::shared_ptr<Track> &t)
{
   Track *pTrack = t.get();
   push_front(ListOfTracks::value_type(t));
   auto n = getBegin();
   pTrack->SetOwner(shared_from_this(), n);
   pTrack->SetId( TrackId{ ++sCounter } );
   RecalcPositions(n);
   AdditionEvent(n);
   return front().get();
}

Track *TrackList::DoAdd(const std::shared_ptr<Track> &t, bool)
{
   push_back(t);

   auto n = getPrev( getEnd() );

   t->SetOwner(shared_from_this(), n);
   t->SetId( TrackId{ ++sCounter } );
   RecalcPositions(n);
   AdditionEvent(n);
   return back().get();
}

void TrackList::GroupChannels( Track &track, size_t groupSize )
{
   // If group size is more than two, for now only the first two channels
   // are grouped as stereo, and any others remain mono
   auto list = track.mList.lock();
   if ( groupSize > 0 && list.get() == this  ) {
      auto iter = track.mNode.first;
      auto after = iter;
      auto end = this->ListOfTracks::end();
      auto count = groupSize;
      for ( ; after != end && count; ++after, --count )
         ;
      if ( count == 0 ) {
         auto unlink = [&] ( Track &tr ) {
            if ( tr.GetLinked() ) {
               tr.SetLinked( false );
            }
         };

         // Disassociate previous tracks -- at most one
         auto pLeader = this->FindLeader( &track );
         if ( *pLeader && *pLeader != &track )
            unlink( **pLeader );
         
         // First disassociate given and later tracks, then reassociate them
         for ( auto iter2 = iter; iter2 != after; ++iter2 )
             unlink( **iter2 );

         if ( groupSize > 1 ) {
            const auto channel = *iter++;
            channel->SetLinked( true );
         }
         return;
      }
   }
   // *this does not contain the track or sufficient following channels
   // or group size is zero
   THROW_INCONSISTENCY_EXCEPTION;
}

auto TrackList::Replace(Track * t, const ListOfTracks::value_type &with) ->
   ListOfTracks::value_type
{
   ListOfTracks::value_type holder;
   if (t && with) {
      auto node = t->GetNode();
      t->SetOwner({}, {});

      holder = *node.first;

      Track *pTrack = with.get();
      *node.first = with;
      pTrack->SetOwner(shared_from_this(), node);
      pTrack->SetId( t->GetId() );
      RecalcPositions(node);

      DeletionEvent();
      AdditionEvent(node);
   }
   return holder;
}

TrackNodePointer TrackList::Remove(Track *t)
{
   auto result = getEnd();
   if (t) {
      auto node = t->GetNode();
      t->SetOwner({}, {});

      if ( !isNull( node ) ) {
         ListOfTracks::value_type holder = *node.first;

         result = getNext( node );
         erase(node.first);
         if ( !isNull( result ) )
            RecalcPositions(result);

         DeletionEvent();
      }
   }
   return result;
}

void TrackList::Clear(bool sendEvent)
{
   // Null out the back-pointers in tracks, in case there are outstanding
   // shared_ptrs to those tracks.
   for ( auto pTrack: *this )
      pTrack->SetOwner( {}, {} );
   for ( auto pTrack: mPendingUpdates )
      pTrack->SetOwner( {}, {} );

   ListOfTracks tempList;
   tempList.swap( *this );

   ListOfTracks updating;
   updating.swap( mPendingUpdates );

   mUpdaters.clear();

   if (sendEvent)
      DeletionEvent();
}

/// For mono track height of track
/// For stereo track combined height of both channels.
int TrackList::GetGroupHeight(const Track * t) const
{
   return Channels(t).sum( &Track::GetHeight );
}

bool TrackList::CanMoveUp(Track * t) const
{
   return nullptr != ( * -- FindLeader( t ) );
}

bool TrackList::CanMoveDown(Track * t) const
{
   return nullptr != ( * ++ FindLeader( t ) );
}

// This is used when you want to swap the channel group starting
// at s1 with that starting at s2.
// The complication is that the tracks are stored in a single
// linked list.
void TrackList::SwapNodes(TrackNodePointer s1, TrackNodePointer s2)
{
   // if a null pointer is passed in, we want to know about it
   wxASSERT(!isNull(s1));
   wxASSERT(!isNull(s2));

   // Deal with first track in each team
   s1 = ( * FindLeader( s1.first->get() ) )->GetNode();
   s2 = ( * FindLeader( s2.first->get() ) )->GetNode();

   // Safety check...
   if (s1 == s2)
      return;

   // Be sure s1 is the earlier iterator
   if ((*s1.first)->GetIndex() >= (*s2.first)->GetIndex())
      std::swap(s1, s2);

   // For saving the removed tracks
   using Saved = std::vector< ListOfTracks::value_type >;
   Saved saved1, saved2;

   auto doSave = [&] ( Saved &saved, TrackNodePointer &s ) {
      size_t nn = Channels( s.first->get() ).size();
      saved.resize( nn );
      // Save them in backwards order
      while( nn-- )
         saved[nn] = *s.first, s.first = erase(s.first);
   };

   doSave( saved1, s1 );
   // The two ranges are assumed to be disjoint but might abut
   const bool same = (s1 == s2);
   doSave( saved2, s2 );
   if (same)
      // Careful, we invalidated s1 in the second doSave!
      s1 = s2;

   // Reinsert them
   auto doInsert = [&] ( Saved &saved, TrackNodePointer &s ) {
      Track *pTrack;
      for (auto & pointer : saved)
         pTrack = pointer.get(),
         // Insert before s, and reassign s to point at the new node before
         // old s; which is why we saved pointers in backwards order
         pTrack->SetOwner(shared_from_this(),
            s = { insert(s.first, pointer), this } );
   };
   // This does not invalidate s2 even when it equals s1:
   doInsert( saved2, s1 );
   // Even if s2 was same as s1, this correctly inserts the saved1 range
   // after the saved2 range, when done after:
   doInsert( saved1, s2 );

   // Now correct the Index in the tracks, and other things
   RecalcPositions(s1);
   PermutationEvent();
}

bool TrackList::MoveUp(Track * t)
{
   if (t) {
      Track *p = ( * -- FindLeader( t ) );
      if (p) {
         SwapNodes(p->GetNode(), t->GetNode());
         return true;
      }
   }

   return false;
}

bool TrackList::MoveDown(Track * t)
{
   if (t) {
      Track *n = ( * ++ FindLeader( t ) );
      if (n) {
         SwapNodes(t->GetNode(), n->GetNode());
         return true;
      }
   }

   return false;
}

bool TrackList::Contains(const Track * t) const
{
   return make_iterator_range( *this ).contains( t );
}

bool TrackList::empty() const
{
   return begin() == end();
}

size_t TrackList::size() const
{
   int cnt = 0;

   if (!empty())
      cnt = getPrev( getEnd() ).first->get()->GetIndex() + 1;

   return cnt;
}

TimeTrack *TrackList::GetTimeTrack()
{
   return *Any<TimeTrack>().begin();
}

const TimeTrack *TrackList::GetTimeTrack() const
{
   return const_cast<TrackList*>(this)->GetTimeTrack();
}

namespace {
   template<typename Array, typename TrackRange>
   Array GetTypedTracks(const TrackRange &trackRange,
                       bool selectionOnly, bool includeMuted)
   {
      using Type = typename std::remove_reference<
         decltype( *std::declval<Array>()[0] )
      >::type;
      auto subRange =
         trackRange.template Filter<Type>();
      if ( selectionOnly )
         subRange = subRange + &Track::IsSelected;
      if ( ! includeMuted )
         subRange = subRange - &Type::GetMute;
      return transform_range<Array>( subRange.begin(), subRange.end(),
         []( Type *t ){ return t->template SharedPointer<Type>(); }
      );
   }
}

WaveTrackArray TrackList::GetWaveTrackArray(bool selectionOnly, bool includeMuted)
{
   return GetTypedTracks<WaveTrackArray>(Any(), selectionOnly, includeMuted);
}

WaveTrackConstArray TrackList::GetWaveTrackConstArray(bool selectionOnly, bool includeMuted) const
{
   return GetTypedTracks<WaveTrackConstArray>(Any(), selectionOnly, includeMuted);
}

#if defined(USE_MIDI)
NoteTrackConstArray TrackList::GetNoteTrackConstArray(bool selectionOnly) const
{
   return GetTypedTracks<NoteTrackConstArray>(Any(), selectionOnly, true);
}
#endif

int TrackList::GetHeight() const
{
   int height = 0;

   if (!empty()) {
      auto track = getPrev( getEnd() ).first->get();
      height = track->GetY() + track->GetHeight();
   }

   return height;
}

namespace {
   // Abstract the common pattern of the following three member functions
   inline double Accumulate
      (const TrackList &list,
       double (Track::*memfn)() const,
       double ident,
       const double &(*combine)(const double&, const double&))
   {
      // Default the answer to zero for empty list
      if (list.empty()) {
         return 0.0;
      }

      // Otherwise accumulate minimum or maximum of track values
      return list.Any().accumulate(ident, combine, memfn);
   }
}

double TrackList::GetMinOffset() const
{
   return Accumulate(*this, &Track::GetOffset, DBL_MAX, std::min);
}

double TrackList::GetStartTime() const
{
   return Accumulate(*this, &Track::GetStartTime, DBL_MAX, std::min);
}

double TrackList::GetEndTime() const
{
   return Accumulate(*this, &Track::GetEndTime, -DBL_MAX, std::max);
}

std::shared_ptr<Track>
TrackList::RegisterPendingChangedTrack( Updater updater, Track *src )
{
   std::shared_ptr<Track> pTrack;
   if (src)
      pTrack = src->Duplicate();

   if (pTrack) {
      mUpdaters.push_back( updater );
      mPendingUpdates.push_back( pTrack );
      auto n = mPendingUpdates.end();
      --n;
      pTrack->SetOwner(shared_from_this(), {n, &mPendingUpdates});
   }

   return pTrack;
}

void TrackList::RegisterPendingNewTrack(
   const std::shared_ptr<Track> &pTrack, bool leader )
{
   Add<Track>( pTrack, leader );
   pTrack->SetId( TrackId{} );
}

void TrackList::UpdatePendingTracks()
{
   auto pUpdater = mUpdaters.begin();
   for (const auto &pendingTrack : mPendingUpdates) {
      // Copy just a part of the track state, according to the update
      // function
      const auto &updater = *pUpdater;
      auto src = FindById( pendingTrack->GetId() );
      if (pendingTrack && src) {
         if (updater)
            updater( *pendingTrack, *src );
         pendingTrack->DoSetY(src->GetY());
         pendingTrack->DoSetHeight(src->GetActualHeight());
         pendingTrack->DoSetMinimized(src->GetMinimized());
         pendingTrack->DoSetLinked(src->GetLinked());
      }
      ++pUpdater;
   }
}

void TrackList::ClearPendingTracks( ListOfTracks *pAdded )
// NOFAIL-GUARANTEE
{
   for (const auto &pTrack: mPendingUpdates)
      pTrack->SetOwner( {}, {} );
   mPendingUpdates.clear();
   mUpdaters.clear();

   if (pAdded)
      pAdded->clear();

   for (auto it = ListOfTracks::begin(), stop = ListOfTracks::end();
        it != stop;) {
      if (it->get()->GetId() == TrackId{}) {
         if (pAdded)
            pAdded->push_back( *it );
         (*it)->SetOwner( {}, {} );
         it = erase( it );
      }
      else
         ++it;
   }

   if (!empty())
      RecalcPositions(getBegin());
}

bool TrackList::ApplyPendingTracks()
{
   bool result = false;

   ListOfTracks additions;
   ListOfTracks updates;
   {
      // Always clear, even if one of the update functions throws
      auto cleanup = finally( [&] { ClearPendingTracks( &additions ); } );
      UpdatePendingTracks();
      updates.swap( mPendingUpdates );
   }

   // Remaining steps must be NOFAIL-GUARANTEE so that this function
   // gives STRONG-GUARANTEE

   std::vector< std::shared_ptr<Track> > reinstated;

   for (auto &pendingTrack : updates) {
      if (pendingTrack) {
         auto src = FindById( pendingTrack->GetId() );
         if (src)
            this->Replace(src, pendingTrack), result = true;
         else
            // Perhaps a track marked for pending changes got deleted by
            // some other action.  Recreate it so we don't lose the
            // accumulated changes.
            reinstated.push_back(pendingTrack);
      }
   }

   // If there are tracks to reinstate, append them to the list.
   for (auto &pendingTrack : reinstated)
      if (pendingTrack)
         this->Add( pendingTrack, pendingTrack->IsLeader() ), result = true;

   // Put the pending added tracks back into the list, preserving their
   // positions.
   bool inserted = false;
   ListOfTracks::iterator first;
   for (auto &pendingTrack : additions) {
      if (pendingTrack) {
         auto iter = ListOfTracks::begin();
         std::advance( iter, pendingTrack->GetIndex() );
         iter = ListOfTracks::insert( iter, pendingTrack );
         pendingTrack->SetOwner( shared_from_this(), {iter, this} );
         pendingTrack->SetId( TrackId{ ++sCounter } );
         if (!inserted) {
            first = iter;
            inserted = true;
         }
      }
   }
   if (inserted) {
      RecalcPositions({first, this});
      result = true;
   }

   return result;
}

std::shared_ptr<const Track> Track::SubstitutePendingChangedTrack() const
{
   // Linear search.  Tracks in a project are usually very few.
   auto pList = mList.lock();
   if (pList) {
      const auto id = GetId();
      const auto end = pList->mPendingUpdates.end();
      auto it = std::find_if(
         pList->mPendingUpdates.begin(), end,
         [=](const ListOfTracks::value_type &ptr){ return ptr->GetId() == id; } );
      if (it != end)
         return *it;
   }
   return SharedPointer();
}

std::shared_ptr<const Track> Track::SubstituteOriginalTrack() const
{
   auto pList = mList.lock();
   if (pList) {
      const auto id = GetId();
      const auto pred = [=]( const ListOfTracks::value_type &ptr ) {
         return ptr->GetId() == id; };
      const auto end = pList->mPendingUpdates.end();
      const auto it = std::find_if( pList->mPendingUpdates.begin(), end, pred );
      if (it != end) {
         const auto &list2 = (const ListOfTracks &) *pList;
         const auto end2 = list2.end();
         const auto it2 = std::find_if( list2.begin(), end2, pred );
         if ( it2 != end2 )
            return *it2;
      }
   }
   return SharedPointer();
}

bool Track::sLoadError;
unsigned long Track::sLoadingChannelsCount;
unsigned long Track::sLoadingChannelsCounter;
std::weak_ptr<TrackList> Track::sLoadingChannelsTrackList;

void Track::PreLoad( const std::shared_ptr<TrackList> &pList )
{
   sLoadError = false;
   sLoadingChannelsCount = sLoadingChannelsCounter = 0;
   sLoadingChannelsTrackList = pList;
}

void Track::PostLoad()
{
   if ( sLoadingChannelsCounter < sLoadingChannelsCount ) {
      sLoadError = true;
      wxLogWarning(
         wxT("Defective final channel group: expected %d, found %d."),
         (int)sLoadingChannelsCount,
         (int)sLoadingChannelsCounter
      );
   }

   if (sLoadingChannelsCounter > 1) {
      auto pList = sLoadingChannelsTrackList.lock();
      if ( pList ) {
         auto revIter = pList->Any< WaveTrack >().rbegin();
         std::advance( revIter, sLoadingChannelsCounter - 1 );
         if ( *revIter )
            pList->GroupChannels( **revIter, sLoadingChannelsCounter );
      }
   }
   sLoadingChannelsCount = sLoadingChannelsCounter = 0;
}

// XMLTagHandler callback methods for loading and saving
void Track::HandleXMLEndTag( const wxChar * )
{
   if ( ++sLoadingChannelsCounter >= sLoadingChannelsCount )
      PostLoad();
}

// Serialize, not with tags of its own, but as attributes within a tag.
void Track::WriteCommonXMLAttributes(XMLWriter &xmlFile, bool includeName) const
{
   if (includeName)
      xmlFile.WriteAttr(wxT("name"), GetName());
   xmlFile.WriteAttr(wxT("height"), this->GetActualHeight());
   xmlFile.WriteAttr(wxT("minimized"), this->GetMinimized());
   xmlFile.WriteAttr(wxT("isSelected"), this->GetSelected());
}

// Return true iff the attribute is recognized.
bool Track::HandleCommonXMLAttribute(const wxChar *attr, const wxChar *value)
{
   long nValue = -1;
   wxString strValue( value );
   if (!wxStrcmp(attr, wxT("name")) &&
      XMLValueChecker::IsGoodString(strValue)) {
      SetName( strValue );
      return true;
   }
   else if (!wxStrcmp(attr, wxT("height")) &&
         XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      SetHeight(nValue);
      return true;
   }
   else if (!wxStrcmp(attr, wxT("minimized")) &&
         XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      SetMinimized(nValue != 0);
      return true;
   }
   else if (!wxStrcmp(attr, wxT("isSelected")) &&
         XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) {
      this->SetSelected(nValue != 0);
      return true;
   }
   return false;
}

bool TrackList::HasPendingTracks() const
{
   if ( !mPendingUpdates.empty() )
      return true;
   if (end() != std::find_if(begin(), end(), [](const Track *t){
      return t->GetId() == TrackId{};
   }))
      return true;
   return false;
}

#include "AudioIO.h"
TransportTracks GetAllPlaybackTracks(TrackList &trackList, bool selectedOnly, bool useMidi)
{
   TransportTracks result;
   result.playbackTracks = trackList.GetWaveTrackArray(selectedOnly);
#ifdef EXPERIMENTAL_MIDI_OUT
   if (useMidi)
      result.midiTracks = trackList.GetNoteTrackConstArray(selectedOnly);
#else
   WXUNUSED(useMidi);
#endif
   return result;
}

#include "ViewInfo.h"
static auto TrackFactoryFactory = []( AudacityProject &project ) {
   auto &dirManager = DirManager::Get( project );
   auto &viewInfo = ViewInfo::Get( project );
   return std::make_shared< TrackFactory >(
      dirManager.shared_from_this(), &viewInfo );
};

static const AudacityProject::AttachedObjects::RegisteredFactory key2{
   TrackFactoryFactory
};

TrackFactory &TrackFactory::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< TrackFactory >( key2 );
}

const TrackFactory &TrackFactory::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

TrackFactory &TrackFactory::Reset( AudacityProject &project )
{
   auto result = TrackFactoryFactory( project );
   project.AttachedObjects::Assign( key2, std::move( result ) );
   return *result;
}

void TrackFactory::Destroy( AudacityProject &project )
{
   project.AttachedObjects::Assign( key2, nullptr );
}
