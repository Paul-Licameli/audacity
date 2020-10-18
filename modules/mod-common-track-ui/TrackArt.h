/**********************************************************************

  Audacity: A Digital Audio Editor

  @file TrackArt.h

  Paul Licameli split from TrackArtist.h

**********************************************************************/
#ifndef __AUDACITY_TRACK_ART__
#define __AUDACITY_TRACK_ART__

class Track;
struct TrackPanelDrawingContext;
class wxBrush;
class wxDC;
class wxRect;

namespace TrackArt {

   static constexpr int ClipFrameRadius{ 6 };

   AUDACITY_DLL_API
   void DrawClipAffordance(wxDC& dc, const wxRect& affordanceRect, bool highlight = false, bool selected = false);

   AUDACITY_DLL_API
   void DrawClipEdges(wxDC& dc, const wxRect& clipRect, bool selected = false);

   // Helper: draws the "sync-locked" watermark tiled to a rectangle
   COMMON_TRACK_UI_API
   void DrawSyncLockTiles(
      TrackPanelDrawingContext &context, const wxRect &rect );

   // Helper: draws background with selection rect
   COMMON_TRACK_UI_API
   void DrawBackgroundWithSelection(TrackPanelDrawingContext &context,
         const wxRect &rect, const Track *track,
         const wxBrush &selBrush, const wxBrush &unselBrush,
         bool useSelection = true);

   COMMON_TRACK_UI_API
   void DrawNegativeOffsetTrackArrows( TrackPanelDrawingContext &context,
                                       const wxRect & rect );
}

extern COMMON_TRACK_UI_API int GetWaveYPos(float value, float min, float max,
                       int height, bool dB, bool outer, float dBr,
                       bool clip);
extern float FromDB(float value, double dBRange);
extern COMMON_TRACK_UI_API float ValueOfPixel(int yy, int height, bool offset,
                          bool dB, double dBRange, float zoomMin, float zoomMax);

#endif
