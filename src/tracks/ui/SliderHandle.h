/**********************************************************************

Audacity: A Digital Audio Editor

SliderHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_SLIDER_HANDLE__
#define __AUDACITY_SLIDER_HANDLE__

#include "../../MemoryX.h"
#include "../../UIHandle.h"
#include <wx/gdicmn.h>

class wxMouseEvent;
class wxMouseState;
class LWSlider;
class Track;

class SliderHandle /* not final */ : public UIHandle
{
   SliderHandle(const SliderHandle&) = delete;

public:
   using SliderFn = LWSlider *(*)( AudacityProject*, const wxRect&, Track* );

   explicit SliderHandle
      ( SliderFn sliderFn, const wxRect &rect,
        const std::shared_ptr<Track> &pTrack );

   SliderHandle &operator=(const SliderHandle&) = default;

   std::shared_ptr<Track> GetTrack() const { return mpTrack.lock(); }
   bool IsClicked() const { return mIsClicked; }

protected:
   virtual ~SliderHandle();

   // These new abstract virtuals simplify the duties of further subclasses.
   // This class will decide whether to refresh the clicked cell for slider state
   // change.
   // Subclass can decide to refresh other things in SetValue or CommitChanges,
   // and the results will be ORed.

   // display is true when formatting the value for the user; else, it must be
   // such as causes no change when passed back to SetValue.
   virtual float GetValue(bool display) = 0;
   virtual Result SetValue(AudacityProject *pProject, float newValue) = 0;
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject) = 0;

   // Return a printf-like format string with one slot for the value
   virtual wxString TipTemplate() const = 0;

   void Enter(bool forward) override;

   Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject)
      final override;

   Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject)
      final override;

   HitTestPreview Preview
      (const TrackPanelMouseState &state, const AudacityProject *pProject)
      final override;

   Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent) final override;

   Result Cancel(AudacityProject *pProject) final override;

   // Derived class is expected to set these two before Click():
   std::weak_ptr<Track> mpTrack;
   wxRect mRect{};
   SliderFn mSliderFn;
   LWSlider *GetSlider( AudacityProject *pProject );

   float mStartingValue {};

   bool mIsClicked{};
};

#endif
