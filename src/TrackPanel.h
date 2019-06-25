/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL__
#define __AUDACITY_TRACK_PANEL__




#include <vector>

#include <wx/setup.h> // for wxUSE_* macros

#include "HitTestResult.h"
#include "Prefs.h"

#include "SelectedRegion.h"

#include "CellularPanel.h"

#include "commands/CommandManagerWindowClasses.h"


class wxRect;

class SelectedRegionEvent;
class SpectrumAnalyst;
class Track;
class TrackList;
struct TrackListEvent;
class TrackPanel;
class TrackArtist;
class SnapManager;
class LWSlider;

class TrackPanelAx;

// Declared elsewhere, to reduce compilation dependencies
class TrackPanelListener;

struct TrackPanelDrawingContext;

enum class UndoPush : unsigned char;

enum {
   kTimerInterval = 50, // milliseconds
};

const int DragThreshold = 3;// Anything over 3 pixels is a drag, else a click.

class AUDACITY_DLL_API TrackPanel final
   : public CellularPanel
   , public NonKeystrokeInterceptingWindow
   , private PrefsListener
{
 public:
   static TrackPanel &Get( AudacityProject &project );
   static const TrackPanel &Get( const AudacityProject &project );
   static void Destroy( AudacityProject &project );
 
   TrackPanel(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              const std::shared_ptr<TrackList> &tracks,
              ViewInfo * viewInfo,
              AudacityProject * project );

   virtual ~ TrackPanel();

   void UpdatePrefs() override;

   void OnAudioIO(wxCommandEvent & evt);

   void OnPaint(wxPaintEvent & event);
   void OnMouseEvent(wxMouseEvent & event);
   void OnKeyDown(wxKeyEvent & event);

   void OnTrackListResizing(TrackListEvent & event);
   void OnTrackListDeletion(wxEvent & event);
   void OnEnsureVisible(TrackListEvent & event);

   double GetMostRecentXPos();

   void OnSize( wxSizeEvent & );
   void OnTimer( wxCommandEvent& event );
   void OnProjectSettingsChange(wxCommandEvent &event);
   void OnSelectionChange(SelectedRegionEvent& evt);
   void OnTrackFocusChange( wxCommandEvent &event );

   void OnUndoReset( wxCommandEvent &event );

   void Refresh
      (bool eraseBackground = true, const wxRect *rect = (const wxRect *) NULL)
      override;

   void RefreshTrack(Track *trk, bool refreshbacking = true);
   void RefreshBacking() { mRefreshBacking = true; }

   void HandlePageUpKey();
   void HandlePageDownKey();
   AudacityProject * GetProject() const override;

   void OnTrackMenu(Track *t = NULL);

   void VerticalScroll( float fracPosition);

   TrackPanelCell *GetFocusedCell() override;
   void SetFocusedCell() override;

   void UpdateVRulers();
   void UpdateVRuler(Track *t);
   void UpdateTrackVRuler(Track *t);
   void UpdateVRulerSize();

   void MakeParentRedrawScrollbars();

   // Rectangle includes track control panel, and the vertical ruler, and
   // the proper track area of all channels, and the separators between them.
   wxRect FindTrackRect( const Track * target );

protected:
   // Get the root object defining a recursive subdivision of the panel's
   // area into cells
   std::shared_ptr<TrackPanelNode> Root() override;

public:
// JKC Nov-2011: These four functions only used from within a dll
// They work around some messy problems with constructors.
   const TrackList * GetTracks() const { return mTracks.get(); }
   TrackList * GetTracks() { return mTracks.get(); }
   ViewInfo * GetViewInfo(){ return mViewInfo;}
   TrackPanelListener * GetListener(){ return mListener;}

protected:
   void DrawTracks(wxDC * dc);

public:
   // Set the object that performs catch-all event handling when the pointer
   // is not in any track or ruler or control panel.
   void SetBackgroundCell
      (const std::shared_ptr< TrackPanelCell > &pCell);
   std::shared_ptr< TrackPanelCell > GetBackgroundCell();

public:

protected:
   TrackPanelListener *mListener;

   std::shared_ptr<TrackList> mTracks;

   std::unique_ptr<TrackArtist> mTrackArtist;

   bool mRefreshBacking;

protected:

   SelectedRegion mLastDrawnSelectedRegion {};

 protected:

   std::shared_ptr<TrackPanelCell> mpBackground;

   DECLARE_EVENT_TABLE()

   void ProcessUIHandleResult
      (TrackPanelCell *pClickedTrack, TrackPanelCell *pLatestCell,
       unsigned refreshResult) override;

   void UpdateStatusMessage( const TranslatableString &status ) override;
};

// A predicate class
struct AUDACITY_DLL_API IsVisibleTrack
{
   IsVisibleTrack(AudacityProject *project);

   bool operator () (const Track *pTrack) const;

   wxRect mPanelRect;
};

#endif
