/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL__
#define __AUDACITY_TRACK_PANEL__

#include <memory>
#include <vector>

#include <wx/dcmemory.h>
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/window.h>

#include "Experimental.h"
#include "audacity/Types.h"
#include "UndoManager.h" //JKC: Included for PUSH_XXX definitions.
#include "widgets/NumericTextCtrl.h"

class wxMenu;
class wxRect;

class LabelTrack;
class SpectrumAnalyst;
class TrackPanel;
class TrackPanelCell;
class TrackPanelOverlay;
class TrackArtist;
class Ruler;
class SnapManager;
class AdornedRulerPanel;
class LWSlider;
class ControlToolBar; //Needed because state of controls can affect what gets drawn.
class ToolsToolBar; //Needed because state of controls can affect what gets drawn.
class MixerBoard;
class AudacityProject;

class TrackPanelAx;

class ViewInfo;

class WaveTrack;
class WaveClip;
class Envelope;
class UIHandle;

// Declared elsewhere, to reduce compilation dependencies
class TrackPanelListener;

// JKC Nov 2011: Disabled warning C4251 which is to do with DLL linkage
// and only a worry when there are DLLs using the structures.
// Array classes are private in TrackInfo, so we will not
// access them directly from the DLL.
// TrackClipArray in TrackPanel needs to be handled with care in the derived
// class, but the C4251 warning is no worry in core Audacity.
// wxWidgets doesn't cater to the exact details we need in
// WX_DECLARE_EXPORTED_OBJARRAY to be able to use that for these two arrays.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4251 )
#endif

DECLARE_EXPORTED_EVENT_TYPE(AUDACITY_DLL_API, EVT_TRACK_PANEL_TIMER, -1);

enum {
   kTimerInterval = 50, // milliseconds
   kOneSecondCountdown = 1000 / kTimerInterval,
};

class AUDACITY_DLL_API TrackInfo
{
public:
   TrackInfo(TrackPanel * pParentIn);
   ~TrackInfo();

public:
   int GetTrackInfoWidth() const;
   void SetTrackInfoFont(wxDC *dc) const;

   void DrawBackground(wxDC * dc, const wxRect & rect, bool bSelected, bool bHasMuteSolo, const int labelw, const int vrul) const;
   void DrawBordersWithin(wxDC * dc, const wxRect & rect, bool bHasMuteSolo ) const;
   void DrawCloseBox(wxDC * dc, const wxRect & rect, bool down) const;
   void DrawTitleBar(wxDC * dc, const wxRect & rect, Track * t, bool down) const;
   void DrawMuteSolo(wxDC * dc, const wxRect & rect, Track * t, bool down, bool solo, bool bHasSoloButton) const;
   void DrawVRuler(wxDC * dc, const wxRect & rect, Track * t) const;
#ifdef EXPERIMENTAL_MIDI_OUT
   void DrawVelocitySlider(wxDC * dc, NoteTrack *t, wxRect rect) const ;
#endif
   void DrawSliders(wxDC * dc, WaveTrack *t, wxRect rect) const;

   // Draw the minimize button *and* the sync-lock track icon, if necessary.
   void DrawMinimize(wxDC * dc, const wxRect & rect, Track * t, bool down) const;

   static void GetTrackControlsRect(const wxRect & rect, wxRect &dest);
   static void GetCloseBoxRect(const wxRect & rect, wxRect &dest);
   static void GetTitleBarRect(const wxRect & rect, wxRect &dest);
   static void GetMuteSoloRect(const wxRect & rect, wxRect &dest, bool solo, bool bHasSoloButton);
   static void GetGainRect(const wxRect & rect, wxRect &dest);
   static void GetPanRect(const wxRect & rect, wxRect &dest);
   static void GetMinimizeRect(const wxRect & rect, wxRect &dest);
   static void GetSyncLockIconRect(const wxRect & rect, wxRect &dest);

   // TrackSelFunc returns true if the click is not
   // set up to be handled, but click is on the sync-lock icon or the blank area to
   // the left of the minimize button, and we want to pass it forward to be a track select.
   static bool TrackSelFunc(Track * t, wxRect rect, int x, int y);

public:
   LWSlider * GainSlider(WaveTrack *t) const;
   LWSlider * PanSlider(WaveTrack *t) const;

private:
   TrackPanel * pParent;
   wxFont mFont;
   LWSlider *mGain;
   LWSlider *mPan;

   friend class TrackPanel;
};


const int DragThreshold = 3;// Anything over 3 pixels is a drag, else a click.


class AUDACITY_DLL_API TrackPanel:public wxPanel {
 public:

   TrackPanel(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              TrackList * tracks,
              ViewInfo * viewInfo,
              TrackPanelListener * listener,
              AdornedRulerPanel * ruler );

   virtual ~ TrackPanel();

   virtual void UpdatePrefs();

   virtual void OnSize(wxSizeEvent & event);
   virtual void OnPaint(wxPaintEvent & event);
   virtual void OnMouseEvent(wxMouseEvent & event);
   virtual void OnCaptureLost(wxMouseCaptureLostEvent & event);
   virtual void OnCaptureKey(wxCommandEvent & event);
   virtual void OnKeyDown(wxKeyEvent & event);
   virtual void OnChar(wxKeyEvent & event);
   virtual void OnKeyUp(wxKeyEvent & event);

   virtual void OnSetFocus(wxFocusEvent & event);
   virtual void OnKillFocus(wxFocusEvent & event);

   virtual void OnContextMenu(wxContextMenuEvent & event);

   virtual void OnTrackListResized(wxCommandEvent & event);
   virtual void OnTrackListUpdated(wxCommandEvent & event);
   virtual void UpdateViewIfNoTracks(); // Call this to update mViewInfo, etc, after track(s) removal, before Refresh().

   virtual double GetMostRecentXPos();

   virtual void OnTimer(wxTimerEvent& event);

   virtual int GetLeftOffset() const { return GetLabelWidth() + 1;}

   // Width and height, relative to upper left corner at (GetLeftOffset(), 0)
   // Either argument may be NULL
   virtual void GetTracksUsableArea(int *width, int *height) const;

   virtual void SelectNone();

   virtual void Refresh(bool eraseBackground = true,
                        const wxRect *rect = (const wxRect *) NULL);
   virtual void RefreshTrack(Track *trk, bool refreshbacking = true);

   virtual void DisplaySelection();

   // These two are neither used nor defined as of Nov-2011
   //virtual void SetSelectionFormat(int iformat)
   //virtual void SetSnapTo(int snapto)

   virtual void CancelDragging();
   virtual void HandleEscapeKey(bool down);
   virtual void HandleAltKey(bool down);
   virtual void HandleShiftKey(bool down);
   virtual void HandleControlKey(bool down);
   virtual void HandlePageUpKey();
   virtual void HandlePageDownKey();
   virtual AudacityProject * GetProject() const;

   virtual void OnPrevTrack(bool shift = false);
   virtual void OnNextTrack(bool shift = false);
   virtual void OnFirstTrack();
   virtual void OnLastTrack();
   virtual void OnToggle();

   virtual void ScrollIntoView(double pos);
   virtual void ScrollIntoView(int x);

   virtual void OnTrackMenu(Track *t = NULL);
   virtual Track * GetFirstSelectedTrack();
   virtual bool IsMouseCaptured();

   virtual void EnsureVisible(Track * t);

   virtual Track *GetFocusedTrack();
   virtual void SetFocusedTrack(Track *t);

   virtual void HandleCursorForLastMouseEvent();

   virtual void UpdateVRulers();
   virtual void UpdateVRuler(Track *t);
   virtual void UpdateTrackVRuler(Track *t);
   virtual void UpdateVRulerSize();

   // Returns the time corresponding to the pixel column one past the track area
   // (ignoring any fisheye)
   virtual double GetScreenEndTime() const;

 protected:
   virtual MixerBoard* GetMixerBoard();

   virtual bool IsAudioActive();
   virtual bool IsUnsafe();
   virtual bool HandleLabelTrackClick(LabelTrack * lTrack, wxRect &rect, wxMouseEvent & event);
   virtual void HandleGlyphDragRelease(LabelTrack * lTrack, wxMouseEvent & event);
   virtual void HandleTextDragRelease(LabelTrack * lTrack, wxMouseEvent & event);
   virtual void HandleTrackSpecificMouseEvent(wxMouseEvent & event);

   virtual void ScrollDuringDrag();

   // Working out where to dispatch the event to.
   virtual int DetermineToolToUse( ToolsToolBar * pTtb, wxMouseEvent & event);
#ifdef USE_MIDI
   // data for NoteTrack interactive stretch operations:
   // Stretching applies to a selected region after quantizing the
   // region to beat boundaries (subbeat stretching is not supported,
   // but maybe it should be enabled with shift or ctrl or something)
   // Stretching can drag the left boundary (the right stays fixed),
   // the right boundary (the left stays fixed), or the center (splits
   // the selection into two parts: when left part grows, the right
   // part shrinks, keeping the leftmost and rightmost boundaries
   // fixed.
   enum StretchEnum {
      stretchLeft,
      stretchCenter,
      stretchRight
   };
   StretchEnum mStretchMode; // remembers what to drag
   bool mStretching; // true between mouse down and mouse up
   bool mStretched; // true after drag has pushed state
   double mStretchStart; // time of initial mouse position, quantized
                         // to the nearest beat
   double mStretchSel0;  // initial sel0 (left) quantized to nearest beat
   double mStretchSel1;  // initial sel1 (left) quantized to nearest beat
   double mStretchLeftBeats; // how many beats from left to cursor
   double mStretchRightBeats; // how many beats from cursor to right
   virtual bool HitTestStretch(Track *track, wxRect &rect, wxMouseEvent & event);
   virtual void Stretch(int mouseXCoordinate, int trackLeftEdge, Track *pTrack);
#endif

   // AS: Selection handling
   virtual void HandleSelect(wxMouseEvent & event);
   virtual void SelectionHandleDrag(wxMouseEvent &event, Track *pTrack);

protected:

   virtual void SelectionHandleClick(wxMouseEvent &event,
                                     Track* pTrack, wxRect rect);
   virtual void StartSelection (int mouseXCoordinate, int trackLeftEdge);
   virtual void ExtendSelection(int mouseXCoordinate, int trackLeftEdge,
                        Track *pTrack);
   virtual void UpdateSelectionDisplay();

public:
   virtual void UpdateAccessibility();

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
public:
   void SnapCenterOnce (const WaveTrack *pTrack, bool up);
protected:
   void StartSnappingFreqSelection (const WaveTrack *pTrack);
   void MoveSnappingFreqSelection (int mouseYCoordinate,
                                   int trackTopEdge,
                                   int trackHeight, Track *pTrack);
   void StartFreqSelection (int mouseYCoordinate, int trackTopEdge,
                            int trackHeight, Track *pTrack);
   void ExtendFreqSelection(int mouseYCoordinate, int trackTopEdge,
                            int trackHeight);
   void ResetFreqSelectionPin(double hintFrequency, bool logF);

#endif

   virtual void SelectTracksByLabel( LabelTrack *t );
   virtual void SelectTrackLength(Track *t);

   // AS: Cursor handling
   virtual bool SetCursorByActivity( );
   virtual void SetCursorAndTipWhenInLabel( Track * t, wxMouseEvent &event, wxString &tip );
   virtual void SetCursorAndTipWhenInVResizeArea( bool blinked, wxString &tip );
   virtual void SetCursorAndTipWhenInLabelTrack( LabelTrack * pLT, wxMouseEvent & event, wxString &tip );
   virtual void SetCursorAndTipWhenSelectTool
      ( Track * t, wxMouseEvent & event, wxRect &rect, bool bMultiToolMode, wxString &tip, const wxCursor ** ppCursor );
   virtual void SetCursorAndTipByTool( int tool, wxMouseEvent & event, wxString &tip );

   virtual void HandleCursor(wxMouseEvent & event);
   virtual void MaySetOnDemandTip( Track * t, wxString &tip );

   void FindCell(const wxMouseEvent &event, wxRect &inner, TrackPanelCell *& pCell, Track *& pTrack);

   // MM: Handle mouse wheel rotation
   virtual void HandleWheelRotation(wxMouseEvent & event);

   // Handle resizing.
   virtual void HandleResizeClick(wxMouseEvent & event);
   virtual void HandleResizeDrag(wxMouseEvent & event);
   virtual void HandleResizeButtonUp(wxMouseEvent & event);
   virtual void HandleResize(wxMouseEvent & event);

   virtual void HandleLabelClick(wxMouseEvent & event);
   virtual void HandleRearrange(wxMouseEvent & event);
   virtual void CalculateRearrangingThresholds(wxMouseEvent & event);

public:
   virtual void MakeParentRedrawScrollbars();

protected:
   // AS: Pushing the state preserves state for Undo operations.
   virtual void MakeParentPushState(wxString desc, wxString shortDesc,
                            int flags = PUSH_AUTOSAVE);
   virtual void MakeParentModifyState(bool bWantsAutoSave);    // if true, writes auto-save file. Should set only if you really want the state change restored after
                                                               // a crash, as it can take many seconds for large (eg. 10 track-hours) projects
protected:
   // Find track info by coordinate
   virtual Track *FindTrack(int mouseX, int mouseY, bool label, bool link,
                     wxRect * trackRect = NULL);

   virtual wxRect FindTrackRect(Track * target, bool label);

   virtual int GetVRulerWidth() const;
   virtual int GetVRulerOffset() const { return mTrackInfo.GetTrackInfoWidth(); }

   virtual int GetLabelWidth() const { return mTrackInfo.GetTrackInfoWidth() + GetVRulerWidth(); }

// JKC Nov-2011: These four functions only used from within a dll such as mod-track-panel
// They work around some messy problems with constructors.
public:
   TrackList * GetTracks(){ return mTracks;}
   ViewInfo * GetViewInfo(){ return mViewInfo;}
   TrackPanelListener * GetListener(){ return mListener;}
   AdornedRulerPanel * GetRuler(){ return mRuler;}
// JKC and here is a factory function which just does 'new' in standard Audacity.
   static TrackPanel *(*FactoryFunction)(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              TrackList * tracks,
              ViewInfo * viewInfo,
              TrackPanelListener * listener,
              AdornedRulerPanel * ruler);

protected:
   virtual void DrawTracks(wxDC * dc);

   virtual void DrawEverythingElse(wxDC *dc, const wxRegion & region,
                           const wxRect & clip);
   virtual void DrawOutside(Track *t, wxDC *dc, const wxRect & rec,
                    const wxRect &trackRect);

   virtual void HighlightFocusedTrack (wxDC* dc, const wxRect &rect);
   virtual void DrawShadow            (Track *t, wxDC* dc, const wxRect & rect);
   virtual void DrawBordersAroundTrack(Track *t, wxDC* dc, const wxRect & rect, const int labelw, const int vrul);
   virtual void DrawOutsideOfTrack    (Track *t, wxDC* dc, const wxRect & rect);

public:
   // Register and unregister overlay objects.
   // The sequence in which they were registered is the sequence in
   // which they are painted.
   // TrackPanel is not responsible for their memory management.
   virtual void AddOverlay(TrackPanelOverlay *pOverlay);
   // Returns true if the overlay was found
   virtual bool RemoveOverlay(TrackPanelOverlay *pOverlay);
   virtual void ClearOverlays();

   // Set the object that performs catch-all event handling when the pointer
   // is not in any track or ruler or control panel.
   // TrackPanel does NOT assume memory management responsibility
   virtual void SetBackgroundCell(TrackPanelCell *pCell);

   // Erase and redraw things like the cursor, cheaply and directly to the
   // client area, without full refresh.
   virtual void DrawOverlays(bool repaint);

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   void UpdateVirtualStereoOrder();
#endif
   // Accessors...
   virtual bool HasSoloButton(){  return mSoloPref!=wxT("None");}

   //JKC: These two belong in the label track.
   int mLabelTrackStartXPos;
   int mLabelTrackStartYPos;

   virtual wxString TrackSubText(Track *t);

   TrackInfo mTrackInfo;
 public:
    TrackInfo *GetTrackInfo() { return &mTrackInfo; }
    const TrackInfo *GetTrackInfo() const { return &mTrackInfo; }

protected:
   TrackPanelListener *mListener;

   TrackList *mTracks;
   ViewInfo *mViewInfo;

   AdornedRulerPanel *mRuler;

   TrackArtist *mTrackArtist;

   class AUDACITY_DLL_API AudacityTimer:public wxTimer {
   public:
     virtual void Notify() {
       // (From Debian)
       //
       // Don't call parent->OnTimer(..) directly here, but instead post
       // an event. This ensures that this is a pure wxWidgets event
       // (no GDK event behind it) and that it therefore isn't processed
       // within the YieldFor(..) of the clipboard operations (workaround
       // for Debian bug #765341).
       wxTimerEvent *event = new wxTimerEvent(*this);
       parent->GetEventHandler()->QueueEvent(event);
     }
     TrackPanel *parent;
   } mTimer;

   int mTimeCount;

   wxMemoryDC mBackingDC;
   wxBitmap *mBacking;
   bool mResizeBacking;
   bool mRefreshBacking;
   int mPrevWidth;
   int mPrevHeight;

   SelectedRegion mInitialSelection;
   // Extra indirection to avoid the stupid MSW compiler warnings!  Rrrr!
   std::vector<bool> *mInitialTrackSelection;

   bool mSelStartValid;
   double mSelStart;

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   enum eFreqSelMode {
      FREQ_SEL_INVALID,

      FREQ_SEL_SNAPPING_CENTER,
      FREQ_SEL_PINNED_CENTER,
      FREQ_SEL_DRAG_CENTER,

      FREQ_SEL_FREE,
      FREQ_SEL_TOP_FREE,
      FREQ_SEL_BOTTOM_FREE,
   }  mFreqSelMode;
   // Following holds:
   // the center for FREQ_SEL_PINNED_CENTER,
   // the ratio of top to center (== center to bottom) for FREQ_SEL_DRAG_CENTER,
   // a frequency boundary for FREQ_SEL_FREE, FREQ_SEL_TOP_FREE, or
   // FREQ_SEL_BOTTOM_FREE,
   // and is ignored otherwise.
   double mFreqSelPin;
   const WaveTrack *mFreqSelTrack;
   std::auto_ptr<SpectrumAnalyst> mFrequencySnapper;

   // For toggling of spectral seletion
   double mLastF0;
   double mLastF1;

public:
   void ToggleSpectralSelection();
protected:

#endif

   Track *mCapturedTrack;
   wxRect mCapturedRect;

   bool mRedrawAfterStop;

   wxMouseEvent mLastMouseEvent;

   int mMouseClickY;

   int mMouseMostRecentX;
   int mMouseMostRecentY;

   // Handles snapping the selection boundaries or track boundaries to
   // line up with existing tracks or labels.  mSnapLeft and mSnapRight
   // are the horizontal index of pixels to display user feedback
   // guidelines so the user knows when such snapping is taking place.
   SnapManager *mSnapManager;
   wxInt64 mSnapLeft;
   wxInt64 mSnapRight;

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   void HandleCenterFrequencyCursor
      (bool shiftDown, wxString &tip, const wxCursor ** ppCursor);

   void HandleCenterFrequencyClick
      (bool shiftDown, Track *pTrack, double value);

   double PositionToFrequency(const WaveTrack *wt,
                              bool maySnap,
                              wxInt64 mouseYCoordinate,
                              wxInt64 trackTopEdge,
                              int trackHeight) const;
   wxInt64 FrequencyToPosition(const WaveTrack *wt,
                               double frequency,
                               wxInt64 trackTopEdge,
                               int trackHeight) const;
#endif

   enum SelectionBoundary {
      SBNone,
      SBLeft, SBRight,
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      SBBottom, SBTop, SBCenter, SBWidth,
#endif
   };
   SelectionBoundary ChooseTimeBoundary
      (double selend, bool onlyWithinSnapDistance,
       wxInt64 *pPixelDist = NULL, double *pPinValue = NULL) const;
   SelectionBoundary ChooseBoundary
      (wxMouseEvent & event, const Track *pTrack,
       const wxRect &rect,
       bool mayDragWidth,
       bool onlyWithinSnapDistance,
       double *pPinValue = NULL) const;

   bool mInitialMinimized;
   int mInitialTrackHeight;
   int mInitialActualHeight;
   int mInitialUpperTrackHeight;
   int mInitialUpperActualHeight;
   bool mAutoScrolling;

public:
   // Old enumeration of click-and-drag states, which will shrink and disappear
   // as UIHandle subclasses take over the repsonsibilities.
   enum   MouseCaptureEnum
   {
      IsUncaptured=0,   // This is the normal state for the mouse
      IsClosing,
      IsSelecting,
      IsAdjustingLabel,
      IsSelectingLabelText,
      IsResizing,
      IsResizingBetweenLinkedTracks,
      IsResizingBelowLinkedTracks,
      IsRearranging,
      IsMuting,
      IsSoloing,
      IsMinimizing,
      IsPopping,
   };

protected:
   enum MouseCaptureEnum mMouseCapture;

public:
   virtual void SetCapturedTrack( Track * t, enum MouseCaptureEnum MouseCapture=IsUncaptured );

protected:
   bool mCircularTrackNavigation;

   // JH: if the user is dragging a track, at what y
   //   coordinate should the dragging track move up or down?
   int mMoveUpThreshold;
   int mMoveDownThreshold;
   int mRearrangeCount;

   wxCursor *mArrowCursor;
   wxCursor *mSelectCursor;
   wxCursor *mResizeCursor;
   wxCursor *mEnvelopeCursor; // doubles as the center frequency cursor
                              // for spectral selection
   wxCursor *mRearrangeCursor;
   wxCursor *mDisabledCursor;
   wxCursor *mAdjustLeftSelectionCursor;
   wxCursor *mAdjustRightSelectionCursor;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   wxCursor *mBottomFrequencyCursor;
   wxCursor *mTopFrequencyCursor;
   wxCursor *mBandWidthCursor;
#endif
#if USE_MIDI
   wxCursor *mStretchCursor;
   wxCursor *mStretchLeftCursor;
   wxCursor *mStretchRightCursor;
#endif

   friend class TrackPanelAx;

   TrackPanelAx *mAx;

public:
   TrackPanelAx &GetAx() { return *mAx; }

protected:

   wxString mSoloPref;

 protected:

   // The screenshot class needs to access internals
   friend class ScreenshotCommand;

 public:
   wxSize vrulerSize;

 protected:
   std::vector<TrackPanelOverlay*> *mOverlays;

   Track *mpClickedTrack;
   // TrackPanel is not responsible for memory management:
   UIHandle *mUIHandle;

   TrackPanelCell *mpBackground;

   DECLARE_EVENT_TABLE()
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif


//This constant determines the size of the vertical region (in pixels) around
//the bottom of a track that can be used for vertical track resizing.
#define TRACK_RESIZE_REGION 5

//This constant determines the size of the horizontal region (in pixels) around
//the right and left selection bounds that can be used for horizontal selection adjusting
//(or, vertical distance around top and bottom bounds in spectrograms,
// for vertical selection adjusting)
#define SELECTION_RESIZE_REGION 3

#endif

