/**********************************************************************

  Audacity: A Digital Audio Editor

  @file TransportUtilities.cpp
  @brief implements some UI related to starting and stopping play and record

  Paul Licameli split from ProjectAudioManager.cpp

**********************************************************************/

#include "TransportUtilities.h"

#include "AudioIO.h"
#include "CommandContext.h"
#include "Project.h"
#include "ProjectAudioIO.h"
#include "ProjectAudioManager.h"
#include "ViewInfo.h"
#include "widgets/ProgressDialog.h"

void TransportUtilities::PlayCurrentRegionAndWait(
   const CommandContext &context,
   bool looped,
   bool cutpreview)
{
   auto &project = context.project;
   auto &projectAudioManager = ProjectAudioManager::Get(project);

   const auto &playRegion = ViewInfo::Get(project).playRegion;
   double t0 = playRegion.GetStart();
   double t1 = playRegion.GetEnd();

   projectAudioManager.PlayCurrentRegion(looped, cutpreview);

   if (project.mBatchMode > 0 && t0 != t1 && !looped) {
      wxYieldIfNeeded();

      /* i18n-hint: This title appears on a dialog that indicates the progress
         in doing something.*/
      ProgressDialog progress(XO("Progress"), XO("Playing"), pdlgHideCancelButton);
      auto gAudioIO = AudioIO::Get();

      while (projectAudioManager.Playing()) {
         ProgressResult result = progress.Update(gAudioIO->GetStreamTime() - t0, t1 - t0);
         if (result != ProgressResult::Success) {
            projectAudioManager.Stop();
            if (result != ProgressResult::Stopped) {
               context.Error(wxT("Playing interrupted"));
            }
            break;
         }

         wxMilliSleep(100);
         wxYieldIfNeeded();
      }

      projectAudioManager.Stop();
      wxYieldIfNeeded();
   }
}

void TransportUtilities::PlayPlayRegionAndWait(
   const CommandContext &context,
   const SelectedRegion &selectedRegion,
   const AudioIOStartStreamOptions &options,
   PlayMode mode)
{
   auto &project = context.project;
   auto &projectAudioManager = ProjectAudioManager::Get(project);

   double t0 = selectedRegion.t0();
   double t1 = selectedRegion.t1();

   projectAudioManager.PlayPlayRegion(selectedRegion, options, mode);

   if (project.mBatchMode > 0) {
      wxYieldIfNeeded();

      /* i18n-hint: This title appears on a dialog that indicates the progress
         in doing something.*/
      ProgressDialog progress(XO("Progress"), XO("Playing"), pdlgHideCancelButton);
      auto gAudioIO = AudioIO::Get();

      while (projectAudioManager.Playing()) {
         ProgressResult result = progress.Update(gAudioIO->GetStreamTime() - t0, t1 - t0);
         if (result != ProgressResult::Success) {
            projectAudioManager.Stop();
            if (result != ProgressResult::Stopped) {
               context.Error(wxT("Playing interrupted"));
            }
            break;
         }

         wxMilliSleep(100);
         wxYieldIfNeeded();
      }

      projectAudioManager.Stop();
      wxYieldIfNeeded();
   }
}

void TransportUtilities::DoStartPlaying(
   const CommandContext &context, bool looping)
{
   auto &project = context.project;
   auto &projectAudioManager = ProjectAudioManager::Get(project);
   auto gAudioIO = AudioIOBase::Get();
   //play the front project
   if (!gAudioIO->IsBusy()) {
      //Otherwise, start playing (assuming audio I/O isn't busy)

      // Will automatically set mLastPlayMode
      TransportUtilities::PlayCurrentRegionAndWait(context, looping);
   }
}

// Returns true if this project was stopped, otherwise false.
// (it may though have stopped another project playing)
bool TransportUtilities::DoStopPlaying(const CommandContext &context)
{
   auto &project = context.project;
   auto &projectAudioManager = ProjectAudioManager::Get(project);
   auto gAudioIO = AudioIOBase::Get();
   auto token = ProjectAudioIO::Get(project).GetAudioIOToken();

   //If this project is playing, stop playing, make sure everything is unpaused.
   if (gAudioIO->IsStreamActive(token)) {
      projectAudioManager.Stop();
      // Playing project was stopped.  All done.
      return true;
   }

   // This project isn't playing.
   // If some other project is playing, stop playing it
   if (gAudioIO->IsStreamActive()) {

      //find out which project we need;
      auto start = AllProjects{}.begin(), finish = AllProjects{}.end(),
         iter = std::find_if(start, finish,
            [&](const AllProjects::value_type &ptr) {
         return gAudioIO->IsStreamActive(
            ProjectAudioIO::Get(*ptr).GetAudioIOToken()); });

      //stop playing the other project
      if (iter != finish) {
         auto otherProject = *iter;
         auto &otherProjectAudioManager =
            ProjectAudioManager::Get(*otherProject);
         otherProjectAudioManager.Stop();
      }
   }
   return false;
}
