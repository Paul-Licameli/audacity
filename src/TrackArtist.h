/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackArtist.h

  Dominic Mazzoni

  This singleton class handles the actual rendering of WaveTracks
  (both waveforms and spectra), NoteTracks, and LabelTracks.

  It's actually a little harder than it looks, because for
  waveforms at least it needs to cache the samples that are
  currently on-screen.

**********************************************************************/

#ifndef __AUDACITY_TRACKARTIST__
#define __AUDACITY_TRACKARTIST__

#include "Audacity.h" // for USE_* macros
#include "Experimental.h"

#include <wx/brush.h> // member variable
#include <wx/pen.h> // member variables

#include "Prefs.h"

class wxRect;

class SelectedRegion;
class Track;
class TrackPanel;
struct TrackPanelDrawingContext;
class ZoomInfo;

namespace TrackArt {
   
#if 0
   void DrawTrackNames(TrackPanelDrawingContext &context,
                       const TrackList *tracks,
                       const wxRegion & reg,
                       const wxRect &clip);
#endif

   void DrawTrackName(TrackPanelDrawingContext &context, 
                      const Track *t, const wxRect & rect );

   // Helper: draws the "sync-locked" watermark tiled to a rectangle
   void DrawSyncLockTiles(
      TrackPanelDrawingContext &context, const wxRect &rect );

   // Helper: draws background with selection rect
   void DrawBackgroundWithSelection(TrackPanelDrawingContext &contex,
         const wxRect &rect, const Track *track,
         const wxBrush &selBrush, const wxBrush &unselBrush,
         bool useSelection = true);

   void DrawNegativeOffsetTrackArrows( TrackPanelDrawingContext &context,
                                       const wxRect & rect );
}

class AUDACITY_DLL_API TrackArtist final : private PrefsListener {

public:

   enum : unsigned {
      PassTracks,
      PassMargins,
      PassBorders,
      PassControls,
      PassZooming,
      
      NPasses
   };

   TrackArtist( TrackPanel *parent_ );
   ~TrackArtist();
   static TrackArtist *Get( TrackPanelDrawingContext & );

   void SetColours(int iColorIndex);

   void SetBackgroundBrushes(wxBrush unselectedBrushIn, wxBrush selectedBrushIn,
                             wxPen unselectedPenIn, wxPen selectedPenIn) {
     this->unselectedBrush = unselectedBrushIn;
     this->selectedBrush = selectedBrushIn;
     this->unselectedPen = unselectedPenIn;
     this->selectedPen = selectedPenIn;
   }

   static int ShowClippingPrefsId();
   void UpdatePrefs() override;
   void UpdateSelectedPrefs( int id ) override;

   void UpdateVRuler(const Track *t, const wxRect & rect);

   TrackPanel *parent;

   // Preference values
   float mdBrange;            // "/GUI/EnvdBRange"
   long mShowClipping;        // "/GUI/ShowClipping"
   int  mSampleDisplay;
   bool mbShowTrackNameInTrack;  // "/GUI/ShowTrackNameInWaveform"

   wxBrush blankBrush;
   wxBrush unselectedBrush;
   wxBrush selectedBrush;
   wxBrush sampleBrush;
   wxBrush selsampleBrush;
   wxBrush dragsampleBrush;// for samples which are draggable.
   wxBrush muteSampleBrush;
   wxBrush blankSelectedBrush;
   wxPen blankPen;
   wxPen unselectedPen;
   wxPen selectedPen;
   wxPen samplePen;
   wxPen rmsPen;
   wxPen muteRmsPen;
   wxPen selsamplePen;
   wxPen muteSamplePen;
   wxPen odProgressNotYetPen;
   wxPen odProgressDonePen;
   wxPen shadowPen;
   wxPen clippedPen;
   wxPen muteClippedPen;
   wxPen blankSelectedPen;

#ifdef EXPERIMENTAL_FFT_Y_GRID
   bool fftYGridOld;
#endif //EXPERIMENTAL_FFT_Y_GRID

#ifdef EXPERIMENTAL_FIND_NOTES
   bool fftFindNotesOld;
   int findNotesMinAOld;
   int findNotesNOld;
   bool findNotesQuantizeOld;
#endif

   SelectedRegion *pSelectedRegion{};
   ZoomInfo *pZoomInfo{};

   bool drawEnvelope{ false };
   bool bigPoints{ false };
   bool drawSliders{ false };
   bool hasSolo{ false };
};

extern int GetWaveYPos(float value, float min, float max,
                       int height, bool dB, bool outer, float dBr,
                       bool clip);
extern float FromDB(float value, double dBRange);
extern float ValueOfPixel(int yy, int height, bool offset,
                          bool dB, double dBRange, float zoomMin, float zoomMax);

#endif                          // define __AUDACITY_TRACKARTIST__
