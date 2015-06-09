/**********************************************************************

  Audacity: A Digital Audio Editor

  ViewInfo.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_VIEWINFO__
#define __AUDACITY_VIEWINFO__

#include "SelectedRegion.h"
#include "Experimental.h"

#include <vector>
#include <utility>
#include <wx/defs.h>

class Track;
class ZoomInfo;
class wxArrayString;
class XMLWriter;

#ifdef EXPERIMENTAL_FISHEYE
struct FisheyeInfo;
#endif

// The subset of ViewInfo information (other than selection)
// that is sufficient for purposes of TrackArtist,
// and for computing conversions between track times and pixel positions.
class AUDACITY_DLL_API ZoomInfo
{
public:
   ZoomInfo(double start, double duration, double pixelsPerSecond);
   ~ZoomInfo();

   void UpdatePrefs();

   int vpos;                    // vertical scroll pos

   double h;                    // h pos in secs

   double screen;               // screen width in secs

protected:
   double zoom;                 // pixels per second

public:

   // do NOT use this once to convert a pixel width to a duration!
   // Instead, call twice to convert start and end times,
   // and take the difference.
   // origin specifies the pixel corresponding to time h
   double PositionToTime(wxInt64 position,
      wxInt64 origin = 0
      , bool ignoreFisheye = false
   ) const;

   // do NOT use this once to convert a duration to a pixel width!
   // Instead, call twice to convert start and end positions,
   // and take the difference.
   // origin specifies the pixel corresponding to time h
   wxInt64 TimeToPosition(double time,
      wxInt64 origin = 0
      , bool ignoreFisheye = false
   ) const;

   double OffsetTimeByPixels(double time, wxInt64 offset) const
   {
      return PositionToTime(offset + TimeToPosition(time));
   }

   bool ZoomInAvailable() const;
   bool ZoomOutAvailable() const;

   // Return pixels, but maybe not a whole number
   double GetScreenWidth() const
   { return screen * zoom; }

   void SetScreenWidth(wxInt64 width)
   { screen = width / zoom; }

   static double GetDefaultZoom()
   { return 44100.0 / 512.0; }

   // There is NO GetZoom()!
   // Use TimeToPosition and PositionToTime and OffsetTimeByPixels!

   // Limits zoom to certain bounds
   void SetZoom(double pixelsPerSecond);

   // Limits zoom to certain bounds
   // multipliers above 1.0 zoom in, below out
   void ZoomBy(double multiplier);

   struct Interval {
      const wxInt64 position; const double averageZoom; const bool inFisheye;
      Interval(wxInt64 p, double z, bool i)
         : position(p), averageZoom(z), inFisheye(i) {}
   };
   typedef std::vector<Interval> Intervals;

   // Find an increasing sequence of pixel positions.  Each is the start
   // of an interval, or is the end position.
   // Each of the disjoint intervals should be drawn
   // separately.
   // It is guaranteed that there is at least one entry and the position of the
   // first entry equals origin.
   // @param origin specifies the pixel position corresponding to time ViewInfo::h.
   void FindIntervals
      (double rate, Intervals &results, wxInt64 origin = 0) const;

   enum FisheyeState {
      HIDDEN,
      PINNED,

      NUM_STATES,
   };

   FisheyeState GetFisheyeState() const;

   // Return true if the mouse position is anywhere in the fisheye
   // origin specifies the pixel corresponding to time h
   bool InFisheye(wxInt64 position, wxInt64 origin = 0) const;

   // These accessors ignore the fisheye hiding state.
   // Inclusive:
   wxInt64 GetFisheyeLeftBoundary(wxInt64 origin = 0) const;
   // Exclusive:
   wxInt64 GetFisheyeRightBoundary(wxInt64 origin = 0) const;

#ifdef EXPERIMENTAL_FISHEYE
   static wxArrayString GetFisheyeStyleChoices();
   static int GetFisheyeDefaultWidth() { return 300; }
   static int GetFisheyeDefaultMagnification() { return 2; }

   SelectedRegion GetFisheyeFocusRegion() const;


   // Return true if the mouse position is in the center portion of the
   // fisheye, which has a constant and maximal zoom
   // origin specifies the pixel corresponding to time h
   bool InFisheyeFocus(wxInt64 position, wxInt64 origin = 0) const;

   // These accessors ignore the fisheye hiding state.
   // Inclusive:
   wxInt64 GetFisheyeFocusLeftBoundary(wxInt64 origin = 0) const;
   wxInt64 GetFisheyeCenterPosition(wxInt64 origin = 0) const;
   // Exclusive:
   wxInt64 GetFisheyeFocusRightBoundary(wxInt64 origin = 0) const;

   // level indicates a multiplier of the background zoom that the fisheye
   // will maintain (except when maximum zoom is limited)
   // and must be at least 1.0
   double GetFisheyeTotalZoom() const;

   bool ZoomFisheyeBy(wxCoord position, wxCoord origin, double multiplier);
   bool DefaultFisheyeZoom(wxCoord position, wxCoord origin);

   void SetFisheyeState(FisheyeState state);

   void ChangeFisheyeStyle();
   void AdjustFisheyePixelWidth(int delta, int maximum);

   double GetFisheyeCenterTime() const;
   void SetFisheyeCenterTime(double time);

protected:
   void UpdateFisheye();
   FisheyeInfo *fisheye;

#endif
};

class AUDACITY_DLL_API ViewInfo
   : public ZoomInfo
{
public:
   ViewInfo(double start, double screenDuration, double pixelsPerSecond);

   double GetBeforeScreenWidth() const
   {
      return h * zoom;
   }
   void SetBeforeScreenWidth(wxInt64 width, double lowerBoundTime = 0.0);

   double GetTotalWidth() const
   { return total * zoom; }

   bool ZoomedAll() const
   { return screen >= total; }

   // Current selection

   SelectedRegion selectedRegion;

   // Scroll info

   Track *track;                // first visible track

   double total;                // total width in secs
   // Current horizontal scroll bar positions, in pixels
   wxInt64 sbarH;
   wxInt64 sbarScreen;
   wxInt64 sbarTotal;

   // Internal wxScrollbar positions are only int in range, so multiply
   // the above values with the following member to get the actual
   // scroll bar positions as reported by the horizontal wxScrollbar's members
   // i.e. units are scroll increments per pixel
   double sbarScale;

   // Vertical scroll step
   int scrollStep;

   // Other stuff, mainly states (true or false) related to autoscroll and
   // drawing the waveform. Maybe this should be put somewhere else?

   bool bUpdateTrackIndicator;

   void WriteXMLAttributes(XMLWriter &xmlFile);
   bool ReadXMLAttribute(const wxChar *attr, const wxChar *value);
};

#endif
