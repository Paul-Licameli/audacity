/**********************************************************************

Audacity: A Digital Audio Editor

EnvelopeHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_ENVELOPE_HANDLE__
#define __AUDACITY_ENVELOPE_HANDLE__

#include "../../UIHandle.h"

#include <wx/gdicmn.h>

class Envelope;
struct HitTestResult;
class ViewInfo;
class WaveTrack;

class EnvelopeHandle : public UIHandle
{
   EnvelopeHandle();
   EnvelopeHandle(const EnvelopeHandle&);
   EnvelopeHandle &operator=(const EnvelopeHandle&);
   static EnvelopeHandle& Instance();
   static HitTestPreview HitPreview(const AudacityProject *pProject, bool unsafe);

public:
   static HitTestResult HitAnywhere(const AudacityProject *pProject);
   static HitTestResult WaveTrackHitTest
      (const wxMouseEvent &event, const wxRect &rect,
       const AudacityProject *pProject, Cell *pCell);

   virtual ~EnvelopeHandle();

   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual HitTestPreview Preview
      (const TrackPanelMouseEvent &event, const AudacityProject *pProject);

   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
      wxWindow *pParent);

   virtual Result Cancel(AudacityProject *pProject);

private:
   bool ForwardEventToEnvelopes
      (const wxMouseEvent &event, const ViewInfo &viewInfo);

   Envelope *mClickedEnvelope;
   WaveTrack *mPartner;
   wxRect mRect;
   bool mLog;
   float mLower, mUpper;
   double mdBRange;
};

#endif
