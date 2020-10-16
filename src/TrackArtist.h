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




#include <wx/brush.h> // member variable
#include <wx/pen.h> // member variables

#include "Prefs.h"

class wxRect;

class AudacityProject;
class TrackList;
class TrackPanel;
class SelectedRegion;
class Track;
class TrackPanel;
struct TrackPanelDrawingContext;
class ZoomInfo;

class AUDACITY_DLL_API TrackArtist final : private PrefsListener {

public:
   static int ShowClippingPrefsID();
   static int ShowTrackNameInWaveformPrefsID();

   enum : unsigned {
      PassTracks,
      PassMargins,
      PassBorders,
      PassControls,
      PassZooming,
      PassBackground,
      PassFocus,
      PassSnapping,
      
      NPasses
   };

   TrackArtist( AudacityProject &project,
      TrackPanel &parent, const ZoomInfo &zoomInfo,
      const SelectedRegion &selectedRegion );
   ~TrackArtist();
   static TrackArtist *Get( TrackPanelDrawingContext & );

   void SetBackgroundBrushes(wxBrush unselectedBrushIn, wxBrush selectedBrushIn,
                             wxPen unselectedPenIn, wxPen selectedPenIn) {
     this->unselectedBrush = unselectedBrushIn;
     this->selectedBrush = selectedBrushIn;
     this->unselectedPen = unselectedPenIn;
     this->selectedPen = selectedPenIn;
   }

   void SetColours(int iColorIndex);

   void UpdatePrefs() override;
   void UpdateSelectedPrefs( int id ) override;

   AudacityProject &project;
   TrackPanel &parent;

   // Preference values
   float mdBrange;            // "/GUI/EnvdBRange"
   bool mShowClipping;        // "/GUI/ShowClipping"
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

   const SelectedRegion &selectedRegion;
   const ZoomInfo &zoomInfo;

   bool drawEnvelope{ false };
   bool bigPoints{ false };
   bool drawSliders{ false };
   bool hasSolo{ false };
};

#endif                          // define __AUDACITY_TRACKARTIST__
