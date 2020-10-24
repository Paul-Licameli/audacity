/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SpectralSelectHandle.h
 
 Paul Licameli split from SelectHandle.h
 
 **********************************************************************/

#ifndef __AUDACITY__SPECTRAL_SELECT_HANDLE__
#define __AUDACITY__SPECTRAL_SELECT_HANDLE__

#include "SelectHandle.h" // to inherit

class SpectralSelectHandle final : public SelectHandle
{
public:
   // This is needed to implement a command assignable to keystrokes
   static void SnapCenterOnce
      (SpectrumAnalyst &analyst,
       ViewInfo &viewInfo, const WaveTrack *pTrack, bool up);

   using SelectHandle::SelectHandle;
   ~SpectralSelectHandle() override;

   void DoDrag( AudacityProject &project,
      ViewInfo &viewInfo,
      TrackView &view, Track &clickedTrack, Track &track,
      wxCoord x, wxCoord y, bool controlDown) override;

   UIHandle::Result Release(
      const TrackPanelMouseEvent &evt, AudacityProject *pProject,
      wxWindow *pWindow) override;
};

#endif
