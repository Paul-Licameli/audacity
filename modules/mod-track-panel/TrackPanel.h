/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL__
#define __AUDACITY_TRACK_PANEL__




#include <vector>

#include <wx/setup.h> // for wxUSE_* macros
#include <wx/timer.h> // to inherit

#include "HitTestResult.h"
#include "Prefs.h"

#include "SelectedRegion.h"

#include "CellularPanel.h"

#include "CommandManagerWindowClasses.h"


class wxRect;

struct AudioIOEvent;
class Track;
class TrackList;
struct TrackListEvent;
class TrackPanel;
class TrackArtist;
struct UndoRedoEvent;
class Ruler;
class SnapManager;
struct SyncLockChangeEvent;
class LWSlider;

class TrackPanelAx;

// Declared elsewhere, to reduce compilation dependencies
class TrackPanelListener;

struct TrackPanelDrawingContext;

enum class UndoPush : unsigned char;

const int DragThreshold = 3;// Anything over 3 pixels is a drag, else a click.

class TRACK_PANEL_API TrackPanel final
   : public CellularPanel
   , public NonKeystrokeInterceptingWindow
   , private PrefsListener
{
 public:
   static TrackPanel &Get( AudacityProject &project );
   static const TrackPanel &Get( const AudacityProject &project );
   bool Destroy() override;
 
   TrackPanel(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              const std::shared_ptr<TrackList> &tracks,
              ViewInfo * viewInfo,
              AudacityProject * project);

   virtual ~ TrackPanel();

   void UpdatePrefs() override;

   void OnAudioIO(AudioIOEvent & evt);

   void OnPaint(wxPaintEvent & event);
   void OnMouseEvent(wxMouseEvent & event);
   void OnKeyDown(wxKeyEvent & event);

   void OnTrackListResizing(TrackListEvent & event);
   void OnTrackListDeletion(wxEvent & event);
   void OnEnsureVisible(TrackListEvent & event);

   void OnSize( wxSizeEvent & );
   void OnIdle(wxIdleEvent & event);
   void OnTimer(wxTimerEvent& event);
   void OnSyncLockChange(SyncLockChangeEvent &event);
   void OnTrackFocusChange( wxCommandEvent &event );

   void OnUndoReset( UndoRedoEvent &event );

   void Refresh
      (bool eraseBackground = true, const wxRect *rect = (const wxRect *) NULL)
      override;

   void RefreshTrack(Track *trk, bool refreshbacking = true);

   void HandlePageUpKey();
   void HandlePageDownKey();
   AudacityProject * GetProject() const override;

   void VerticalScroll( float fracPosition);

   TrackPanelCell *GetFocusedCell() override;
   void SetFocusedCell() override;

   void UpdateVRulers();
   void UpdateVRuler(Track *t);
   void UpdateTrackVRuler(Track *t);
   void UpdateVRulerSize();

protected:
   bool IsAudioActive();

   void UpdateSelectionDisplay();

public:
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

   class TRACK_PANEL_API AudacityTimer final : public wxTimer {
   public:
     void Notify() override{
       // (From Debian)
       //
       // Don't call parent->OnTimer(..) directly here, but instead post
       // an event. This ensures that this is a pure wxWidgets event
       // (no GDK event behind it) and that it therefore isn't processed
       // within the YieldFor(..) of the clipboard operations (workaround
       // for Debian bug #765341).
       // QueueEvent() will take ownership of the event
       parent->GetEventHandler()->QueueEvent(safenew wxTimerEvent(*this));
     }
     TrackPanel *parent;
   } mTimer;

   int mTimeCount;

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

#endif
