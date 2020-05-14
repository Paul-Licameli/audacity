/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2010 Audacity Team
   File License: wxWidgets

   Michael Chinen

******************************************************************//**

\file ODManager.cpp
\brief Singleton ODManager class.  Is the bridge between client side
ODTask requests and internals.

*//*******************************************************************/

#include "../Audacity.h"
#include "ODManager.h"

#include "ODTask.h"
#include "ODWaveTrackTaskQueue.h"
#include "../Project.h"
#include <wx/utils.h>
#include <wx/wx.h>
#include <wx/event.h>
#include <thread>

static std::atomic<bool> gManagerCreated{ false };
std::atomic< bool > gPause{ false }; //to be loaded in and used with Pause/Resume before ODMan init.
/// a flag that is set if we have loaded some OD blockfiles from PCM.
static bool sHasLoadedOD=false;

std::unique_ptr<ODManager> ODManager::pMan;

ODManager *ODManager::Instance()
{
   static std::once_flag flag;
   std::call_once( flag, []{
      pMan.reset( safenew ODManager );
      pMan->Init();
      gManagerCreated.store( true, std::memory_order_release );
   } );
   return pMan.get();
}

wxDEFINE_EVENT(EVT_ODTASK_UPDATE, wxCommandEvent);

//using this with wxStringArray::Sort will give you a list that
//is alphabetical, without depending on case.  If you use the
//default sort, you will get strings with 'R' before 'a', because it is in caps.
int CompareNoCaseFileName(const wxString& first, const wxString& second)
{
   return first.CmpNoCase(second);
}

//private constructor - Singleton.
ODManager::ODManager()
{
   mTerminate = false;
   mTerminated = false;

   //must set up the queue condition
   mQueueNotEmptyCond = std::make_unique<ODCondition>(&mQueueNotEmptyCondLock);
}

//private destructor - DELETE with static method Quit()
ODManager::~ODManager()
{
   mTerminateMutex.Lock();
   mTerminate = true;
   mTerminateMutex.Unlock();

   //This while loop waits for ODTasks to finish and the DELETE removes all tasks from the Queue.
   //This function is called from the main audacity event thread, so there should not be more requests for pMan
   mTerminatedMutex.Lock();
   while (!mTerminated)
   {
      using namespace std::chrono;
      mTerminatedMutex.Unlock();
      std::this_thread::sleep_for( 200ms );

      //signal the queue not empty condition since the ODMan thread will wait on the queue condition
      mQueueNotEmptyCondLock.Lock();
      mQueueNotEmptyCond->Signal();
      mQueueNotEmptyCondLock.Unlock();

      mTerminatedMutex.Lock();
   }
   mTerminatedMutex.Unlock();

   //get rid of all the queues.  The queues get rid of the tasks, so we don't worry about them.
   //nothing else should be running on OD related threads at this point, so we don't lock.
   mQueues.clear();
}

///Adds a task to running queue.  Thread-safe.
void ODManager::AddTask(ODTask* task)
{
   mTasksMutex.Lock();
   mTasks.push_back(task);
   mTasksMutex.Unlock();
   //signal the queue not empty condition.
   bool paused = gPause.load( std::memory_order_acquire );

   //don't signal if we are paused since if we wake up the loop it will start processing other tasks while paused
   if(!paused)
      mQueueNotEmptyCond->Signal();
}

void ODManager::SignalTaskQueueLoop()
{
   bool paused = gPause.load( std::memory_order_acquire );
   //don't signal if we are paused
   if(!paused)
      mQueueNotEmptyCond->Signal();
}

///removes a task from the active task queue
void ODManager::RemoveTaskIfInQueue(ODTask* task)
{
   mTasksMutex.Lock();
   //linear search okay for now, (probably only 1-5 tasks exist at a time.)
   for(unsigned int i=0;i<mTasks.size();i++)
   {
      if(mTasks[i]==task)
      {
         mTasks.erase(mTasks.begin()+i);
         break;
      }
   }
   mTasksMutex.Unlock();

}

///Adds a NEW task to the queue.  Creates a queue if the tracks associated with the task is not in the list
///
///@param task the task to add
///@param lockMutex locks the mutexes if true (default).  This function is used within other ODManager calls, which many need to set this to false.
void ODManager::AddNewTask(std::unique_ptr<ODTask> &&mtask, bool lockMutex)
{
   auto task = mtask.get();
   ODWaveTrackTaskQueue* queue = NULL;

   if(lockMutex)
      mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      //search for a task containing the lead track.  wavetrack removal is threadsafe and bound to the mQueuesMutex
      //note that GetWaveTrack is not threadsafe, but we are assuming task is not running on a different thread yet.
      if(mQueues[i]->ContainsWaveTrack(task->GetWaveTrack(0).get()))
         queue = mQueues[i].get();
   }

   if(queue)
   {
      //Add it to the existing queue but keep the lock since this reference can be deleted.
      queue->AddTask(std::move(mtask));
      if(lockMutex)
         mQueuesMutex.Unlock();
   }
   else
   {
      //Make a NEW one, add it to the local track queue, and to the immediate running task list,
      //since this task is definitely at the head
      auto newqueue = std::make_unique<ODWaveTrackTaskQueue>();
      newqueue->AddTask(std::move(mtask));
      mQueues.push_back(std::move(newqueue));
      if(lockMutex)
         mQueuesMutex.Unlock();

      AddTask(task);
   }
}

bool ODManager::IsInstanceCreated()
{
   return gManagerCreated.load( std::memory_order_acquire );
}

///Launches a thread for the manager and starts accepting Tasks.
void ODManager::Init()
{
   mCurrentThreads = 0;
   mMaxThreads = 5;

   //   wxLogDebug(wxT("Initializing ODManager...Creating manager thread"));

//   startThread->SetPriority(0);//default of 50.
//   wxPrintf("starting thread from init\n");

   std::thread{ [this]{ DispatchLoop(); } }.detach();

//   wxPrintf("started thread from init\n");
   //destruction of thread is taken care of by thread library
}

void ODManager::DecrementCurrentThreads()
{
   mCurrentThreads.fetch_sub( 1, std::memory_order_release );
}

///Main loop for managing threads and tasks.
/// Runs in its own thread, which spawns other worker threads.
void ODManager::DispatchLoop()
{
   bool tasksInArray;
   bool paused;
   int  numQueues=0;

   int mNeedsDraw=0;

   //wxLog calls not threadsafe.  are printfs?  thread-messy for sure, but safe?
//   wxPrintf("ODManager thread strating \n");
   //TODO: Figure out why this has no effect at all.
   //wxThread::This()->SetPriority(30);
   mTerminateMutex.Lock();
   while(!mTerminate)
   {
      mTerminateMutex.Unlock();
//    wxPrintf("ODManager thread running \n");

      //we should look at our WaveTrack queues to see if we can process a NEW task to the running queue.
      UpdateQueues();

      //start some threads if necessary

      mTasksMutex.Lock();
      tasksInArray = mTasks.size()>0;
      mTasksMutex.Unlock();

      bool paused = gPause.load( std::memory_order_acquire );

      // keep adding tasks if there is work to do, up to the limit.
      while(!paused && tasksInArray &&
            (mCurrentThreads.load( std::memory_order_acquire ) < mMaxThreads))
      {
         mCurrentThreads.fetch_add( 1, std::memory_order_relaxed );

         mTasksMutex.Lock();
         //detach a NEW thread.
         auto pTask = mTasks[0];
         std::thread{ [ this, pTask ]{
            //TODO: Figure out why this has no effect at all.
            //wxThread::This()->SetPriority( 40);
            //Do at least 5 percent of the task
            pTask->DoSome(0.05f);
            
            //release the thread count so that the dispatcher thread knows how
            //many active threads are alive.
            DecrementCurrentThreads();
         } }.detach();

         //thread->SetPriority(10);//default is 50.

         mTasks.erase(mTasks.begin());
         tasksInArray = mTasks.size()>0;
         mTasksMutex.Unlock();
      }

      //use a condition variable to block here instead of a sleep.

      // JKC: If there are no tasks ready to run, or we're paused then
      // we wait for there to be tasks in the queue.
      {
         ODLocker locker{ &mQueueNotEmptyCondLock };
         if( (!tasksInArray) || paused)
            mQueueNotEmptyCond->Wait();
      }

      //if there is some ODTask running, then there will be something in the queue.  If so then redraw to show progress
      mQueuesMutex.Lock();
      mNeedsDraw += mQueues.size()>0?1:0;
      numQueues=mQueues.size();
      mQueuesMutex.Unlock();

      //redraw the current project only (ODTasks will send a redraw on complete even if the projects are in the background)
      //we don't want to redraw at a faster rate when we have more queues because
      //this means the CPU is already taxed.  This if statement normalizes the rate
      if((mNeedsDraw>numQueues) && numQueues)
      {
         mNeedsDraw=0;
         wxCommandEvent event( EVT_ODTASK_UPDATE );
         wxTheApp->AddPendingEvent(event);
      }
      mTerminateMutex.Lock();
   }
   mTerminateMutex.Unlock();

   mTerminatedMutex.Lock();
   mTerminated=true;
   mTerminatedMutex.Unlock();

   //wxLogDebug Not thread safe.
   //wxPrintf("ODManager thread terminating\n");
}

//static function that prevents ODTasks from being scheduled
//does not stop currently running tasks from completing their immediate subtask,
//but presumably they will finish within a second
void ODManager::Pauser::Pause(bool pause)
{
   gPause.store( pause, std::memory_order_release );
   if(IsInstanceCreated())
   {
      if(!pause)
         //we should check the queue again.
         pMan->mQueueNotEmptyCond->Signal();
   }
}

void ODManager::Pauser::Resume()
{
   Pause(false);
}

void ODManager::Quit()
{
   if(IsInstanceCreated())
   {
      pMan.reset();
   }
}

///replace the wavetrack whose wavecache the gui watches for updates
void ODManager::ReplaceWaveTrack(Track *oldTrack,
   const std::shared_ptr<Track> &newTrack)
{
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      mQueues[i]->ReplaceWaveTrack( oldTrack, newTrack );
   }
   mQueuesMutex.Unlock();
}

///if it shares a queue/task, creates a NEW queue/task for the track, and removes it from any previously existing tasks.
void ODManager::MakeWaveTrackIndependent(
   const std::shared_ptr< WaveTrack > &track)
{
   ODWaveTrackTaskQueue* owner=NULL;
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      if(mQueues[i]->ContainsWaveTrack(track.get()))
      {
         owner = mQueues[i].get();
         break;
      }
   }
   if(owner)
      owner->MakeWaveTrackIndependent(track);

   mQueuesMutex.Unlock();
}

///attach the track in question to another, already existing track's queues and tasks.  Remove the task/tracks.
///only works if both tracks exist.  Sets needODUpdate flag for the task.  This is complicated and will probably need
///better design in the future.
///@return returns success.  Some ODTask conditions require that the tasks finish before merging.
///e.g. they have different effects being processed at the same time.
bool ODManager::MakeWaveTrackDependent(
   const std::shared_ptr< WaveTrack > &dependentTrack,
   WaveTrack* masterTrack
)
{
   //First, check to see if the task lists are mergeable.  If so, we can simply add this track to the other task and queue,
   //then DELETE this one.
   ODWaveTrackTaskQueue* masterQueue=NULL;
   ODWaveTrackTaskQueue* dependentQueue=NULL;
   unsigned int dependentIndex = 0;
   bool canMerge = false;

   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      if(mQueues[i]->ContainsWaveTrack(masterTrack))
      {
         masterQueue = mQueues[i].get();
      }
      else if(mQueues[i]->ContainsWaveTrack(dependentTrack.get()))
      {
         dependentQueue = mQueues[i].get();
         dependentIndex = i;
      }

   }
   if(masterQueue&&dependentQueue)
      canMerge=masterQueue->CanMergeWith(dependentQueue);

   //otherwise we need to let dependentTrack's queue live on.  We'll have to wait till the conflicting tasks are done.
   if(!canMerge)
   {
      mQueuesMutex.Unlock();
      return false;
   }
   //then we add dependentTrack to the masterTrack's queue - this will allow future ODScheduling to affect them together.
   //this sets the NeedODUpdateFlag since we don't want the head task to finish without haven't dealt with the dependent
   masterQueue->MergeWaveTrack(dependentTrack);

   //finally remove the dependent track
   mQueues.erase(mQueues.begin()+dependentIndex);
   mQueuesMutex.Unlock();
   return true;
}


///changes the tasks associated with this Waveform to process the task from a different point in the track
///@param track the track to update
///@param seconds the point in the track from which the tasks associated with track should begin processing from.
void ODManager::DemandTrackUpdate(WaveTrack* track, double seconds)
{
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      mQueues[i]->DemandTrackUpdate(track,seconds);
   }
   mQueuesMutex.Unlock();
}

///remove tasks from ODWaveTrackTaskQueues that have been done.  Schedules NEW ones if they exist
///Also remove queues that have become empty.
void ODManager::UpdateQueues()
{
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      if(mQueues[i]->IsFrontTaskComplete())
      {
         //this should DELETE and remove the front task instance.
         mQueues[i]->RemoveFrontTask();
         //schedule next.
         if(!mQueues[i]->IsEmpty())
         {
            //we need to release the lock on the queue vector before using the task vector's lock or we deadlock
            //so get a temp.
            ODWaveTrackTaskQueue* queue = mQueues[i].get();

            AddTask(queue->GetFrontTask());
         }
      }

      //if the queue is empty DELETE it.
      if(mQueues[i]->IsEmpty())
      {
         mQueues.erase(mQueues.begin()+i);
         i--;
      }
   }
   mQueuesMutex.Unlock();
}

//static
///sets a flag that is set if we have loaded some OD blockfiles from PCM.
void ODManager::MarkLoadedODFlag()
{
   sHasLoadedOD = true;
}

//static
///resets a flag that is set if we have loaded some OD blockfiles from PCM.
void ODManager::UnmarkLoadedODFlag()
{
   sHasLoadedOD = false;
}

//static
///returns a flag that is set if we have loaded some OD blockfiles from PCM.
bool ODManager::HasLoadedODFlag()
{
   return sHasLoadedOD;
}

///fills in the status bar message for a given track
void ODManager::FillTipForWaveTrack( const WaveTrack * t, TranslatableString &tip )
{
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      mQueues[i]->FillTipForWaveTrack(t, tip);
   }
   mQueuesMutex.Unlock();
}

///Gets the total percent complete for all tasks combined.
float ODManager::GetOverallPercentComplete()
{
   float total=0.0;
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      total+=mQueues[i]->GetFrontTask()->PercentComplete();
   }
   mQueuesMutex.Unlock();

   //avoid div by zero and be thread smart.
   int totalTasks = GetTotalNumTasks();
   return (float) total/(totalTasks>0?totalTasks:1);
}

///Get Total Number of Tasks.
int ODManager::GetTotalNumTasks()
{
   int ret=0;
   mQueuesMutex.Lock();
   for(unsigned int i=0;i<mQueues.size();i++)
   {
      ret+=mQueues[i]->GetNumTasks();
   }
   mQueuesMutex.Unlock();
   return ret;
}
