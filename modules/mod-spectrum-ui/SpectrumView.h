/**********************************************************************

Audacity: A Digital Audio Editor

SpectrumView.h

Paul Licameli split from WaveTrackView.h

**********************************************************************/

#ifndef __AUDACITY_SPECTRUM_VIEW__
#define __AUDACITY_SPECTRUM_VIEW__

#include "WaveTrackView.h" // to inherit

class WaveTrack;

class SpectrumView final : public WaveTrackSubView
{
   SpectrumView( const SpectrumView& ) = delete;
   SpectrumView &operator=( const SpectrumView& ) = delete;

public:
   using WaveTrackSubView::WaveTrackSubView;
   ~SpectrumView() override;

   const Type &SubViewType() const override;

private:
   // TrackPanelDrawable implementation
   void Draw(
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass ) override;

   static void DoDraw( TrackPanelDrawingContext &context,
      const WaveTrack *track,
      const WaveClip* selectedClip,
      const wxRect & rect );

   std::vector<UIHandlePtr> DetailedHitTest(
      const TrackPanelMouseState &state,
      const AudacityProject *pProject, int currentTool, bool bMultiTool )
      override;

   UIHandlePtr SelectionHitTest(
       std::weak_ptr<UIHandle> &mSelectHandle,
       const TrackPanelMouseState &state, const AudacityProject *pProject)
      override;

protected:
   void DoSetMinimized( bool minimized ) override;
};

#endif
