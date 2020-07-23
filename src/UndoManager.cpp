/**********************************************************************

  Audacity: A Digital Audio Editor

  UndoManager.cpp

  Dominic Mazzoni

*******************************************************************//**

\class UndoManager
\brief Works with HistoryDialog to provide the Undo functionality.

*//****************************************************************//**

\class UndoStackElem
\brief Holds one item with description and time range for the
UndoManager

*//*******************************************************************/


#include "Audacity.h"
#include "UndoManager.h"

#include <algorithm>
#include <wx/hashset.h>

#include "Clipboard.h"
#include "Diags.h"
#include "Project.h"
#include "ProjectFileIO.h"
#include "SampleBlock.h"
#include "Sequence.h"
#include "WaveClip.h"
#include "WaveTrack.h"          // temp
#include "NoteTrack.h"  // for Sonify* function declarations
#include "Diags.h"
#include "Tags.h"


#include <unordered_set>

wxDEFINE_EVENT(EVT_UNDO_PUSHED, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNDO_MODIFIED, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNDO_OR_REDO, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNDO_RESET, wxCommandEvent);

using SampleBlockID = long long;

struct UndoStackElem {

   UndoStackElem(std::shared_ptr<TrackList> &&tracks_,
      const TranslatableString &description_,
      const TranslatableString &shortDescription_,
      const SelectedRegion &selectedRegion_,
      const std::shared_ptr<Tags> &tags_)
      : state(std::move(tracks_), tags_, selectedRegion_)
      , description(description_)
      , shortDescription(shortDescription_)
   {
   }

   UndoState state;
   TranslatableString description;
   TranslatableString shortDescription;
};

static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject &project)
      { return std::make_unique<UndoManager>( project ); }
};

UndoManager &UndoManager::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< UndoManager >( key );
}

const UndoManager &UndoManager::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

UndoManager::UndoManager( AudacityProject &project )
   : mProject{ project }
{
   current = -1;
   saved = -1;
}

UndoManager::~UndoManager()
{
}

namespace {
   SpaceArray::value_type
   CalculateUsage(const TrackList &tracks, SampleBlockIDSet &seen)
   {
      SpaceArray::value_type result = 0;
      //TIMER_START( "CalculateSpaceUsage", space_calc );
      InspectBlocks(
         tracks,
         BlockSpaceUsageAccumulator( result ),
         &seen
      );
      return result;
   }
}

void UndoManager::CalculateSpaceUsage()
{
   space.clear();
   space.resize(stack.size(), 0);

   SampleBlockIDSet seen;

   // After copies and pastes, a block file may be used in more than
   // one place in one undo history state, and it may be used in more than
   // one undo history state.  It might even be used in two states, but not
   // in another state that is between them -- as when you have state A,
   // then make a cut to get state B, but then paste it back into state C.

   // So be sure to count each block file once only, in the last undo item that
   // contains it.

   // Why the last and not the first? Because the user of the History dialog
   // may DELETE undo states, oldest first.  To reclaim disk space you must
   // DELETE all states containing the block file.  So the block file's
   // contribution to space usage should be counted only in that latest state.

   for (size_t nn = stack.size(); nn--;)
   {
      // Scan all tracks at current level
      auto &tracks = *stack[nn]->state.tracks;
      space[nn] = CalculateUsage(tracks, seen);
   }

   // Count the usage of the clipboard separately, using another set.  Do not
   // multiple-count any block occurring multiple times within the clipboard.
   seen.clear();
   mClipboardSpaceUsage = CalculateUsage(
      Clipboard::Get().GetTracks(), seen);

   //TIMER_STOP( space_calc );
}

wxLongLong_t UndoManager::GetLongDescription(
   unsigned int n, TranslatableString *desc, TranslatableString *size)
{
   wxASSERT(n < stack.size());
   wxASSERT(space.size() == stack.size());

   *desc = stack[n]->description;

   *size = Internat::FormatSize(space[n]);

   return space[n];
}

void UndoManager::GetShortDescription(unsigned int n, TranslatableString *desc)
{
   wxASSERT(n < stack.size());

   *desc = stack[n]->shortDescription;
}

void UndoManager::SetLongDescription(
  unsigned int n, const TranslatableString &desc)
{
   n -= 1;

   wxASSERT(n < stack.size());

   stack[n]->description = desc;
}

void UndoManager::RemoveStateAt(int n)
{
   stack.erase(stack.begin() + n);
}

void UndoManager::DeleteBlocksExcept(
   UndoStack::const_iterator begin,
   UndoStack::const_iterator end )
{
   auto &projectFileIO = ProjectFileIO::Get(mProject);

   // Collect ids that must not be deleted
   SampleBlockIDSet ids;
   std::for_each( begin, end, [&ids](const auto &pElem){
      const auto &tracks = *pElem->state.tracks;
      InspectBlocks(tracks, {}, &ids);
   });

   // Delete all others
   bool ok = projectFileIO.DeleteBlocks(ids, true);
   if ( !ok ) {
      // Ignore the database error.  At worst, we make orphans.
   }
}

void UndoManager::RemoveNewStates( int num )
{
   const auto begin = stack.begin(), end = begin + num;
   DeleteBlocksExcept( begin, end );

   // Destroy the tracks (which should dereference SampleBlocks that were
   // deleted from the database)
   while (num < stack.size()) {
      RemoveStateAt(num);
   }
}

void UndoManager::RemoveOldStates(int num)
{
   const auto begin = stack.begin() + num, end = stack.end();
   DeleteBlocksExcept( begin, end );

   // Destroy the tracks (which should dereference SampleBlocks that were
   // deleted from the database)
   for (int i = 0; i < num; i++) {
      RemoveStateAt(0);

      current -= 1;
      saved -= 1;
   }
}

void UndoManager::ClearStates()
{
   RemoveOldStates(stack.size());
   current = -1;
   saved = -1;
}

unsigned int UndoManager::GetNumStates()
{
   return stack.size();
}

unsigned int UndoManager::GetCurrentState()
{
   return current;
}

bool UndoManager::UndoAvailable()
{
   return (current > 0);
}

bool UndoManager::RedoAvailable()
{
   return (current < (int)stack.size() - 1);
}

void UndoManager::ModifyState(const TrackList * l,
                              const SelectedRegion &selectedRegion,
                              const std::shared_ptr<Tags> &tags)
{
   if (current == wxNOT_FOUND) {
      return;
   }

   SonifyBeginModifyState();
   // Delete current -- not necessary, but let's reclaim space early
   stack[current]->state.tracks.reset();

   // Duplicate
   auto tracksCopy = TrackList::Create( nullptr );
   for (auto t : *l) {
      if ( t->GetId() == TrackId{} )
         // Don't copy a pending added track
         continue;
      tracksCopy->Add(t->Duplicate());
   }

   // Replace
   stack[current]->state.tracks = std::move(tracksCopy);
   stack[current]->state.tags = tags;

   stack[current]->state.selectedRegion = selectedRegion;
   SonifyEndModifyState();

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_MODIFIED } );
}

void UndoManager::PushState(const TrackList * l,
                            const SelectedRegion &selectedRegion,
                            const std::shared_ptr<Tags> &tags,
                            const TranslatableString &longDescription,
                            const TranslatableString &shortDescription,
                            UndoPush flags)
{
   unsigned int i;

   if ( (flags & UndoPush::CONSOLIDATE) != UndoPush::NONE &&
       // compare full translations not msgids!
       lastAction.Translation() == longDescription.Translation() &&
       mayConsolidate ) {
      ModifyState(l, selectedRegion, tags);
      // MB: If the "saved" state was modified by ModifyState, reset
      //  it so that UnsavedChanges returns true.
      if (current == saved) {
         saved = -1;
      }
      return;
   }

   auto tracksCopy = TrackList::Create( nullptr );
   for (auto t : *l) {
      if ( t->GetId() == TrackId{} )
         // Don't copy a pending added track
         continue;
      tracksCopy->Add(t->Duplicate());
   }

   mayConsolidate = true;

   RemoveNewStates( current + 1 );

   // Assume tags was duplicated before any changes.
   // Just save a NEW shared_ptr to it.
   stack.push_back(
      std::make_unique<UndoStackElem>
         (std::move(tracksCopy),
            longDescription, shortDescription, selectedRegion, tags)
   );

   current++;

   if (saved >= current) {
      saved = -1;
   }

   lastAction = longDescription;

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_PUSHED } );
}

void UndoManager::SetStateTo(unsigned int n, const Consumer &consumer)
{
   wxASSERT(n < stack.size());

   current = n;

   lastAction = {};
   mayConsolidate = false;

   consumer( stack[current]->state );

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_RESET } );
}

void UndoManager::Undo(const Consumer &consumer)
{
   wxASSERT(UndoAvailable());

   current--;

   lastAction = {};
   mayConsolidate = false;

   consumer( stack[current]->state );

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_OR_REDO } );
}

void UndoManager::Redo(const Consumer &consumer)
{
   wxASSERT(RedoAvailable());

   current++;

   /*
   if (!RedoAvailable()) {
      *sel0 = stack[current]->sel0;
      *sel1 = stack[current]->sel1;
   }
   else {
      current++;
      *sel0 = stack[current]->sel0;
      *sel1 = stack[current]->sel1;
      current--;
   }
   */

   lastAction = {};
   mayConsolidate = false;

   consumer( stack[current]->state );

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_OR_REDO } );
}

bool UndoManager::UnsavedChanges() const
{
   return (saved != current);
}

void UndoManager::StateSaved()
{
   saved = current;
}

// currently unused
//void UndoManager::Debug()
//{
//   for (unsigned int i = 0; i < stack.size(); i++) {
//      for (auto t : stack[i]->tracks->Any())
//         wxPrintf(wxT("*%d* %s %f\n"),
//                  i, (i == (unsigned int)current) ? wxT("-->") : wxT("   "),
//                t ? t->GetEndTime()-t->GetStartTime() : 0);
//   }
//}

