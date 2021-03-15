/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackVRulerControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_VRULER_CONTROLS__
#define __AUDACITY_WAVE_TRACK_VRULER_CONTROLS__

#include "TrackVRulerControls.h"

class Ruler;
class WaveTrack;

namespace WaveTrackVRulerControls
{
   WAVE_TRACK_UI_API Ruler &ScratchRuler();

   WAVE_TRACK_UI_API void DoDraw( TrackVRulerControls &controls,
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass );
};

#endif
