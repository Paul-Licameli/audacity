/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL__
#define __AUDACITY_TRACK_PANEL__

#include <memory>

#include <wx/dcmemory.h>
#include <wx/dynarray.h>
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/window.h>

#include "Experimental.h"
#include "Sequence.h"  //Stm: included for the sampleCount declaration
#include "WaveClip.h"
#include "WaveTrack.h"
#include "UndoManager.h" //JKC: Included for PUSH_XXX definitions.
#include "widgets/NumericTextCtrl.h"

class wxMenu;
class wxRect;

class LabelTrack;
class SpectrumAnalyst;
class TrackPanel;
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

struct ViewInfo;

WX_DEFINE_ARRAY(LWSlider *, LWSliderArray);

class AUDACITY_DLL_API TrackClip
{
 public:
   TrackClip(Track *t, WaveClip *c) { track = t; clip = c; }
   Track *track;
   WaveClip *clip;
};

WX_DECLARE_OBJARRAY(TrackClip, TrackClipArray);

// Declared elsewhere, to reduce compilation dependencies
class TrackPanelListener;

//
// TrackInfo sliders: we keep a pool of sliders, and attach them to tracks as
// they come on screen (this helps deal with very large numbers of tracks, esp.
// on MSW).
//
// With the initial set of sliders smaller than the page size, a new slider is
// created at track-creation time for tracks between 16 and when 80 goes past
// the top of the screen. After that, existing sliders are re-used for new
// tracks.
//
const unsigned int kInitialSliders = 16;
const unsigned int kSliderPageFlip = 80;

// JKC Nov 2011: Disabled warning C4251 which is to do with DLL linkage
// and only a worry when there are DLLs using the structures.
// LWSliderArray and TrackClipArray are private in TrackInfo, so we will not
// access them directly from the DLL.
// TrackClipArray in TrackPanel needs to be handled with care in the derived
// class, but the C4251 warning is no worry in core Audacity.
// wxWidgets doesn't cater to the exact details we need in
// WX_DECLARE_EXPORTED_OBJARRAY to be able to use that for these two arrays.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4251 )
#endif

class AUDACITY_DLL_API TrackInfo
{
public:
   TrackInfo(wxWindow * pParentIn);
   ~TrackInfo();

   int GetTrackInfoWidth() const;

   void UpdateSliderOffset(Track *t);

private:
   void MakeMoreSliders();
   void EnsureSufficientSliders(int index);

   void SetTrackInfoFont(wxDC *dc);
   void DrawBackground(wxDC * dc, const wxRect r, bool bSelected, bool bHasMuteSolo, const int labelw, const int vrul);
   void DrawBordersWithin(wxDC * dc, const wxRect r, bool bHasMuteSolo );
   void DrawCloseBox(wxDC * dc, const wxRect r, bool down);
   void DrawTitleBar(wxDC * dc, const wxRect r, Track * t, bool down);
   void DrawMuteSolo(wxDC * dc, const wxRect r, Track * t, bool down, bool solo, bool bHasSoloButton);
   void DrawVRuler(wxDC * dc, const wxRect r, Track * t);
#ifdef EXPERIMENTAL_MIDI_OUT
   void DrawVelocitySlider(wxDC * dc, NoteTrack *t, wxRect r);
#endif
   void DrawSliders(wxDC * dc, WaveTrack *t, wxRect r);

   // Draw the minimize button *and* the sync-lock track icon, if necessary.
   void DrawMinimize(wxDC * dc, const wxRect r, Track * t, bool down);

   void GetTrackControlsRect(const wxRect r, wxRect &dest) const;
   void GetCloseBoxRect(const wxRect r, wxRect &dest) const;
   void GetTitleBarRect(const wxRect r, wxRect &dest) const;
   void GetMuteSoloRect(const wxRect r, wxRect &dest, bool solo, bool bHasSoloButton) const;
   void GetGainRect(const wxRect r, wxRect &dest) const;
   void GetPanRect(const wxRect r, wxRect &dest) const;
   void GetMinimizeRect(const wxRect r, wxRect &dest) const;
   void GetSyncLockIconRect(const wxRect r, wxRect &dest) const;

   // These arrays are always kept the same size.
   LWSliderArray mGains;
   LWSliderArray mPans;

   // index of track whose pan/gain sliders are at index 0 in the above arrays
   unsigned int mSliderOffset;

public:

   // Slider access by track index
   LWSlider * GainSlider(int trackIndex);
   LWSlider * PanSlider(int trackIndex);

   wxWindow * pParent;
   wxFont mFont;

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

   void BuildMenus(void);

   void DeleteMenus(void);

   void UpdatePrefs();

   void OnSize(wxSizeEvent & event);
   void OnErase(wxEraseEvent & event);
   void OnPaint(wxPaintEvent & event);
   void OnMouseEvent(wxMouseEvent & event);
   void OnCaptureLost(wxMouseCaptureLostEvent & event);
   void OnCaptureKey(wxCommandEvent & event);
   void OnKeyDown(wxKeyEvent & event);
   void OnChar(wxKeyEvent & event);

   void OnSetFocus(wxFocusEvent & event);
   void OnKillFocus(wxFocusEvent & event);

   void OnContextMenu(wxContextMenuEvent & event);

   void OnTrackListResized(wxCommandEvent & event);
   void OnTrackListUpdated(wxCommandEvent & event);
   void UpdateViewIfNoTracks(); // Call this to update mViewInfo, etc, after track(s) removal, before Refresh().

   double GetMostRecentXPos();

   void OnTimer();

   int GetLeftOffset() const { return GetLabelWidth() + 1;}

   void GetTracksUsableArea(int *width, int *height) const;

   void SelectNone();

   // void SetStop(bool bStopped);

   void Refresh(bool eraseBackground = true,
                        const wxRect *rect = (const wxRect *) NULL);
   void RefreshTrack(Track *trk, bool refreshbacking = true);

   void DisplaySelection();

   // These two are neither used nor defined as of Nov-2011
   //void SetSelectionFormat(int iformat)
   //void SetSnapTo(int snapto)

   void HandleAltKey(bool down);
   void HandleShiftKey(bool down);
   void HandleControlKey(bool down);
   void HandlePageUpKey();
   void HandlePageDownKey();
   AudacityProject * GetProject() const;

   void OnPrevTrack(bool shift = false);
   void OnNextTrack(bool shift = false);
   void OnToggle();

   void OnCursorLeft(bool shift, bool ctrl, bool keyup = false);
   void OnCursorRight(bool shift, bool ctrl, bool keyup = false);
   void OnCursorMove(bool forward, bool jump, bool longjump);
   void OnBoundaryMove(bool left, bool boundaryContract);
   void ScrollIntoView(double pos);
   void ScrollIntoView(int x);

   void OnTrackPan();
   void OnTrackPanLeft();
   void OnTrackPanRight();
   void OnTrackGain();
   void OnTrackGainDec();
   void OnTrackGainInc();
   void OnTrackMenu(Track *t = NULL);
   void OnTrackMute(bool shiftdown, Track *t = NULL);
   void OnTrackSolo(bool shiftdown, Track *t = NULL);
   void OnTrackClose();
   Track * GetFirstSelectedTrack();
   bool IsMouseCaptured();

   void EnsureVisible(Track * t);

   Track *GetFocusedTrack();
   void SetFocusedTrack(Track *t);

   void HandleCursorForLastMouseEvent();

   void UpdateVRulers();
   void UpdateVRuler(Track *t);
   void UpdateTrackVRuler(Track *t);
   void UpdateVRulerSize();

 protected:
   MixerBoard* GetMixerBoard();
   /** @brief Populates the track pop-down menu with the common set of
    * initial items.
    *
    * Ensures that all pop-down menus start with Name, and the commands for moving
    * the track around, via a single set of code.
    * @param menu the menu to add the commands to.
    */
   void BuildCommonDropMenuItems(wxMenu * menu);
   bool IsUnsafe();
   bool HandleLabelTrackMouseEvent(LabelTrack * lTrack, wxRect &r, wxMouseEvent & event);
   bool HandleTrackLocationMouseEvent(WaveTrack * track, wxRect &r, wxMouseEvent &event);
   void HandleTrackSpecificMouseEvent(wxMouseEvent & event);
   void DrawIndicator();
   /// draws the green line on the tracks to show playback position
   /// @param repairOld if true the playback position is not updated/erased, and simply redrawn
   void DoDrawIndicator(wxDC & dc, bool repairOld = false);
   void DrawCursor();
   void DoDrawCursor(wxDC & dc);

   void ScrollDuringDrag();

   // Working out where to dispatch the event to.
   int DetermineToolToUse( ToolsToolBar * pTtb, wxMouseEvent & event);
   bool HitTestEnvelope(Track *track, wxRect &r, wxMouseEvent & event);
   bool HitTestSamples(Track *track, wxRect &r, wxMouseEvent & event);
   bool HitTestSlide(Track *track, wxRect &r, wxMouseEvent & event);
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
   bool HitTestStretch(Track *track, wxRect &r, wxMouseEvent & event);
   void Stretch(int mouseXCoordinate, int trackLeftEdge, Track *pTrack);
#endif

   // AS: Selection handling
   void HandleSelect(wxMouseEvent & event);
   void SelectionHandleDrag(wxMouseEvent &event, Track *pTrack);
   void StartOrJumpPlayback(wxMouseEvent &event);
#ifdef EXPERIMENTAL_SCRUBBING
   void StartScrubbing(double position);
#endif
   void SelectionHandleClick(wxMouseEvent &event,
                                     Track* pTrack, wxRect r);
   void StartSelection (int mouseXCoordinate, int trackLeftEdge);
   void ExtendSelection(int mouseXCoordinate, int trackLeftEdge,
                        Track *pTrack);
   void UpdateSelectionDisplay();

   // Handle small cursor and play head movements
   void SeekLeftOrRight
      (bool left, bool shift, bool ctrl, bool keyup,
       int snapToTime, bool mayAccelerateQuiet, bool mayAccelerateAudio,
       double quietSeekStepPositive, bool quietStepIsPixels,
       double audioSeekStepPositive, bool audioStepIsPixels);

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
public:
   void SnapCenterOnce (WaveTrack *pTrack, bool up);
protected:
   void StartSnappingFreqSelection (WaveTrack *pTrack);
   void MoveSnappingFreqSelection (int mouseYCoordinate,
                                   int trackTopEdge,
                                   int trackHeight, Track *pTrack);
   void StartFreqSelection (int mouseYCoordinate, int trackTopEdge,
                            int trackHeight, Track *pTrack);
   void ExtendFreqSelection(int mouseYCoordinate, int trackTopEdge,
                            int trackHeight);
   void ResetFreqSelectionPin(double hintFrequency, bool logF);

#endif

   void SelectTracksByLabel( LabelTrack *t );
   void SelectTrackLength(Track *t);

   // Helper for moving by keyboard with snap-to-grid enabled
   double GridMove(double t, int minPix);

   // AS: Cursor handling
   bool SetCursorByActivity( );
   void SetCursorAndTipWhenInLabel( Track * t, wxMouseEvent &event, const wxChar ** ppTip );
   void SetCursorAndTipWhenInVResizeArea( Track * label, bool blinked, const wxChar ** ppTip );
   void SetCursorAndTipWhenInLabelTrack( LabelTrack * pLT, wxMouseEvent & event, const wxChar ** ppTip );
   void SetCursorAndTipWhenSelectTool
      ( Track * t, wxMouseEvent & event, wxRect &r, bool bMultiToolMode, const wxChar ** ppTip, const wxCursor ** ppCursor );
   void SetCursorAndTipByTool( int tool, wxMouseEvent & event, const wxChar **ppTip );

   bool RecenterAt(wxCoord position);

#ifdef EXPERIMENTAL_FISHEYE
public:
   bool InFisheyeFocus(wxPoint position) const;

protected:
   // If ignoreFisheye is true, figure new center time from mouse position without fisheye.
   // Return true if really moved
   bool MoveFisheyeTo(wxCoord xx, bool ignoreFisheye);
   wxCoord mFisheyeCursorOffset;
   wxCoord mFisheyeClickPosition;

public:
   // Return true if really moved
   bool MoveFisheye();
   void RefreshFisheye();
protected:
#endif

   void HandleCursor(wxMouseEvent & event);
   void MaySetOnDemandTip( Track * t, const wxChar ** ppTip );

   // AS: Envelope editing handlers
   void HandleEnvelope(wxMouseEvent & event);
   void ForwardEventToTimeTrackEnvelope(wxMouseEvent & event);
   void ForwardEventToWaveTrackEnvelope(wxMouseEvent & event);
   void ForwardEventToEnvelope(wxMouseEvent &event);

   // AS: Track sliding handlers
   void HandleSlide(wxMouseEvent & event);
   void StartSlide(wxMouseEvent &event);
   void DoSlide(wxMouseEvent &event);
   void AddClipsToCaptured(Track *t, bool withinSelection);
   void AddClipsToCaptured(Track *t, double t0, double t1);

   // AS: Handle zooming into tracks
   void HandleZoom(wxMouseEvent & event);
   void HandleZoomClick(wxMouseEvent & event);
   void HandleZoomDrag(wxMouseEvent & event);
   void HandleZoomButtonUp(wxMouseEvent & event);

   bool IsDragZooming();
   void DragZoom(wxMouseEvent &event, int x);
   void DoZoomInOut(wxMouseEvent &event, int x);

   void HandleVZoom(wxMouseEvent & event);
   void HandleVZoomClick(wxMouseEvent & event);
   void HandleVZoomDrag(wxMouseEvent & event);
   void HandleVZoomButtonUp(wxMouseEvent & event);

   // Handle sample editing using the 'draw' tool.
   bool IsSampleEditingPossible( wxMouseEvent & event, Track * t );
   void HandleSampleEditing(wxMouseEvent & event);
   void HandleSampleEditingClick( wxMouseEvent & event );
   void HandleSampleEditingDrag( wxMouseEvent & event );
   void HandleSampleEditingButtonUp( wxMouseEvent & event );

   // MM: Handle mouse wheel rotation
   void HandleWheelRotation(wxMouseEvent & event);

   // Handle resizing.
   void HandleResizeClick(wxMouseEvent & event);
   void HandleResizeDrag(wxMouseEvent & event);
   void HandleResizeButtonUp(wxMouseEvent & event);
   void HandleResize(wxMouseEvent & event);

   void HandleLabelClick(wxMouseEvent & event);
   void HandleRearrange(wxMouseEvent & event);
   void CalculateRearrangingThresholds(wxMouseEvent & event);
   void HandleClosing(wxMouseEvent & event);
   void HandlePopping(wxMouseEvent & event);
   void HandleMutingSoloing(wxMouseEvent & event, bool solo);
   void HandleMinimizing(wxMouseEvent & event);
   void HandleSliders(wxMouseEvent &event, bool pan);


   // These *Func methods are used in TrackPanel::HandleLabelClick to set up
   // for actual handling in methods called by TrackPanel::OnMouseEvent, and
   // to draw button-down states, etc.
   bool CloseFunc(Track * t, wxRect r, int x, int y);
   bool PopupFunc(Track * t, wxRect r, int x, int y);

   // TrackSelFunc, unlike the other *Func methods, returns true if the click is not
   // set up to be handled, but click is on the sync-lock icon or the blank area to
   // the left of the minimize button, and we want to pass it forward to be a track select.
   bool TrackSelFunc(Track * t, wxRect r, int x, int y);

   bool MuteSoloFunc(Track *t, wxRect r, int x, int f, bool solo);
   bool MinimizeFunc(Track *t, wxRect r, int x, int f);
   bool GainFunc(Track * t, wxRect r, wxMouseEvent &event,
                 int x, int y);
   bool PanFunc(Track * t, wxRect r, wxMouseEvent &event,
                int x, int y);


   void MakeParentRedrawScrollbars();

   // AS: Pushing the state preserves state for Undo operations.
   void MakeParentPushState(wxString desc, wxString shortDesc,
                            int flags = PUSH_AUTOSAVE | PUSH_CALC_SPACE);
   void MakeParentModifyState(bool bWantsAutoSave);    // if true, writes auto-save file. Should set only if you really want the state change restored after
                                                               // a crash, as it can take many seconds for large (eg. 10 track-hours) projects
   void MakeParentResize();

   void OnSetName(wxCommandEvent &event);

   void OnSetFont(wxCommandEvent &event);

   void OnMoveTrack    (wxCommandEvent &event);
   void OnChangeOctave (wxCommandEvent &event);
   void OnChannelChange(wxCommandEvent &event);
   void OnSetDisplay   (wxCommandEvent &event);
   void OnSetTimeTrackRange (wxCommandEvent &event);
   void OnTimeTrackLin(wxCommandEvent &event);
   void OnTimeTrackLog(wxCommandEvent &event);
   void OnTimeTrackLogInt(wxCommandEvent &event);

   void SetMenuCheck( wxMenu & menu, int newId );
   void SetRate(Track *pTrack, double rate);
   void OnRateChange(wxCommandEvent &event);
   void OnRateOther(wxCommandEvent &event);

   void OnFormatChange(wxCommandEvent &event);

   void OnSwapChannels(wxCommandEvent &event);
   void OnSplitStereo(wxCommandEvent &event);
   void OnSplitStereoMono(wxCommandEvent &event);
   void SplitStereo(bool stereo);
   void OnMergeStereo(wxCommandEvent &event);
   void OnCutSelectedText(wxCommandEvent &event);
   void OnCopySelectedText(wxCommandEvent &event);
   void OnPasteSelectedText(wxCommandEvent &event);
   void OnDeleteSelectedLabel(wxCommandEvent &event);

   void SetTrackPan(Track * t, LWSlider * s);
   void SetTrackGain(Track * t, LWSlider * s);

   void RemoveTrack(Track * toRemove);

   // Find track info by coordinate
   Track *FindTrack(int mouseX, int mouseY, bool label, bool link,
                     wxRect * trackRect = NULL);

   wxRect FindTrackRect(Track * target, bool label);

   int GetVRulerWidth() const;
   int GetVRulerOffset() const { return mTrackInfo.GetTrackInfoWidth(); }

   int GetLabelWidth() const { return mTrackInfo.GetTrackInfoWidth() + GetVRulerWidth(); }

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
   void DrawTracks(wxDC * dc);

   void DrawEverythingElse(wxDC *dc, const wxRegion region,
                           const wxRect panelRect, const wxRect clip);
   void DrawOutside(Track *t, wxDC *dc, const wxRect rec,
                    const wxRect trackRect);
   void DrawZooming(wxDC* dc, const wxRect clip);

   void HighlightFocusedTrack (wxDC* dc, const wxRect r);
   void DrawShadow            (Track *t, wxDC* dc, const wxRect r);
   void DrawBordersAroundTrack(Track *t, wxDC* dc, const wxRect r, const int labelw, const int vrul);
   void DrawOutsideOfTrack    (Track *t, wxDC* dc, const wxRect r);

   int IdOfRate( int rate );
   int IdOfFormat( int format );

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   void UpdateVirtualStereoOrder();
#endif
   // Accessors...
   bool HasSoloButton(){  return mSoloPref!=wxT("None");}

   //JKC: These two belong in the label track.
   int mLabelTrackStartXPos;
   int mLabelTrackStartYPos;

   wxString TrackSubText(Track *t);

   bool MoveClipToTrack(WaveClip *clip, WaveTrack* dst);

   TrackInfo mTrackInfo;

   TrackPanelListener *mListener;

   TrackList *mTracks;
   ViewInfo *mViewInfo;

   AdornedRulerPanel *mRuler;

   double mSeekShort;
   double mSeekLong;

   TrackArtist *mTrackArtist;

   class AUDACITY_DLL_API AudacityTimer:public wxTimer {
   public:
     virtual void Notify() { parent->OnTimer(); }
     TrackPanel *parent;
   } mTimer;


   // This stores the parts of the screen that get overwritten by the indicator
   // and cursor
   double mLastIndicator;
   double mLastCursor;

   int mTimeCount;

   wxMemoryDC mBackingDC;
   wxBitmap *mBacking;
   bool mRefreshBacking;
   int mPrevWidth;
   int mPrevHeight;

   wxLongLong mLastSelectionAdjustment;

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
   Envelope *mCapturedEnvelope;
   WaveClip *mCapturedClip;
   TrackClipArray mCapturedClipArray;
   bool mCapturedClipIsSelection;
   WaveTrack::Location mCapturedTrackLocation;
   wxRect mCapturedTrackLocationRect;
   wxRect mCapturedRect;

   // When sliding horizontally, the moving clip may automatically
   // snap to the beginning and ending of other clips, or to label
   // starts and stops.  When you start sliding, SlideSnapFromPoints
   // gets populated with the start and stop times of selected clips,
   // and SlideSnapToPoints gets populated with the start and stop times
   // of other clips.  In both cases, times that are within 3 pixels
   // of another at the same zoom level are eliminated; you can't snap
   // when there are two things arbitrarily close at that zoom level.
   wxBaseArrayDouble mSlideSnapFromPoints;
   wxBaseArrayDouble mSlideSnapToPoints;
   wxArrayInt mSlideSnapLinePixels;

   // The amount that clips are sliding horizontally; this allows
   // us to undo the slide and then slide it by another amount
   double mHSlideAmount;

   bool mDidSlideVertically;

   bool mRedrawAfterStop;

   bool mIndicatorShowing;

   wxMouseEvent mLastMouseEvent;

   int mMouseClickX;
   int mMouseClickY;

   int mMouseMostRecentX;
   int mMouseMostRecentY;

   int mZoomStart;
   int mZoomEnd;

   // Handles snapping the selection boundaries or track boundaries to
   // line up with existing tracks or labels.  mSnapLeft and mSnapRight
   // are the horizontal index of pixels to display user feedback
   // guidelines so the user knows when such snapping is taking place.
   SnapManager *mSnapManager;
   wxInt64 mSnapLeft;
   wxInt64 mSnapRight;
   bool mSnapPreferRightEdge;

   NumericConverter mConverter;

   Track * mDrawingTrack;          // Keeps track of which track you are drawing on between events cf. HandleDraw()
   int mDrawingTrackTop;           // Keeps track of the top position of the drawing track.
   sampleCount mDrawingStartSample;   // sample of last click-down
   float mDrawingStartSampleValue;    // value of last click-down
   sampleCount mDrawingLastDragSample; // sample of last drag-over
   float mDrawingLastDragSampleValue;  // value of last drag-over

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   void HandleCenterFrequencyCursor
      (bool shiftDown, const wxChar ** ppTip, const wxCursor ** ppCursor);

   void HandleCenterFrequencyClick
      (bool shiftDown, Track *pTrack, double value);
#endif

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   double PositionToFrequency(bool maySnap,
                              wxInt64 mouseYCoordinate,
                              wxInt64 trackTopEdge,
                              int trackHeight,
                              double rate,
                              bool logF) const;
   wxInt64 FrequencyToPosition(double frequency,
                               wxInt64 trackTopEdge,
                               int trackHeight,
                               double rate,
                               bool logF) const;
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

   int mInitialTrackHeight;
   int mInitialUpperTrackHeight;
   bool mAutoScrolling;

   enum   MouseCaptureEnum
   {
      IsUncaptured=0,   // This is the normal state for the mouse
      IsVZooming,
      IsClosing,
      IsSelecting,
      IsAdjustingLabel,
      IsResizing,
      IsResizingBetweenLinkedTracks,
      IsResizingBelowLinkedTracks,
      IsRearranging,
      IsSliding,
      IsEnveloping,
      IsMuting,
      IsSoloing,
      IsGainSliding,
      IsPanSliding,
      IsMinimizing,
      IsOverCutLine,
      WasOverCutLine,
      IsPopping,
#ifdef USE_MIDI
      IsStretching,
#endif
      IsZooming,

#ifdef EXPERIMENTAL_FISHEYE
      IsAdjustingFisheye,
      IsCoarseAdjustingFisheye,
      IsFineAdjustingFisheye,
      IsRecenteringFisheye,
      IsUltraFineAdjustingFisheye,
#endif
   };

   enum MouseCaptureEnum mMouseCapture;
   void SetCapturedTrack( Track * t, enum MouseCaptureEnum MouseCapture=IsUncaptured );

   bool mAdjustSelectionEdges;
   bool mSlideUpDownOnly;
   bool mCircularTrackNavigation;
   float mdBr;

   // JH: if the user is dragging a track, at what y
   //   coordinate should the dragging track move up or down?
   int mMoveUpThreshold;
   int mMoveDownThreshold;

#ifdef EXPERIMENTAL_SCRUBBING
   bool mScrubbing;
   wxLongLong mLastScrubTime; // milliseconds
   double mLastScrubPosition;
#endif

   wxCursor *mArrowCursor;
   wxCursor *mPencilCursor;
   wxCursor *mSelectCursor;
   wxCursor *mResizeCursor;
   wxCursor *mSlideCursor;
   wxCursor *mEnvelopeCursor; // doubles as the center frequency cursor
                              // for spectral selection
   wxCursor *mSmoothCursor;
   wxCursor *mZoomInCursor;
   wxCursor *mZoomOutCursor;
   wxCursor *mLabelCursorLeft;
   wxCursor *mLabelCursorRight;
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

   wxMenu *mWaveTrackMenu;
   wxMenu *mNoteTrackMenu;
   wxMenu *mTimeTrackMenu;
   wxMenu *mLabelTrackMenu;
   wxMenu *mRateMenu;
   wxMenu *mFormatMenu;
   wxMenu *mLabelTrackInfoMenu;

   Track *mPopupMenuTarget;

   friend class TrackPanelAx;

   TrackPanelAx *mAx;

   wxString mSoloPref;

   // Keeps track of extra fractional vertical scroll steps
   double mVertScrollRemainder;

 protected:

   // The screenshot class needs to access internals
   friend class ScreenshotCommand;

 public:
   wxSize vrulerSize;

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

#define SMOOTHING_KERNEL_RADIUS 3
#define SMOOTHING_BRUSH_RADIUS 5
#define SMOOTHING_PROPORTION_MAX 0.7
#define SMOOTHING_PROPORTION_MIN 0.0

#endif

