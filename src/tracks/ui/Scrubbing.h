/**********************************************************************

Audacity: A Digital Audio Editor

Scrubbing.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_SCRUBBING__
#define __AUDACITY_SCRUBBING__

#include <wx/event.h>
#include <wx/longlong.h>

#include "../../Experimental.h"
#include "../../TrackPanelOverlay.h"

class AudacityProject;

// Scrub state object
class Scrubber : public wxEvtHandler
{
public:
   Scrubber(AudacityProject *project);
   ~Scrubber();

   void MarkScrubStart(
      double time
#ifdef EXPERIMENTAL_SCRUBBING_SMOOTH_SCROLL
      , bool smoothScrolling
#endif
   );
   // Returns true iff the event should be considered consumed by this:
   bool MaybeStartScrubbing(const wxMouseEvent &event);
   void ContinueScrubbing();
   bool StopScrubbing();

   bool IsScrubbing() const;
   bool IsScrollScrubbing() const // If true, implies IsScrubbing()
   { return mSmoothScrollingScrub; }

   bool ShouldDrawScrubSpeed();
   double FindScrubSpeed(bool seeking, double time) const;
   double GetMaxScrubSpeed() const { return mMaxScrubSpeed; }

   void HandleScrollWheel(int steps);

   void SetSeeking() { mScrubSeekPress = true; }

private:
   int mScrubToken;
   wxLongLong mScrubStartClockTimeMillis;
   bool mScrubHasFocus;
   int mScrubSpeedDisplayCountdown;
   wxCoord mScrubStartPosition;
   double mMaxScrubSpeed;
   bool mScrubSeekPress;

#ifdef EXPERIMENTAL_SCRUBBING_SCROLL_WHEEL
   bool mSmoothScrollingScrub;
   int mLogMaxScrubSpeed;
#endif

private:
   void OnActivateOrDeactivateApp(wxActivateEvent & event);

   AudacityProject *mProject;
};

// Specialist in drawing the scrub speed, and listening for certain events
class ScrubbingOverlay : public wxEvtHandler, public TrackPanelOverlay
{
public:
   ScrubbingOverlay(AudacityProject *project);
   virtual ~ScrubbingOverlay();

private:
   virtual std::pair<wxRect, bool> GetRectangle(wxSize size);
   virtual void Draw
      (wxDC &dc, TrackPanelCellIterator begin, TrackPanelCellIterator end);

   void OnTimer(wxCommandEvent &event);

   const Scrubber &GetScrubber() const;
   Scrubber &GetScrubber();

   AudacityProject *mProject;

   wxRect mLastScrubRect, mNextScrubRect;
   wxString mLastScrubSpeedText, mNextScrubSpeedText;
};

#endif
