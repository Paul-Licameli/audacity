/**********************************************************************

Audacity: A Digital Audio Editor

ProjectSelectionManager.cpp

Paul Licameli split from ProjectManager.cpp

**********************************************************************/

#include "ProjectSelectionManager.h"



#include "Project.h"
#include "ProjectHistory.h"
#include "ProjectWindows.h"
#include "ProjectRate.h"
#include "ProjectSettings.h"
#include "ProjectWindow.h"
#include "Snap.h"
#include "ViewInfo.h"
#include "WaveTrack.h"

static AudacityProject::AttachedObjects::RegisteredFactory
sProjectSelectionManagerKey {
   []( AudacityProject &project ) {
      return std::make_shared< ProjectSelectionManager >( project );
   }
};

ProjectSelectionManager &ProjectSelectionManager::Get(
   AudacityProject &project )
{
   return project.AttachedObjects::Get< ProjectSelectionManager >(
      sProjectSelectionManagerKey );
}

const ProjectSelectionManager &ProjectSelectionManager::Get(
   const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

ProjectSelectionManager::ProjectSelectionManager( AudacityProject &project )
   : mProject{ project }
{
   mProject.Bind( EVT_PROJECT_SETTINGS_CHANGE,
        &ProjectSelectionManager::OnSettingsChanged, this );
}

ProjectSelectionManager::~ProjectSelectionManager() = default;

bool ProjectSelectionManager::SnapSelection()
{
   auto &project = mProject;
   auto &settings = ProjectSettings::Get( project );
   auto &window = ProjectWindow::Get( project );
   auto snapTo = settings.GetSnapTo();
   if (snapTo != SNAP_OFF) {
      auto &viewInfo = ViewInfo::Get( project );
      auto &selectedRegion = viewInfo.selectedRegion;
      NumericConverter nc(NumericConverter::TIME,
         settings.GetSelectionFormat(), 0, ProjectRate::Get(project).GetRate());
      const bool nearest = (snapTo == SNAP_NEAREST);

      const double oldt0 = selectedRegion.t0();
      const double oldt1 = selectedRegion.t1();

      nc.ValueToControls(oldt0, nearest);
      nc.ControlsToValue();
      const double t0 = nc.GetValue();

      nc.ValueToControls(oldt1, nearest);
      nc.ControlsToValue();
      const double t1 = nc.GetValue();

      if (t0 != oldt0 || t1 != oldt1) {
         selectedRegion.setTimes(t0, t1);
         return true;
      }
   }

   return false;
}

void ProjectSelectionManager::OnSettingsChanged(wxCommandEvent &evt)
{
   evt.Skip();
   auto &settings = ProjectSettings::Get(mProject);
   switch (evt.GetInt()) {
   case ProjectSettings::ChangedSnapTo:
      return AS_SetSnapTo( settings.GetSnapTo() );
   case ProjectSettings::ChangedSelectionFormat:
      return AS_SetSelectionFormat( settings.GetSelectionFormat() );
   case ProjectSettings::ChangedAudioTimeFormat:
      return TT_SetAudioTimeFormat( settings.GetAudioTimeFormat() );
   case ProjectSettings::ChangedFrequencyFormat:
      return SSBL_SetFrequencySelectionFormatName(
         settings.GetFrequencySelectionFormatName() );
   case ProjectSettings::ChangedBandwidthFormat:
      return SSBL_SetBandwidthSelectionFormatName(
         settings.GetBandwidthSelectionFormatName() );
   default:
      break;
   }
}

void ProjectSelectionManager::AS_SetSnapTo(int snap)
{
   CallAfter([this, snap]{
      auto &window = ProjectWindow::Get( mProject );

   // LLL: TODO - what should this be changed to???
   // GetCommandManager()->Check(wxT("Snap"), mSnapTo);
      gPrefs->Write(wxT("/SnapTo"), snap);
      gPrefs->Flush();

      SnapSelection();

      window.RedrawProject();
   });
}

void ProjectSelectionManager::AS_SetSelectionFormat(
   const NumericFormatSymbol & format)
{
   CallAfter([this, format]{
      gPrefs->Write(wxT("/SelectionFormat"), format.Internal());
      gPrefs->Flush();

      if (SnapSelection())
         GetProjectPanel( mProject ).Refresh(false);
   });
}

void ProjectSelectionManager::TT_SetAudioTimeFormat(
   const NumericFormatSymbol & format)
{
   gPrefs->Write(wxT("/AudioTimeFormat"), format.Internal());
   gPrefs->Flush();
}

void ProjectSelectionManager::AS_ModifySelection(
   double &start, double &end, bool done)
{
   auto &project = mProject;
   auto &history = ProjectHistory::Get( project );
   auto &trackPanel = GetProjectPanel( project );
   auto &viewInfo = ViewInfo::Get( project );
   viewInfo.selectedRegion.setTimes(start, end);
   trackPanel.Refresh(false);
   if (done) {
      history.ModifyState(false);
   }
}

void ProjectSelectionManager::SSBL_SetFrequencySelectionFormatName(
   const NumericFormatSymbol & formatName)
{
   gPrefs->Write(wxT("/FrequencySelectionFormatName"),
                 formatName.Internal());
   gPrefs->Flush();
}

void ProjectSelectionManager::SSBL_SetBandwidthSelectionFormatName(
   const NumericFormatSymbol & formatName)
{
   gPrefs->Write(wxT("/BandwidthSelectionFormatName"),
      formatName.Internal());
   gPrefs->Flush();
}

void ProjectSelectionManager::SSBL_ModifySpectralSelection(
   double &bottom, double &top, bool done)
{
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   auto &project = mProject;
   auto &history = ProjectHistory::Get( project );
   auto &trackPanel = GetProjectPanel( project );
   auto &viewInfo = ViewInfo::Get( project );

   auto &tracks = TrackList::Get(mProject);
      auto nyq = std::max( ProjectRate::Get(project).GetRate(),
         tracks.Any<const WaveTrack>().max( &WaveTrack::GetRate ) )
         / 2.0;
   if (bottom >= 0.0)
      bottom = std::min(nyq, bottom);
   if (top >= 0.0)
      top = std::min(nyq, top);
   viewInfo.selectedRegion.setFrequencies(bottom, top);
   trackPanel.Refresh(false);
   if (done) {
      history.ModifyState(false);
   }
#else
   bottom; top; done;
#endif
}
