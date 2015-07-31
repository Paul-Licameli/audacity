/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.cpp

  Dominic Mazzoni
  and lots of other contributors

  Implements TrackPanel and TrackInfo.

********************************************************************//*!

\todo
  Refactoring of the TrackPanel, possibly as described
  in \ref TrackPanelRefactor

*//*****************************************************************//*!

\file TrackPanel.cpp
\brief
  Implements TrackPanel and TrackInfo.

  TrackPanel.cpp is currently some of the worst code in Audacity.
  It's not really unreadable, there's just way too much stuff in this
  one file.  Rather than apply a quick fix, the long-term plan
  is to create a GUITrack class that knows how to draw itself
  and handle events.  Then this class just helps coordinate
  between tracks.

  Plans under discussion are described in \ref TrackPanelRefactor

*//********************************************************************/

// Documentation: Rather than have a lengthy \todo section, having
// a \todo a \file and a \page in EXACTLY that order gets Doxygen to
// put the following lengthy description of refactoring on a new page
// and link to it from the docs.

/*****************************************************************//**

\class TrackPanel
\brief
  The TrackPanel class coordinates updates and operations on the
  main part of the screen which contains multiple tracks.

  It uses many other classes, but in particular it uses the
  TrackInfo class to draw the controls area on the left of a track,
  and the TrackArtist class to draw the actual waveforms.

  Note that in some of the older code here, e.g., GetLabelWidth(),
  "Label" means the TrackInfo plus the vertical ruler.
  Confusing relative to LabelTrack labels.

  The TrackPanel manages multiple tracks and their TrackInfos.

  Note that with stereo tracks there will be one TrackInfo
  being used by two wavetracks.

*//*****************************************************************//**

\class TrackInfo
\brief
  The TrackInfo is shown to the side of a track
  It has the menus, pan and gain controls displayed in it.
  So "Info" is somewhat a misnomer. Should possibly be "TrackControls".

  TrackPanel and not TrackInfo takes care of the functionality for
  each of the buttons in that panel.

  In its current implementation TrackInfo is not derived from a
  wxWindow.  Following the original coding style, it has
  been coded as a 'flyweight' class, which is passed
  state as needed, except for the array of gains and pans.

  If we'd instead coded it as a wxWindow, we would have an instance
  of this class for each instance displayed.

*//**************************************************************//**

\class TrackPanelListener
\brief A now badly named class which is used to give access to a
subset of the TrackPanel methods from all over the place.

*//**************************************************************//**

\class TrackList
\brief A list of TrackListNode items.

*//**************************************************************//**

\class TrackListIterator
\brief An iterator for a TrackList.

*//**************************************************************//**

\class TrackListNode
\brief Used by TrackList, points to a Track.

*//**************************************************************//**

\class TrackPanel::AudacityTimer
\brief Timer class dedicated to infomring the TrackPanel that it
is time to refresh some aspect of the screen.

*//*****************************************************************//**

\page TrackPanelRefactor Track Panel Refactor
\brief Planned refactoring of TrackPanel.cpp

 - Move menus from current TrackPanel into TrackInfo.
 - Convert TrackInfo from 'flyweight' to heavyweight.
 - Split GuiStereoTrack and GuiWaveTrack out from TrackPanel.

  JKC: Incremental refactoring started April/2003

  Possibly aiming for Gui classes something like this - it's under
  discussion:

<pre>
   +----------------------------------------------------+
   |      AdornedRulerPanel                             |
   +----------------------------------------------------+
   +----------------------------------------------------+
   |+------------+ +-----------------------------------+|
   ||            | | (L)  GuiWaveTrack                 ||
   || TrackInfo  | +-----------------------------------+|
   ||            | +-----------------------------------+|
   ||            | | (R)  GuiWaveTrack                 ||
   |+------------+ +-----------------------------------+|
   +-------- GuiStereoTrack ----------------------------+
   +----------------------------------------------------+
   |+------------+ +-----------------------------------+|
   ||            | | (L)  GuiWaveTrack                 ||
   || TrackInfo  | +-----------------------------------+|
   ||            | +-----------------------------------+|
   ||            | | (R)  GuiWaveTrack                 ||
   |+------------+ +-----------------------------------+|
   +-------- GuiStereoTrack ----------------------------+
</pre>

  With the whole lot sitting in a TrackPanel which forwards
  events to the sub objects.

  The GuiStereoTrack class will do the special logic for
  Stereo channel grouping.

  The precise names of the classes are subject to revision.
  Have deliberately not created new files for the new classes
  such as AdornedRulerPanel and TrackInfo - yet.

*//*****************************************************************/

#include "Audacity.h"
#include "Experimental.h"
#include "TrackPanel.h"
#include "TrackPanelCell.h"
#include "TrackPanelCellIterator.h"
#include "TrackPanelMouseEvent.h"
#include "TrackPanelOverlay.h"

//#define DEBUG_DRAW_TIMING 1
// #define SPECTRAL_EDITING_ESC_KEY

#include "FreqWindow.h" // for SpectrumAnalyst

#include "AColor.h"
#include "AllThemeResources.h"
#include "AudioIO.h"
#include "float_cast.h"
#include "LabelTrack.h"
#include "MixerBoard.h"

#include "NoteTrack.h"
#include "NumberScale.h"
#include "Prefs.h"
#include "RefreshCode.h"
#include "Snap.h"
#include "ShuttleGui.h"
#include "TrackArtist.h"
#include "TrackPanelAx.h"
#include "UIHandle.h"
#include "HitTestResult.h"
#include "WaveTrack.h"

#include "commands/Keyboard.h"

#include "ondemand/ODManager.h"

#include "prefs/PrefsDialog.h"
#include "prefs/SpectrumPrefs.h"
#include "prefs/WaveformPrefs.h"

#include "toolbars/ControlToolBar.h"
#include "toolbars/ToolsToolBar.h"

// To do:  eliminate this!
#include "tracks/ui/Scrubbing.h"

#define ZOOMLIMIT 0.001f

//This loads the appropriate set of cursors, depending on platform.
#include "../images/Cursors.h"

DEFINE_EVENT_TYPE(EVT_TRACK_PANEL_TIMER)

/*

This is a diagram of TrackPanel's division of one (non-stereo) track rectangle.
Total height equals Track::GetHeight()'s value.  Total width is the wxWindow's width.
Each charater that is not . represents one pixel.

Inset space of this track, and top inset of the next track, are used to draw the focus highlight.

Top inset of the right channel of a stereo track, and bottom shadow line of the
left channel, are used for the channel separator.

TrackInfo::GetTrackInfoWidth() == GetVRulerOffset()
counts columns from the left edge up to and including controls, and is a constant.

GetVRulerWidth() is variable -- all tracks have the same ruler width at any time,
but that width may be adjusted when tracks change their vertical scales.

GetLabelWidth() counts columns up to and including the VRuler.
GetLeftOffset() is yet one more -- it counts the "one pixel" column.

FindTrack() for label returns a rectangle up to and including the One Pixel column,
but OMITS left and top insets

FindTrack() for !label returns a rectangle with x == GetLeftOffset(), and INCLUDES
right and top insets

+--------------- ... ------ ... --------------------- ...       ... -------------+
| Top Inset                                                                      |
|                                                                                |
|  +------------ ... ------ ... --------------------- ...       ... ----------+  |
| L|+-Border---- ... ------ ... --------------------- ...       ... -Border-+ |R |
| e||+---------- ... -++--- ... -+++----------------- ...       ... -------+| |i |
| f|B|                ||         |||                                       |BS|g |
| t|o| Controls       || V       |O|  The good stuff                       |oh|h |
|  |r|                || R       |n|                                       |ra|t |
| I|d|                || u       |e|                                       |dd|  |
| n|e|                || l       | |                                       |eo|I |
| s|r|                || e       |P|                                       |rw|n |
| e|||                || r       |i|                                       ||||s |
| t|||                ||         |x|                                       ||||e |
|  |||                ||         |e|                                       ||||t |
|  |||                ||         |l|                                       ||||  |
|  |||                ||         |||                                       ||||  |

.  ...                ..         ...                                       ....  .
.  ...                ..         ...                                       ....  .
.  ...                ..         ...                                       ....  .

|  |||                ||         |||                                       ||||  |
|  ||+----------     -++--  ... -+++----------------- ...       ... -------+|||  |
|  |+-Border---- ... -----  ... --------------------- ...       ... -Border-+||  |
|  |  Shadow---- ... -----  ... --------------------- ...       ... --Shadow-+|  |
*/
enum {
   kLeftInset = 4,
   kRightInset = kLeftInset,
   kTopInset = 4,
   kShadowThickness = 1,
   kBorderThickness = 1,
   kTopMargin = kTopInset + kBorderThickness,
   kBottomMargin = kShadowThickness + kBorderThickness,
   kLeftMargin = kLeftInset + kBorderThickness,
   kRightMargin = kRightInset + kShadowThickness + kBorderThickness,
};

// Is the distance between A and B less than D?
template < class A, class B, class DIST > bool within(A a, B b, DIST d)
{
   return (a > b - d) && (a < b + d);
}

template < class CLIPPEE, class CLIPVAL >
    void clip_top(CLIPPEE & clippee, CLIPVAL val)
{
   if (clippee > val)
      clippee = val;
}

template < class CLIPPEE, class CLIPVAL >
    void clip_bottom(CLIPPEE & clippee, CLIPVAL val)
{
   if (clippee < val)
      clippee = val;
}

enum {
   TrackPanelFirstID = 2000,

   OnChannelLeftID,
   OnChannelRightID,
   OnChannelMonoID,

   OnRate8ID,              // <---
   OnRate11ID,             //    |
   OnRate16ID,             //    |
   OnRate22ID,             //    |
   OnRate44ID,             //    |
   OnRate48ID,             //    | Leave these in order
   OnRate88ID,             //    |
   OnRate96ID,             //    |
   OnRate176ID,            //    | see OnTrackMenu()
   OnRate192ID,            //    |
   OnRate352ID,            //    |
   OnRate384ID,            //    |
   OnRateOtherID,          //    |
                           //    |
   On16BitID,              //    |
   On24BitID,              //    |
   OnFloatID,              // <---

   OnWaveformID,
   OnWaveformDBID,
   OnSpectrumID,
   OnSpectrogramSettingsID,

   OnSplitStereoID,
   OnSplitStereoMonoID,
   OnMergeStereoID,
   OnSwapChannelsID,

   // Reserve an ample block of ids for waveform scale types
   OnFirstWaveformScaleID,
   OnLastWaveformScaleID = OnFirstWaveformScaleID + 9,

   // Reserve an ample block of ids for spectrum scale types
   OnFirstSpectrumScaleID,
   OnLastSpectrumScaleID = OnFirstSpectrumScaleID + 19,

   OnZoomInVerticalID,
   OnZoomOutVerticalID,
   OnZoomFitVerticalID,
};

BEGIN_EVENT_TABLE(TrackPanel, wxWindow)
    EVT_MOUSE_EVENTS(TrackPanel::OnMouseEvent)
    EVT_MOUSE_CAPTURE_LOST(TrackPanel::OnCaptureLost)
    EVT_COMMAND(wxID_ANY, EVT_CAPTURE_KEY, TrackPanel::OnCaptureKey)
    EVT_KEY_DOWN(TrackPanel::OnKeyDown)
    EVT_KEY_UP(TrackPanel::OnKeyUp)
    EVT_CHAR(TrackPanel::OnChar)
    EVT_SIZE(TrackPanel::OnSize)
    EVT_PAINT(TrackPanel::OnPaint)
    EVT_SET_FOCUS(TrackPanel::OnSetFocus)
    EVT_KILL_FOCUS(TrackPanel::OnKillFocus)
    EVT_CONTEXT_MENU(TrackPanel::OnContextMenu)

    EVT_MENU_RANGE(OnChannelLeftID, OnChannelMonoID,
               TrackPanel::OnChannelChange)
    EVT_MENU_RANGE(OnWaveformID, OnSpectrumID, TrackPanel::OnSetDisplay)
    EVT_MENU(OnSpectrogramSettingsID, TrackPanel::OnSpectrogramSettings)
    EVT_MENU_RANGE(OnRate8ID, OnRate384ID, TrackPanel::OnRateChange)
    EVT_MENU_RANGE(On16BitID, OnFloatID, TrackPanel::OnFormatChange)
    EVT_MENU(OnRateOtherID, TrackPanel::OnRateOther)
    EVT_MENU(OnSwapChannelsID, TrackPanel::OnSwapChannels)
    EVT_MENU(OnSplitStereoID, TrackPanel::OnSplitStereo)
    EVT_MENU(OnSplitStereoMonoID, TrackPanel::OnSplitStereoMono)
    EVT_MENU(OnMergeStereoID, TrackPanel::OnMergeStereo)

    EVT_MENU_RANGE(OnFirstWaveformScaleID, OnLastWaveformScaleID, TrackPanel::OnWaveformScaleType)
    EVT_MENU_RANGE(OnFirstSpectrumScaleID, OnLastSpectrumScaleID, TrackPanel::OnSpectrumScaleType)

    EVT_MENU(OnZoomInVerticalID, TrackPanel::OnZoomInVertical)
    EVT_MENU(OnZoomOutVerticalID, TrackPanel::OnZoomOutVertical)
    EVT_MENU(OnZoomFitVerticalID, TrackPanel::OnZoomFitVertical)

    EVT_TIMER(wxID_ANY, TrackPanel::OnTimer)
END_EVENT_TABLE()

/// Makes a cursor from an XPM, uses CursorId as a fallback.
/// TODO:  Move this function to some other source file for reuse elsewhere.
wxCursor * MakeCursor( int WXUNUSED(CursorId), const char * pXpm[36],  int HotX, int HotY )
{
   wxCursor * pCursor;

#ifdef CURSORS_SIZE32
   const int HotAdjust =0;
#else
   const int HotAdjust =8;
#endif

   wxImage Image = wxImage(wxBitmap(pXpm).ConvertToImage());
   Image.SetMaskColour(255,0,0);
   Image.SetMask();// Enable mask.

   Image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_X, HotX-HotAdjust );
   Image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_Y, HotY-HotAdjust );
   pCursor = new wxCursor( Image );

   return pCursor;
}



// Don't warn us about using 'this' in the base member initializer list.
#ifndef __WXGTK__ //Get rid if this pragma for gtk
#pragma warning( disable: 4355 )
#endif
TrackPanel::TrackPanel(wxWindow * parent, wxWindowID id,
                       const wxPoint & pos,
                       const wxSize & size,
                       TrackList * tracks,
                       ViewInfo * viewInfo,
                       TrackPanelListener * listener,
                       AdornedRulerPanel * ruler)
   : wxPanel(parent, id, pos, size, wxWANTS_CHARS | wxNO_BORDER),
     mTrackInfo(this),
     mListener(listener),
     mTracks(tracks),
     mViewInfo(viewInfo),
     mRuler(ruler),
     mTrackArtist(NULL),
     mBacking(NULL),
     mResizeBacking(false),
     mRefreshBacking(false),
     mAutoScrolling(false),
     mVertScrollRemainder(0),
     vrulerSize(36,0)
     , mpClickedTrack(NULL)
     , mUIHandle(NULL)
     , mpBackground(NULL)
#ifndef __WXGTK__   //Get rid if this pragma for gtk
#pragma warning( default: 4355 )
#endif
{
   SetLabel(_("Track Panel"));
   SetName(_("Track Panel"));
   SetBackgroundStyle(wxBG_STYLE_PAINT);

   mAx = new TrackPanelAx( this );
#if wxUSE_ACCESSIBILITY
   SetAccessible( mAx );
#endif

   // Preinit the backing DC and bitmap so routines that require it will
   // not cause a crash if they run before the panel is fully initialized.
   mBacking = new wxBitmap(1, 1);
   mBackingDC.SelectObject(*mBacking);

   mMouseCapture = IsUncaptured;
   mLabelTrackStartXPos=-1;
   mCircularTrackNavigation = false;

   UpdatePrefs();

   mRedrawAfterStop = false;

   mSelectCursor  = MakeCursor( wxCURSOR_IBEAM,     IBeamCursorXpm,   17, 16);
   mEnvelopeCursor= MakeCursor( wxCURSOR_ARROW,     EnvCursorXpm,     16, 16);
   mDisabledCursor= MakeCursor( wxCURSOR_NO_ENTRY,  DisabledCursorXpm,16, 16);
   mZoomInCursor  = MakeCursor( wxCURSOR_MAGNIFIER, ZoomInCursorXpm,  19, 15);
   mZoomOutCursor = MakeCursor( wxCURSOR_MAGNIFIER, ZoomOutCursorXpm, 19, 15);
   
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   mBottomFrequencyCursor =
      MakeCursor( wxCURSOR_ARROW,  BottomFrequencyCursorXpm, 16, 16);
   mTopFrequencyCursor =
      MakeCursor( wxCURSOR_ARROW,  TopFrequencyCursorXpm, 16, 16);
   mBandWidthCursor = MakeCursor( wxCURSOR_ARROW,  BandWidthCursorXpm, 16, 16);
#endif

#if USE_MIDI
   mStretchMode = stretchCenter;
   mStretching = false;
   mStretched = false;
   mStretchStart = 0;
   mStretchCursor = MakeCursor( wxCURSOR_BULLSEYE,  StretchCursorXpm, 16, 16);
   mStretchLeftCursor = MakeCursor( wxCURSOR_BULLSEYE,
                                    StretchLeftCursorXpm, 16, 16);
   mStretchRightCursor = MakeCursor( wxCURSOR_BULLSEYE,
                                     StretchRightCursorXpm, 16, 16);
#endif

   mArrowCursor = new wxCursor(wxCURSOR_ARROW);
   mResizeCursor = new wxCursor(wxCURSOR_SIZENS);
   mRearrangeCursor = new wxCursor(wxCURSOR_HAND);
   mAdjustLeftSelectionCursor = new wxCursor(wxCURSOR_POINT_LEFT);
   mAdjustRightSelectionCursor = new wxCursor(wxCURSOR_POINT_RIGHT);

   mWaveTrackMenu = NULL;
   mChannelItemsInsertionPoint = 0;
   mShowMono = false;
   
   mRulerWaveformMenu = mRulerSpectrumMenu = NULL;

   BuildMenus();

   mTrackArtist = new TrackArtist();
   mTrackArtist->SetInset(1, kTopMargin, kRightMargin, kBottomMargin);

   mCapturedTrack = NULL;
   mPopupMenuTarget = NULL;

   mTimeCount = 0;
   mTimer.parent = this;
   mTimer.Start(kTimerInterval, FALSE);

   //More initializations, some of these for no other reason than
   //to prevent runtime memory check warnings
   mZoomStart = -1;
   mZoomEnd = -1;
   mPrevWidth = -1;
   mPrevHeight = -1;

   // This is used to snap the cursor to the nearest track that
   // lines up with it.
   mSnapManager = NULL;
   mSnapLeft = -1;
   mSnapRight = -1;

   // Register for tracklist updates
   mTracks->Connect(EVT_TRACKLIST_RESIZED,
                    wxCommandEventHandler(TrackPanel::OnTrackListResized),
                    NULL,
                    this);
   mTracks->Connect(EVT_TRACKLIST_UPDATED,
                    wxCommandEventHandler(TrackPanel::OnTrackListUpdated),
                    NULL,
                    this);

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   mFreqSelMode = FREQ_SEL_INVALID;
   mFrequencySnapper.reset(new SpectrumAnalyst());

   mLastF0 = mLastF1 = SelectedRegion::UndefinedFrequency;
#endif

   mSelStartValid = false;
   mSelStart = 0;

   mInitialTrackSelection = new std::vector<bool>;

   mOverlays = new std::vector<TrackPanelOverlay*>;
}


TrackPanel::~TrackPanel()
{
   mTimer.Stop();

   // Unregister for tracklist updates
   mTracks->Disconnect(EVT_TRACKLIST_UPDATED,
                       wxCommandEventHandler(TrackPanel::OnTrackListUpdated),
                       NULL,
                       this);
   mTracks->Disconnect(EVT_TRACKLIST_RESIZED,
                       wxCommandEventHandler(TrackPanel::OnTrackListResized),
                       NULL,
                       this);

   // This can happen if a label is being edited and the user presses
   // ALT+F4 or Command+Q
   if (HasCapture())
      ReleaseMouse();

   if (mBacking)
   {
      mBackingDC.SelectObject( wxNullBitmap );
      delete mBacking;
   }
   delete mTrackArtist;

   delete mArrowCursor;
   delete mSelectCursor;
   delete mEnvelopeCursor;
   delete mDisabledCursor;
   delete mResizeCursor;
   delete mZoomInCursor;
   delete mZoomOutCursor;
   delete mRearrangeCursor;
   delete mAdjustLeftSelectionCursor;
   delete mAdjustRightSelectionCursor;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   delete mBottomFrequencyCursor;
   delete mTopFrequencyCursor;
   delete mBandWidthCursor;
#endif
#if USE_MIDI
   delete mStretchCursor;
   delete mStretchLeftCursor;
   delete mStretchRightCursor;
#endif

   delete mSnapManager;

   DeleteMenus();

#if !wxUSE_ACCESSIBILITY
   delete mAx;
#endif

   delete mInitialTrackSelection;

   delete mOverlays;
}

namespace
{

   void ShowMonoItems(wxMenu *pMenu, size_t pos)
   {
      // Insert in the reverse of the sequence in which they appear
      pMenu->Insert(pos, OnMergeStereoID, _("Ma&ke Stereo Track"));
      pMenu->InsertRadioItem(pos, OnChannelRightID, _("R&ight Channel"));
      pMenu->InsertRadioItem(pos, OnChannelLeftID, _("&Left Channel"));
      pMenu->InsertRadioItem(pos, OnChannelMonoID, _("&Mono"));
   }

   void HideMonoItems(wxMenu *pMenu)
   {
      pMenu->Delete(OnChannelMonoID);
      pMenu->Delete(OnChannelLeftID);
      pMenu->Delete(OnChannelRightID);
      pMenu->Delete(OnMergeStereoID);
   }

   void ShowStereoItems(wxMenu *pMenu, size_t pos)
   {
      // Insert in the reverse of the sequence in which they appear
      pMenu->Insert(pos, OnSplitStereoMonoID, _("Split Stereo to &Mono"));
      pMenu->Insert(pos, OnSplitStereoID, _("Spl&it Stereo Track"));
      pMenu->Insert(pos, OnSwapChannelsID, _("Swap Stereo &Channels"));
   }

   void HideStereoItems(wxMenu *pMenu)
   {
      pMenu->Delete(OnSwapChannelsID);
      pMenu->Delete(OnSplitStereoID);
      pMenu->Delete(OnSplitStereoMonoID);
   }

}

void TrackPanel::BuildMenus(void)
{
   // Get rid of existing menus
   DeleteMenus();

   // Use AppendCheckItem so we can have ticks beside the items.
   // We would use AppendRadioItem but it only currently works on windows and GTK.
   mRateMenu = new wxMenu();
   mRateMenu->AppendRadioItem(OnRate8ID, wxT("8000 Hz"));
   mRateMenu->AppendRadioItem(OnRate11ID, wxT("11025 Hz"));
   mRateMenu->AppendRadioItem(OnRate16ID, wxT("16000 Hz"));
   mRateMenu->AppendRadioItem(OnRate22ID, wxT("22050 Hz"));
   mRateMenu->AppendRadioItem(OnRate44ID, wxT("44100 Hz"));
   mRateMenu->AppendRadioItem(OnRate48ID, wxT("48000 Hz"));
   mRateMenu->AppendRadioItem(OnRate88ID, wxT("88200 Hz"));
   mRateMenu->AppendRadioItem(OnRate96ID, wxT("96000 Hz"));
   mRateMenu->AppendRadioItem(OnRate176ID, wxT("176400 Hz"));
   mRateMenu->AppendRadioItem(OnRate192ID, wxT("192000 Hz"));
   mRateMenu->AppendRadioItem(OnRate352ID, wxT("352800 Hz"));
   mRateMenu->AppendRadioItem(OnRate384ID, wxT("384000 Hz"));
   mRateMenu->AppendRadioItem(OnRateOtherID, _("&Other..."));

   mFormatMenu = new wxMenu();
   mFormatMenu->AppendRadioItem(On16BitID, GetSampleFormatStr(int16Sample));
   mFormatMenu->AppendRadioItem(On24BitID, GetSampleFormatStr(int24Sample));
   mFormatMenu->AppendRadioItem(OnFloatID, GetSampleFormatStr(floatSample));

   /* build the pop-down menu used on wave (sampled audio) tracks */
   mWaveTrackMenu = new wxMenu();
   mWaveTrackMenu->AppendRadioItem(OnWaveformID, _("Wa&veform"));
   mWaveTrackMenu->AppendRadioItem(OnWaveformDBID, _("&Waveform (dB)"));
   mWaveTrackMenu->AppendRadioItem(OnSpectrumID, _("&Spectrogram"));
   mWaveTrackMenu->Append(OnSpectrogramSettingsID, _("S&pectrogram Settings..."));
   mWaveTrackMenu->AppendSeparator();

   mChannelItemsInsertionPoint = mWaveTrackMenu->GetMenuItemCount();
   ShowMonoItems(mWaveTrackMenu, mChannelItemsInsertionPoint);
   mShowMono = true;
   mWaveTrackMenu->AppendSeparator();

   mWaveTrackMenu->Append(0, _("&Format"), mFormatMenu);

   mWaveTrackMenu->AppendSeparator();

   mWaveTrackMenu->Append(0, _("&Rate"), mRateMenu);

   mRulerWaveformMenu = new wxMenu();
   BuildVRulerMenuItems
      (mRulerWaveformMenu, OnFirstWaveformScaleID,
       WaveformSettings::GetScaleNames());

   mRulerSpectrumMenu = new wxMenu();
   BuildVRulerMenuItems
      (mRulerSpectrumMenu, OnFirstSpectrumScaleID,
       SpectrogramSettings::GetScaleNames());
}

// static
void TrackPanel::BuildVRulerMenuItems
(wxMenu * menu, int firstId, const wxArrayString &names)
{
   int id = firstId;
   for (int ii = 0, nn = names.size(); ii < nn; ++ii)
      menu->AppendRadioItem(id++, names[ii]);
   menu->AppendSeparator();
   menu->Append(OnZoomInVerticalID, _("Zoom In\tLeft-Click/Left-Drag"));
   menu->Append(OnZoomOutVerticalID, _("Zoom Out\tShift-Left-Click"));
   menu->Append(OnZoomFitVerticalID, _("Zoom to Fit\tShift-Right-Click"));
}

void TrackPanel::DeleteMenus(void)
{
   // Note that the submenus (mRateMenu, ...)
   // are deleted by their parent
   if (mWaveTrackMenu) {
      delete mWaveTrackMenu;
      mWaveTrackMenu = NULL;
   }

   delete mRulerWaveformMenu;
   delete mRulerSpectrumMenu;
}

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
void TrackPanel::UpdateVirtualStereoOrder()
{
   TrackListIterator iter(mTracks);
   Track *t;
   int temp;

   for (t = iter.First(); t; t = iter.Next()) {
      if(t->GetKind() == Track::Wave && t->GetChannel() == Track::MonoChannel){
         WaveTrack *wt = (WaveTrack*)t;

         if(WaveTrack::mMonoAsVirtualStereo && wt->GetPan() != 0){
            temp = wt->GetHeight();
            wt->SetHeight(temp*wt->GetVirtualTrackPercentage());
            wt->SetHeight(temp - wt->GetHeight(),true);
         }else if(!WaveTrack::mMonoAsVirtualStereo && wt->GetPan() != 0){
            wt->SetHeight(wt->GetHeight() + wt->GetHeight(true));
         }
      }
   }
   t = iter.First();
   if(t){
      t->ReorderList(false);
   }
}
#endif

void TrackPanel::UpdatePrefs()
{
   gPrefs->Read(wxT("/GUI/AutoScroll"), &mViewInfo->bUpdateTrackIndicator,
      true);
   gPrefs->Read(wxT("/GUI/CircularTrackNavigation"), &mCircularTrackNavigation,
      false);
   gPrefs->Read(wxT("/GUI/Solo"), &mSoloPref, wxT("Standard") );

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   bool temp = WaveTrack::mMonoAsVirtualStereo;
   gPrefs->Read(wxT("/GUI/MonoAsVirtualStereo"), &WaveTrack::mMonoAsVirtualStereo,
      false);

   if(WaveTrack::mMonoAsVirtualStereo != temp)
      UpdateVirtualStereoOrder();
#endif

   mViewInfo->UpdatePrefs();

   if (mTrackArtist) {
      mTrackArtist->UpdatePrefs();
   }

   // All vertical rulers must be recalculated since the minimum and maximum
   // frequences may have been changed.
   UpdateVRulers();
   Refresh();
}

/// Remembers the track we clicked on and why we captured it.
/// We also use this method to clear the record
/// of the captured track, passing NULL as the track.
void TrackPanel::SetCapturedTrack( Track * t, enum MouseCaptureEnum MouseCapture )
{
#if defined(__WXDEBUG__)
   if (t == NULL) {
      wxASSERT(MouseCapture == IsUncaptured);
   }
   else {
      wxASSERT(MouseCapture != IsUncaptured);
   }
#endif
   mCapturedTrack = t;
   mMouseCapture = MouseCapture;
}

void TrackPanel::SelectNone()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      t->SetSelected(false);
      t = iter.Next();
   }
}

/// Select all tracks marked by the label track lt
void TrackPanel::SelectTracksByLabel( LabelTrack *lt )
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   //do nothing if at least one other track is selected
   while (t) {
      if( t->GetSelected() && t != lt )
         return;
      t = iter.Next();
   }

   //otherwise, select all tracks
   t = iter.First();
   while( t )
   {
      t->SetSelected( true );
      t = iter.Next();
   }
}

// Set selection length to the length of a track -- but if sync-lock is turned
// on, use the largest possible selection in the sync-lock group.
// If it's a stereo track, do the same for the stereo channels.
void TrackPanel::SelectTrackLength(Track *t)
{
   AudacityProject *p = GetActiveProject();
   SyncLockedTracksIterator it(mTracks);
   Track *t1 = it.First(t);
   double minOffset = t->GetOffset();
   double maxEnd = t->GetEndTime();

   // If we have a sync-lock group and sync-lock linking is on,
   // check the sync-lock group tracks.
   if (p->IsSyncLocked() && t1 != NULL)
   {
      for ( ; t1; t1 = it.Next())
      {
         if (t1->GetOffset() < minOffset)
            minOffset = t1->GetOffset();
         if (t1->GetEndTime() > maxEnd)
            maxEnd = t1->GetEndTime();
      }
   }
   else
   {
      // Otherwise, check for a stereo pair
      t1 = t->GetLink();
      if (t1)
      {
         if (t1->GetOffset() < minOffset)
            minOffset = t1->GetOffset();
         if (t1->GetEndTime() > maxEnd)
            maxEnd = t1->GetEndTime();
      }
   }

   // PRL: double click or click on track control.
   // should this select all frequencies too?  I think not.
   mViewInfo->selectedRegion.setTimes(minOffset, maxEnd);
}

void TrackPanel::GetTracksUsableArea(int *width, int *height) const
{
   GetSize(width, height);
   if (width) {
      *width -= GetLeftOffset();
      *width -= kRightMargin;
      *width = std::max(0, *width);
   }
}

namespace {
 
   wxRect UsableTrackRect(const wxRect &trackRect)
   {
      // Convert what FindTrack() returns to the "usable" part only.
      wxRect result = trackRect;
      result.width = std::max(0, result.width - kRightMargin);
      result.y += kTopMargin;
      result.height = std::max(0, result.height - (kTopMargin + kBottomMargin));
      return result;
   }

   wxRect UsableControlRect(const wxRect &controlRect)
   {
      return controlRect;
   }

   wxRect UsableVRulerRect(const wxRect &controlRect)
   {
      return controlRect;
   }

}

/// Gets the pointer to the AudacityProject that
/// goes with this track panel.
AudacityProject * TrackPanel::GetProject() const
{
   //JKC casting away constness here.
   //Do it in two stages in case 'this' is not a wxWindow.
   //when the compiler will flag the error.
   wxWindow const * const pConstWind = this;
   wxWindow * pWind=(wxWindow*)pConstWind;
#ifdef EXPERIMENTAL_NOTEBOOK
   pWind = pWind->GetParent(); //Page
   wxASSERT( pWind );
   pWind = pWind->GetParent(); //Notebook
   wxASSERT( pWind );
#endif
   pWind = pWind->GetParent(); //MainPanel
   wxASSERT( pWind );
   pWind = pWind->GetParent(); //Project
   wxASSERT( pWind );
   return (AudacityProject*)pWind;
}

/// AS: This gets called on our wx timer events.
void TrackPanel::OnTimer(wxTimerEvent& )
{
   mTimeCount++;
   // AS: If the user is dragging the mouse and there is a track that
   //  has captured the mouse, then scroll the screen, as necessary.
   if ((mMouseCapture==IsSelecting) && mCapturedTrack) {
      ScrollDuringDrag();
   }

   AudacityProject *const p = GetProject();

   // Check whether we were playing or recording, but the stream has stopped.
   if (p->GetAudioIOToken()>0 && !IsAudioActive())
   {
      //the stream may have been started up after this one finished (by some other project)
      //in that case reset the buttons don't stop the stream
      p->GetControlToolBar()->StopPlaying(!gAudioIO->IsStreamActive());
   }

   // Next, check to see if we were playing or recording
   // audio, but now Audio I/O is completely finished.
   if (p->GetAudioIOToken()>0 &&
         !gAudioIO->IsAudioTokenActive(p->GetAudioIOToken()))
   {
      p->FixScrollbars();
      p->SetAudioIOToken(0);
      p->RedrawProject();

      mRedrawAfterStop = false;

      //ANSWER-ME: Was DisplaySelection added to solve a repaint problem?
      DisplaySelection();
   }

   // Notify listeners for timer ticks
   {
      wxCommandEvent e(EVT_TRACK_PANEL_TIMER);
      p->GetEventHandler()->ProcessEvent(e);
   }

   DrawOverlays(false);

   if(IsAudioActive() && gAudioIO->GetNumCaptureChannels()) {

      // Periodically update the display while recording

      if (!mRedrawAfterStop) {
         mRedrawAfterStop = true;
         MakeParentRedrawScrollbars();
         mListener->TP_ScrollUpDown( 99999999 );
         Refresh( false );
      }
      else {
         if ((mTimeCount % 5) == 0) {
            // Must tell OnPaint() to recreate the backing bitmap
            // since we've not done a full refresh.
            mRefreshBacking = true;
            Refresh( false );
         }
      }
   }
   if(mTimeCount > 1000)
      mTimeCount = 0;
}

///  We check on each timer tick to see if we need to scroll.
///  Scrolling is handled by mListener, which is an interface
///  to the window TrackPanel is embedded in.
void TrackPanel::ScrollDuringDrag()
{
   // DM: If we're "autoscrolling" (which means that we're scrolling
   //  because the user dragged from inside to outside the window,
   //  not because the user clicked in the scroll bar), then
   //  the selection code needs to be handled slightly differently.
   //  We set this flag ("mAutoScrolling") to tell the selecting
   //  code that we didn't get here as a result of a mouse event,
   //  and therefore it should ignore the mouseEvent parameter,
   //  and instead use the last known mouse position.  Setting
   //  this flag also causes the Mac to redraw immediately rather
   //  than waiting for the next update event; this makes scrolling
   //  smoother on MacOS 9.

   if (mMouseMostRecentX >= mCapturedRect.x + mCapturedRect.width) {
      mAutoScrolling = true;
      mListener->TP_ScrollRight();
   }
   else if (mMouseMostRecentX < mCapturedRect.x) {
      mAutoScrolling = true;
      mListener->TP_ScrollLeft();
   }

   if (mAutoScrolling) {
      // AS: To keep the selection working properly as we scroll,
      //  we fake a mouse event (remember, this method is called
      //  from a timer tick).

      // AS: For some reason, GCC won't let us pass this directly.
      wxMouseEvent e(wxEVT_MOTION);
      HandleSelect(e);
      mAutoScrolling = false;
   }
}

double TrackPanel::GetScreenEndTime() const
{
   int width;
   GetTracksUsableArea(&width, NULL);
   return mViewInfo->PositionToTime(width, true);
}

/// OnSize() is called when the panel is resized
void TrackPanel::OnSize(wxSizeEvent & /* event */)
{
   // Tell OnPaint() to recreate the backing bitmap
   mResizeBacking = true;

   // Refresh the entire area.  Really only need to refresh when
   // expanding...is it worth the trouble?
   Refresh();
}

/// AS: OnPaint( ) is called during the normal course of
///  completing a repaint operation.
void TrackPanel::OnPaint(wxPaintEvent & /* event */)
{
#if DEBUG_DRAW_TIMING
   wxStopWatch sw;
#endif

   // Construct the paint DC on the heap so that it may be deleted
   // early
   wxPaintDC *dc = new wxPaintDC(this);

   // Retrieve the damage rectangle
   wxRect box = GetUpdateRegion().GetBox();

   // Recreate the backing bitmap if we have a full refresh
   // (See TrackPanel::Refresh())
   if (mRefreshBacking || (box == GetRect()))
   {
      // Reset (should a mutex be used???)
      mRefreshBacking = false;

      if (mResizeBacking)
      {
         // Reset
         mResizeBacking = false;

         // Delete the backing bitmap
         if (mBacking)
         {
            mBackingDC.SelectObject(wxNullBitmap);
            delete mBacking;
            mBacking = NULL;
         }

         wxSize sz = GetClientSize();
         mBacking = new wxBitmap();
         mBacking->Create(sz.x, sz.y); //, *dc);
         mBackingDC.SelectObject(*mBacking);
      }

      // Redraw the backing bitmap
      DrawTracks(&mBackingDC);

      // Copy it to the display
      dc->Blit(0, 0, mBacking->GetWidth(), mBacking->GetHeight(), &mBackingDC, 0, 0);
   }
   else
   {
      // Copy full, possibly clipped, damage rectangle
      dc->Blit(box.x, box.y, box.width, box.height, &mBackingDC, box.x, box.y);
   }

   // Done with the clipped DC
   delete dc;

   // Drawing now goes directly to the client area.  It can't use the paint DC
   // becuase the paint DC might be clipped and DrawOverlays() may need to draw
   // outside the clipped region.
   DrawOverlays(true);

#if DEBUG_DRAW_TIMING
   sw.Pause();
   wxLogDebug(wxT("Total: %ld milliseconds"), sw.Time());
   wxPrintf(wxT("Total: %ld milliseconds\n"), sw.Time());
#endif
}

/// Makes our Parent (well, whoever is listening to us) push their state.
/// this causes application state to be preserved on a stack for undo ops.
void TrackPanel::MakeParentPushState(wxString desc, wxString shortDesc,
                                     int flags)
{
   mListener->TP_PushState(desc, shortDesc, flags);
}

void TrackPanel::MakeParentModifyState(bool bWantsAutoSave)
{
   mListener->TP_ModifyState(bWantsAutoSave);
}

void TrackPanel::MakeParentRedrawScrollbars()
{
   mListener->TP_RedrawScrollbars();
}

namespace
{
   void ProcessUIHandleResult
      (TrackPanel *panel, Track *pClickedTrack, Track *pLatestTrack,
      UIHandle::Result refreshResult)
   {
      // TODO:  make a finer distinction between refreshing the track control area,
      // and the waveform area.  As it is, redraw both whenever you must redraw either.

      using namespace RefreshCode;

      panel->UpdateViewIfNoTracks();

      if (refreshResult & DestroyedCell) {
         // Beware stale pointer!
         if (pLatestTrack == pClickedTrack)
            pLatestTrack = NULL;
         pClickedTrack = NULL;
      }

      if (pClickedTrack && (refreshResult & UpdateVRuler))
         panel->UpdateVRuler(pClickedTrack);

      // Refresh all if told to do so, or if told to refresh a track that
      // is not known.
      const bool refreshAll =
         (    (refreshResult & RefreshAll)
          || ((refreshResult & RefreshCell) && !pClickedTrack)
          || ((refreshResult & RefreshLatestCell) && !pLatestTrack));

      if (refreshAll)
         panel->Refresh(false);
      else {
         if (refreshResult & RefreshCell)
            panel->RefreshTrack(pClickedTrack);
         if (refreshResult & RefreshLatestCell)
            panel->RefreshTrack(pLatestTrack);
      }

      if (refreshResult & FixScrollbars)
         panel->MakeParentRedrawScrollbars();

      if (refreshResult & Resize)
         panel->GetListener()->TP_HandleResize();

      // This flag is superfluouos if you do full refresh,
      // because TrackPanel::Refresh() does this too
      if (refreshResult & UpdateSelection) {
         panel->DisplaySelection();

         {
            // Formerly in TrackPanel::UpdateSelectionDisplay():

            // Make sure the ruler follows suit.
            // mRuler->DrawSelection();

            // ... but that too is superfluous it does nothing but refres
            // the ruler, while DisplaySelection calls TP_DisplaySelection which
            // also always refreshes the ruler.
         }
      }

      if ((refreshResult & EnsureVisible) && pClickedTrack)
         panel->EnsureVisible(pClickedTrack);
   }
}

void TrackPanel::CancelDragging()
{
   if (mUIHandle) {
      UIHandle::Result refreshResult = mUIHandle->Cancel(GetProject());
      {
         // TODO: avoid dangling pointers to mpClickedTrack
         // when the undo stack management of the typical Cancel override
         // causes it to relocate.  That is implement some means to
         // re-fetch the track according to its position in the list.
         mpClickedTrack = NULL;
      }
      ProcessUIHandleResult(this, mpClickedTrack, NULL, refreshResult);
      mpClickedTrack = NULL;
      mUIHandle = NULL;
      if (HasCapture())
         ReleaseMouse();
      wxMouseEvent dummy;
      HandleCursor(dummy);
   }
}

void TrackPanel::HandleEscapeKey(bool down)
{
   if (!down)
      return;

   if (mUIHandle) {
      // UIHANDLE CANCEL
      CancelDragging();
      return;
   }
   else switch (mMouseCapture) {
   case IsSelecting:
   {
      TrackListIterator iter(mTracks);
      std::vector<bool>::const_iterator
         it = mInitialTrackSelection->begin(),
         end = mInitialTrackSelection->end();
      for (Track *t = iter.First(); t; t = iter.Next()) {
         wxASSERT(it != end);
         t->SetSelected(*it++);
      }
      mViewInfo->selectedRegion = mInitialSelection;
   }
      break;
   case IsVZooming:
   //case IsAdjustingSample:
      break;
   case IsResizing:
      mCapturedTrack->SetHeight(mInitialActualHeight);
      mCapturedTrack->SetMinimized(mInitialMinimized);
      break;
   case IsResizingBetweenLinkedTracks:
   {
      Track *const next = mTracks->GetNext(mCapturedTrack);
      mCapturedTrack->SetHeight(mInitialUpperActualHeight);
      mCapturedTrack->SetMinimized(mInitialMinimized);
      next->SetHeight(mInitialActualHeight);
      next->SetMinimized(mInitialMinimized);
   }
      break;
   case IsResizingBelowLinkedTracks:
   {
      Track *const prev = mTracks->GetPrev(mCapturedTrack);
      mCapturedTrack->SetHeight(mInitialActualHeight);
      mCapturedTrack->SetMinimized(mInitialMinimized);
      prev->SetHeight(mInitialUpperActualHeight);
      prev->SetMinimized(mInitialMinimized);
   }
      break;
   default:
      return;
   }

   // Common part in all cases that do anything
   SetCapturedTrack(NULL, IsUncaptured);
   if (HasCapture())
      ReleaseMouse();
   wxMouseEvent dummy;
   HandleCursor(dummy);
   Refresh(false);
}

void TrackPanel::HandleAltKey(bool down)
{
   mLastMouseEvent.m_altDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandleShiftKey(bool down)
{
   mLastMouseEvent.m_shiftDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandleControlKey(bool down)
{
   mLastMouseEvent.m_controlDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandlePageUpKey()
{
   mListener->TP_ScrollWindow(GetScreenEndTime());
}

void TrackPanel::HandlePageDownKey()
{
   mListener->TP_ScrollWindow(2 * mViewInfo->h - GetScreenEndTime());
}

void TrackPanel::HandleCursorForLastMouseEvent()
{
   HandleCursor(mLastMouseEvent);
}

MixerBoard* TrackPanel::GetMixerBoard()
{
   AudacityProject *p = GetProject();
   wxASSERT(p);
   return p->GetMixerBoard();
}

/// Used to determine whether it is safe or not to perform certain
/// edits at the moment.
/// @return true if audio is being recorded or is playing.
bool TrackPanel::IsUnsafe()
{
   return IsAudioActive();
}

bool TrackPanel::IsAudioActive()
{
   AudacityProject *p = GetProject();
   return p->IsAudioActive();
}


/// Uses a previously noted 'activity' to determine what
/// cursor to use.
/// @var mMouseCapture holds the current activity.
bool TrackPanel::SetCursorByActivity( )
{
   bool unsafe = IsUnsafe();

   switch( mMouseCapture )
   {
   case IsSelecting:
      SetCursor(*mSelectCursor);
      return true;
   case IsRearranging:
      SetCursor( unsafe ? *mDisabledCursor : *mRearrangeCursor);
      return true;
   case IsAdjustingLabel:
   case IsSelectingLabelText:
      return true;
#if 0
   case IsStretching:
      SetCursor( unsafe ? *mDisabledCursor : *mStretchCursor);
      return true;
#endif
   default:
      break;
   }
   return false;
}

/// When in the "label" (TrackInfo or vertical ruler), we can either vertical zoom or re-order tracks.
/// Dont't change cursor/tip to zoom if display is not waveform (either linear of dB) or Spectrum
void TrackPanel::SetCursorAndTipWhenInLabel( Track * t,
         wxMouseEvent &event, wxString &tip )
{
   if (event.m_x >= GetVRulerOffset() && (t->GetKind() == Track::Wave) )
   {
      tip = _("Click to vertically zoom in. Shift-click to zoom out. Drag to specify a zoom region.");
      SetCursor(event.ShiftDown()? *mZoomOutCursor : *mZoomInCursor);
   }
#ifdef USE_MIDI
   else if (event.m_x >= GetVRulerOffset() && t->GetKind() == Track::Note) {
      tip = _("Click to verticaly zoom in, Shift-click to zoom out, Drag to create a particular zoom region.");
      SetCursor(event.ShiftDown() ? *mZoomOutCursor : *mZoomInCursor);
   }
#endif
   else {
      // Set a status message if over TrackInfo.
      tip = _("Drag the track vertically to change the order of the tracks.");
      SetCursor(*mArrowCursor);
   }
}

/// When in the resize area we can adjust size or relative size.
void TrackPanel::SetCursorAndTipWhenInVResizeArea( bool bLinked, wxString &tip )
{
   // Check to see whether it is the first channel of a stereo track
   if (bLinked) {
      // If we are in the label we got here 'by mistake' and we're
      // not actually in the resize area at all.  (The resize area
      // is shorter when it is between stereo tracks).

      tip = _("Click and drag to adjust relative size of stereo tracks.");
      SetCursor(*mResizeCursor);
   } else {
      tip = _("Click and drag to resize the track.");
      SetCursor(*mResizeCursor);
   }
}

/// When in a label track, find out if we've hit anything that
/// would cause a cursor change.
void TrackPanel::SetCursorAndTipWhenInLabelTrack( LabelTrack * pLT,
       wxMouseEvent & event, wxString &tip )
{
   int edge=pLT->OverGlyph(event.m_x, event.m_y);
   if(edge !=0)
   {
      SetCursor(*mArrowCursor);
   }

   //KLUDGE: We refresh the whole Label track when the icon hovered over
   //changes colouration.  As well as being inefficient we are also
   //doing stuff that should be delegated to the label track itself.
   edge += pLT->mbHitCenter ? 4:0;
   if( edge != pLT->mOldEdge )
   {
      pLT->mOldEdge = edge;
      RefreshTrack( pLT );
   }
   // IF edge!=0 THEN we've set the cursor and we're done.
   // signal this by setting the tip.
   if( edge != 0 )
   {
      tip =
         (pLT->mbHitCenter ) ?
         _("Drag one or more label boundaries.") :
         _("Drag label boundary.");
   }
}

namespace {

// This returns true if we're a spectral editing track.
inline bool isSpectralSelectionTrack(const Track *pTrack) {
   if (pTrack &&
       pTrack->GetKind() == Track::Wave) {
      const WaveTrack *const wt = static_cast<const WaveTrack*>(pTrack);
      const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
      const int display = wt->GetDisplay();
      return (display == WaveTrack::Spectrum) && settings.SpectralSelectionEnabled();
   }
   else {
      return false;
   }
}

} // namespace

// If we're in OnDemand mode, we may change the tip.
void TrackPanel::MaySetOnDemandTip( Track * t, wxString &tip )
{
   wxASSERT( t );
   //For OD regions, we need to override and display the percent complete for this task.
   //first, make sure it's a wavetrack.
   if(t->GetKind() != Track::Wave)
      return;
   //see if the wavetrack exists in the ODManager (if the ODManager exists)
   if(!ODManager::IsInstanceCreated())
      return;
   //ask the wavetrack for the corresponding tip - it may not change tip, but that's fine.
   ODManager::Instance()->FillTipForWaveTrack(static_cast<WaveTrack*>(t), tip);
   return;
}

namespace
{
   TrackPanelCell *FindNonTrackCell
      (const wxMouseEvent &event, Track *pTrack, int vRulerOffset,
       const wxRect &outer, const wxRect &outer2, wxRect &inner)
   {
      const bool inRuler = event.m_x >= vRulerOffset;
      TrackPanelCell *const pCell =
         inRuler ? pTrack->GetVRulerControl() : pTrack->GetTrackControl();
      if (inRuler)
         inner = outer2,
         inner.width -= vRulerOffset - inner.x,
         inner.x = vRulerOffset,
         inner = UsableVRulerRect(inner);
      else
         // Subtract VRuler from the rectangle
         inner = outer,
         inner.width = vRulerOffset - inner.x,
         inner = UsableControlRect(inner);
      return pCell;
   }
}

void TrackPanel::FindCell
(const wxMouseEvent &event,
 wxRect &inner, TrackPanelCell *& pCell, Track *& pTrack)
{
   wxRect rect;
   inner = rect;
   pCell = mpBackground;
   pTrack = FindTrack(event.m_x, event.m_y, false, false, &rect);
   if (pTrack) {
      pCell = pTrack;
      inner = UsableTrackRect(rect);
   }
   else {
      pTrack = FindTrack(event.m_x, event.m_y, true, true, &rect);
      if (pTrack) {
         wxRect rect2;
         FindTrack(event.m_x, event.m_y, true, false, &rect2);
         pCell =
            FindNonTrackCell(event, pTrack, GetVRulerOffset(), rect, rect2, inner);
      }
   }
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
void TrackPanel::HandleCenterFrequencyCursor
(bool shiftDown, wxString &tip, const wxCursor ** ppCursor)
{
#ifndef SPECTRAL_EDITING_ESC_KEY
   tip =
      shiftDown ?
      _("Click and drag to move center selection frequency.") :
      _("Click and drag to move center selection frequency to a spectral peak.");

#else
   shiftDown;

   tip =
      _("Click and drag to move center selection frequency.");

#endif

   *ppCursor = mEnvelopeCursor;
}

void TrackPanel::HandleCenterFrequencyClick
(bool shiftDown, Track *pTrack, double value)
{
   if (shiftDown) {
      // Disable time selection
      mSelStartValid = false;
      mFreqSelTrack = static_cast<WaveTrack*>(pTrack);
      mFreqSelPin = value;
      mFreqSelMode = FREQ_SEL_DRAG_CENTER;
   }
   else {
#ifndef SPECTRAL_EDITING_ESC_KEY
      // Start center snapping
      WaveTrack *wt = static_cast<WaveTrack*>(pTrack);
      // Turn center snapping on (the only way to do this)
      mFreqSelMode = FREQ_SEL_SNAPPING_CENTER;
      // Disable time selection
      mSelStartValid = false;
      StartSnappingFreqSelection(wt);
#endif
   }
}
#endif

// The select tool can have different cursors and prompts depending on what
// we hover over, most notably when hovering over the selction boundaries.
// Determine and set the cursor and tip accordingly.
void TrackPanel::SetCursorAndTipWhenSelectTool( Track * t,
        wxMouseEvent & event, wxRect &rect, bool bMultiToolMode,
        wxString &tip, const wxCursor ** ppCursor )
{
   // Do not set the default cursor here and re-set later, that causes
   // flashing.
   *ppCursor = mSelectCursor;

   //In Multi-tool mode, give multitool prompt if no-special-hit.
   if( bMultiToolMode ) {
      // Look up the current key binding for Preferences.
      // (Don't assume it's the default!)
      wxString keyStr
         (GetProject()->GetCommandManager()->GetKeyFromName(wxT("Preferences")));
      if (keyStr.IsEmpty())
         // No keyboard preference defined for opening Preferences dialog
         /* i18n-hint: These are the names of a menu and a command in that menu */
         keyStr = _("Edit, Preferences...");
      else
         keyStr = KeyStringDisplay(keyStr);
      /* i18n-hint: %s is usually replaced by "Ctrl+P" for Windows/Linux, "Command+," for Mac */
      tip = wxString::Format(
         _("Multi-Tool Mode: %s for Mouse and Keyboard Preferences."),
         keyStr.c_str());
      // Later in this function we may point to some other string instead.
   }

   // Not over a track?  Get out of here.
   if(!t)
      return;

   //Make sure we are within the selected track
   // Adjusting the selection edges can be turned off in
   // the preferences...
   if ( !t->GetSelected() || !mViewInfo->bAdjustSelectionEdges)
   {
      MaySetOnDemandTip( t, tip );
      return;
   }

   {
      wxInt64 leftSel = mViewInfo->TimeToPosition(mViewInfo->selectedRegion.t0(), rect.x);
      wxInt64 rightSel = mViewInfo->TimeToPosition(mViewInfo->selectedRegion.t1(), rect.x);
      // Something is wrong if right edge comes before left edge
      wxASSERT(!(rightSel < leftSel));
   }

   const bool bShiftDown = event.ShiftDown();

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   if ( (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER) &&
      isSpectralSelectionTrack(t)) {
      // Not shift-down, but center frequency snapping toggle is on
      tip = _("Click and drag to set frequency bandwidth.");
      *ppCursor = mEnvelopeCursor;
      return;
   }
#endif

   // If not shift-down and not snapping center, then
   // choose boundaries only in snapping tolerance,
   // and may choose center.
   // But don't change the cursor when scrubbing.
   SelectionBoundary boundary =
#ifdef EXPERIMENTAL_SCRUBBING_BASIC
      GetProject()->GetScrubber().IsScrubbing()
      ? SBNone
      :
#endif
        ChooseBoundary(event, t, rect, !bShiftDown, !bShiftDown);

#ifdef USE_MIDI
   // The MIDI HitTest will only succeed if we are on a midi track, so 
   // typically we will fall through.
   switch( boundary) {
      case SBNone:
      case SBLeft:
      case SBRight:
         if ( HitTestStretch(t, rect, event)) {
            tip = _("Click and drag to stretch within selected region.");
            *ppCursor = mStretchCursor;
            return;
         }
         break;
      default:
         break;
   }
#endif

   switch (boundary) {
      case SBNone:
         if( bShiftDown ){
            // wxASSERT( false );
            // Same message is used for moving left right top or bottom edge.
            tip = _("Click to move selection boundary to cursor.");
            // No cursor change.
            return;
         }
         break;
      case SBLeft:
         tip = _("Click and drag to move left selection boundary.");
         *ppCursor = mAdjustLeftSelectionCursor;
         return;
      case SBRight:
         tip = _("Click and drag to move right selection boundary.");
         *ppCursor = mAdjustRightSelectionCursor;
         return;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      case SBBottom:
         tip = _("Click and drag to move bottom selection frequency.");
         *ppCursor = mBottomFrequencyCursor;
         return;
      case SBTop:
         tip = _("Click and drag to move top selection frequency.");
         *ppCursor = mTopFrequencyCursor;
         return;
      case SBCenter:
         HandleCenterFrequencyCursor(bShiftDown, tip, ppCursor);
         return;
      case SBWidth:
         tip = _("Click and drag to adjust frequency bandwidth.");
         *ppCursor = mBandWidthCursor;
         return;
#endif
      default:
         wxASSERT(false);
   } // switch
   // Falls through the switch if there was no boundary found.

   MaySetOnDemandTip( t, tip );
}

/// In this method we know what tool we are using,
/// so set the cursor accordingly.
void TrackPanel::SetCursorAndTipByTool( int tool,
         wxMouseEvent &, wxString& )
{
   // Change the cursor based on the active tool.
   switch (tool) {
   case selectTool:
      wxFAIL;// should have already been handled
      break;
   }
   // doesn't actually change the tip itself, but it could (should?) do at some
   // future date.
}

///  TrackPanel::HandleCursor( ) sets the cursor drawn at the mouse location.
///  As this procedure checks which region the mouse is over, it is
///  appropriate to establish the message in the status bar.
void TrackPanel::HandleCursor(wxMouseEvent & event)
{
   mLastMouseEvent = event;

   // (1), If possible, set the cursor based on the current activity
   //      ( leave the StatusBar alone ).
   if( SetCursorByActivity() )
      return;

   // (2) If we are not over a track at all, set the cursor to Arrow and
   //     clear the StatusBar,

   wxRect inner;
   TrackPanelCell *pCell;
   Track *pTrack;
   wxCursor *pCursor = NULL;
   FindCell(event, inner, pCell, pTrack);
   const bool inLabel = (pTrack != pCell);

   if (!pTrack) {
      SetCursor(*mArrowCursor);
      mListener->TP_DisplayStatusMessage(wxT(""));
      return;
   }

   // (3) The easy cases are done.
   // Now we've got to hit-test against a number of different possibilities.
   // We could be over the label or a vertical ruler etc...

   // Strategy here is to set the tip when we have determined and
   // set the correct cursor.  We stop doing tests for what we've
   // hit once the tip is not NULL.

   wxString tip;

   // Are we within the vertical resize area?
   // (Note: add bottom border thickness back to inner rectangle)
   if (within(event.m_y, inner.y + inner.height + kBorderThickness, TRACK_RESIZE_REGION))
      SetCursorAndTipWhenInVResizeArea(!inLabel && pTrack->GetLinked(), tip);
      // tip may still be NULL at this point, in which case we go on looking.

   // Otherwise, we must be over a track of some kind
   if (pCursor == NULL && tip == wxString()) {
      HitTestResult hitTest(pCell->HitTest
         (TrackPanelMouseEvent(event, inner, pCell), GetProject()));
      tip = hitTest.preview.message;
      ProcessUIHandleResult(this, pTrack, pTrack, hitTest.preview.refreshCode);
      pCursor = hitTest.preview.cursor;
      if (pCursor)
         SetCursor(*pCursor);
      else if (inLabel)
         SetCursorAndTipWhenInLabel(pTrack, event, tip);
   }

   // Is it a label track?
   if (pCursor == NULL && tip == wxString() && pTrack->GetKind() == Track::Label)
   {
      // We are over a label track
      SetCursorAndTipWhenInLabelTrack( static_cast<LabelTrack*>(pTrack), event, tip );
      // ..and if we haven't yet determined the cursor,
      // we go on to do all the standard track hit tests.
   }

   if( pCursor == NULL && tip == wxString() )
   {
      ToolsToolBar * ttb = mListener->TP_GetToolsToolBar();
      if( ttb == NULL )
         return;
      // JKC: DetermineToolToUse is called whenever the mouse
      // moves.  I had some worries about calling it when in
      // multimode as it then has to hit-test all 'objects' in
      // the track panel, but performance seems fine in
      // practice (on a P500).
      int tool = DetermineToolToUse( ttb, event );

      tip = ttb->GetMessageForTool(tool);

      // We don't include the select tool in
      // SetCursorAndTipByTool() because it's more complex than
      // the other tool cases.
      if( tool != selectTool )
      {
         SetCursorAndTipByTool( tool, event, tip);
      }
      else
      {
         bool bMultiToolMode = ttb->IsDown(multiTool);
         const wxCursor *pSelection = 0;
         SetCursorAndTipWhenSelectTool
            ( pTrack, event, inner, bMultiToolMode, tip, &pSelection );
         if (pSelection)
            // Set cursor once only here, to avoid flashing during drags
            SetCursor(*pSelection);
      }
   }

   if (pCursor != NULL || tip != wxString())
      mListener->TP_DisplayStatusMessage(tip);
}


/// This method handles various ways of starting and extending
/// selections.  These are the selections you make by clicking and
/// dragging over a waveform.
void TrackPanel::HandleSelect(wxMouseEvent & event)
{
   wxRect rect;
   Track *t = FindTrack(event.m_x, event.m_y, false, false, &rect);

   // AS: Ok, did the user just click the mouse, release the mouse,
   //  or drag?
   if (event.LeftDown() ||
      (event.LeftDClick() && event.CmdDown())) {
      // AS: Now, did they click in a track somewhere?  If so, we want
      //  to extend the current selection or start a new selection,
      //  depending on the shift key.  If not, cancel all selections.
      if (t)
         SelectionHandleClick(event, t, rect);
   } else if (event.LeftUp() || event.RightUp()) {
      if (mSnapManager) {
         delete mSnapManager;
         mSnapManager = NULL;
      }
      // Do not draw yellow lines
      if (mSnapLeft != -1 || mSnapRight != -1) {
         mSnapLeft = mSnapRight = -1;
         Refresh(false);
      }

      SetCapturedTrack( NULL );
      //Send the new selection state to the undo/redo stack:
      MakeParentModifyState(false);

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      // This stops center snapping with mouse movement
      mFreqSelMode = FREQ_SEL_INVALID;
#endif

   } else if (event.LeftDClick() && !event.ShiftDown()
#ifdef EXPERIMENTAL_SCRUBBING_SMOOTH_SCROLL
      && !event.CmdDown()
#endif
      ) {
      if (!mCapturedTrack) {
         wxRect rect;
         mCapturedTrack =
            FindTrack(event.m_x, event.m_y, false, false, &rect);
         if (!mCapturedTrack)
            return;
      }

      // Deselect all other tracks and select this one.
      SelectNone();

      mTracks->Select(mCapturedTrack);

      // Default behavior: select whole track
      SelectTrackLength(mCapturedTrack);

      // Special case: if we're over a clip in a WaveTrack,
      // select just that clip
      if (mCapturedTrack->GetKind() == Track::Wave) {
         WaveTrack *w = (WaveTrack *)mCapturedTrack;
         WaveClip *selectedClip = w->GetClipAtX(event.m_x);
         if (selectedClip) {
            mViewInfo->selectedRegion.setTimes(
               selectedClip->GetOffset(), selectedClip->GetEndTime());
         }

         Refresh(false);
         goto done;
      }

      Refresh(false);
      SetCapturedTrack( NULL );
      MakeParentModifyState(false);
   }
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
#ifdef SPECTRAL_EDITING_ESC_KEY
   else if (!event.IsButton() &&
            mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
            !mViewInfo->selectedRegion.isPoint())
      MoveSnappingFreqSelection(event.m_y, rect.y, rect.height, t);
#endif
#endif
 done:
   SelectionHandleDrag(event, t);
}

/// This method gets called when we're handling selection
/// and the mouse was just clicked.
void TrackPanel::SelectionHandleClick(wxMouseEvent & event,
                                      Track * pTrack, wxRect rect)
{
   Track *rightTrack = NULL;
   mCapturedTrack = pTrack;
   rect.y += kTopMargin;
   rect.height -= kTopMargin + kBottomMargin;
   mCapturedRect = rect;

   mMouseCapture=IsSelecting;
   mInitialSelection = mViewInfo->selectedRegion;

   // Save initial state of track selections, also,
   // if the shift button is down and no track is selected yet,
   // at least select the track we clicked into.
   bool isAtLeastOneTrackSelected = false;
   mInitialTrackSelection->clear();
   {
      bool nextTrackIsLinkFromPTrack = false;
      TrackListIterator iter(mTracks);
      for (Track *t = iter.First(); t; t = iter.Next()) {
         const bool isSelected = t->GetSelected();
         mInitialTrackSelection->push_back(isSelected);
         if (isSelected) {
            isAtLeastOneTrackSelected = true;
         }
         if (!isAtLeastOneTrackSelected) {
            if (t == pTrack && t->GetLinked()) {
               nextTrackIsLinkFromPTrack = true;
            }
            else if (nextTrackIsLinkFromPTrack) {
               rightTrack = t;
               nextTrackIsLinkFromPTrack = false;
            }
         }
      }
   }

   // We create a new snap manager in case any snap-points have changed
   if (mSnapManager)
      delete mSnapManager;

   mSnapManager = new SnapManager(mTracks, mViewInfo);

   mSnapLeft = -1;
   mSnapRight = -1;

#ifdef USE_MIDI
   mStretching = false;
   bool stretch = HitTestStretch(pTrack, rect, event);
#endif

   if (event.ShiftDown()

#ifdef USE_MIDI
       && !stretch
#endif
   ) {
      if (!isAtLeastOneTrackSelected) {
         pTrack->SetSelected(true);
         if (rightTrack)
            rightTrack->SetSelected(true);
         else if (pTrack->GetLink())
            pTrack->GetLink()->SetSelected(true);
      }

      double value;
      // Shift-click, choose closest boundary
      SelectionBoundary boundary =
         ChooseBoundary(event, pTrack, rect, false, false, &value);
      switch (boundary) {
         case SBLeft:
         case SBRight:
         {
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
            // If drag starts, change time selection only
            // (also exit frequency snapping)
            mFreqSelMode = FREQ_SEL_INVALID;
#endif
            mSelStartValid = true;
            mSelStart = value;
            ExtendSelection(event.m_x, rect.x, pTrack);
            break;
         }
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
         case SBBottom:
         case SBTop:
         {
            mFreqSelTrack = static_cast<const WaveTrack*>(pTrack);
            mFreqSelPin = value;
            mFreqSelMode =
               (boundary == SBBottom)
               ? FREQ_SEL_BOTTOM_FREE : FREQ_SEL_TOP_FREE;

            // Drag frequency only, not time:
            mSelStartValid = false;
            ExtendFreqSelection(event.m_y, rect.y, rect.height);
            break;
         }
         case SBCenter:
            HandleCenterFrequencyClick(true, pTrack, value);
            break;
#endif
         default:
            wxASSERT(false);
      };

      UpdateSelectionDisplay();
      // For persistence of the selection change:
      MakeParentModifyState(false);
      return;
   }

   else if(event.CmdDown()
#ifdef USE_MIDI
      && !stretch
#endif
   ) {

#ifdef EXPERIMENTAL_SCRUBBING_BASIC
      if (
#ifdef EXPERIMENTAL_SCRUBBING_SMOOTH_SCROLL
         event.LeftDClick() ||
#endif
         event.LeftDown()) {
         GetProject()->GetScrubber().MarkScrubStart(
            event.m_x
#ifdef EXPERIMENTAL_SCRUBBING_SMOOTH_SCROLL
            , event.LeftDClick()
#endif
         );
         return;
      }

#else

      StartOrJumpPlayback(event);

#endif

      // Not starting a drag
      SetCapturedTrack(NULL, IsUncaptured);
      return;
   }

   //Make sure you are within the selected track
   bool startNewSelection = true;
   if (pTrack && pTrack->GetSelected()) {
      // Adjusting selection edges can be turned off in the
      // preferences now
      if (mViewInfo->bAdjustSelectionEdges) {
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
         if (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
            isSpectralSelectionTrack(pTrack)) {
            // Ignore whether we are inside the time selection.
            // Exit center-snapping, start dragging the width.
            mFreqSelMode = FREQ_SEL_PINNED_CENTER;
            mFreqSelTrack = static_cast<WaveTrack*>(pTrack);
            mFreqSelPin = mViewInfo->selectedRegion.fc();
            // Do not adjust time boundaries
            mSelStartValid = false;
            ExtendFreqSelection(event.m_y, rect.y, rect.height);
            UpdateSelectionDisplay();
            // Frequency selection persists too, so do this:
            MakeParentModifyState(false);

            return;
         }
      else
#endif
         {
            // Not shift-down, choose boundary only within snapping
            double value;
            SelectionBoundary boundary =
               ChooseBoundary(event, pTrack, rect, true, true, &value);
            switch (boundary) {
            case SBNone:
               // startNewSelection remains true
               break;
            case SBLeft:
            case SBRight:
               startNewSelection = false;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
               // Disable frequency selection
               mFreqSelMode = FREQ_SEL_INVALID;
#endif
               mSelStartValid = true;
               mSelStart = value;
               break;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
            case SBBottom:
            case SBTop:
            case SBWidth:
               startNewSelection = false;
               // Disable time selection
               mSelStartValid = false;
               mFreqSelTrack = static_cast<const WaveTrack*>(pTrack);
               mFreqSelPin = value;
               mFreqSelMode =
                  (boundary == SBWidth) ? FREQ_SEL_PINNED_CENTER :
                  (boundary == SBBottom) ? FREQ_SEL_BOTTOM_FREE :
                  FREQ_SEL_TOP_FREE;
               break;
            case SBCenter:
               HandleCenterFrequencyClick(false, pTrack, value);
               startNewSelection = false;
               break;
#endif
            default:
               wxASSERT(false);
            }
         }
      } // mViewInfo->bAdjustSelectionEdges
   }

#ifdef USE_MIDI
   if (stretch) {
      NoteTrack *nt = (NoteTrack *) pTrack;
      // find nearest beat to sel0, sel1
      double minPeriod = 0.05; // minimum beat period
      double qBeat0, qBeat1;
      double centerBeat = 0.0f;
      mStretchSel0 = nt->NearestBeatTime(mViewInfo->selectedRegion.t0(), &qBeat0);
      mStretchSel1 = nt->NearestBeatTime(mViewInfo->selectedRegion.t1(), &qBeat1);

      // If there is not (almost) a beat to stretch that is slower
      // than 20 beats per second, don't stretch
      if (within(qBeat0, qBeat1, 0.9) ||
          (mStretchSel1 - mStretchSel0) / (qBeat1 - qBeat0) < minPeriod) return;

      if (startNewSelection) { // mouse is not at an edge, but after
         // quantization, we could be indicating the selection edge
         mSelStartValid = true;
         mSelStart = std::max(0.0, mViewInfo->PositionToTime(event.m_x, rect.x));
         mStretchStart = nt->NearestBeatTime(mSelStart, &centerBeat);
         if (within(qBeat0, centerBeat, 0.1)) {
            mListener->TP_DisplayStatusMessage(
                    _("Click and drag to stretch selected region."));
            SetCursor(*mStretchLeftCursor);
            // mStretchMode = stretchLeft;
            mSelStart = mViewInfo->selectedRegion.t1();
            // condition that implies stretchLeft
            startNewSelection = false;
         } else if (within(qBeat1, centerBeat, 0.1)) {
            mListener->TP_DisplayStatusMessage(
                    _("Click and drag to stretch selected region."));
            SetCursor(*mStretchRightCursor);
            // mStretchMode = stretchRight;
            mSelStart = mViewInfo->selectedRegion.t0();
            // condition that implies stretchRight
            startNewSelection = false;
         }
      }

      if (startNewSelection) {
         mStretchMode = stretchCenter;
         mStretchLeftBeats = qBeat1 - centerBeat;
         mStretchRightBeats = centerBeat - qBeat0;
      } else if (mSelStartValid && mViewInfo->selectedRegion.t1() == mSelStart) {
         // note that at this point, mSelStart is at the opposite
         // end of the selection from the cursor. If the cursor is
         // over sel0, then mSelStart is at sel1.
         mStretchMode = stretchLeft;
      } else {
         mStretchMode = stretchRight;
      }

      if (mStretchMode == stretchLeft) {
         mStretchLeftBeats = 0;
         mStretchRightBeats = qBeat1 - qBeat0;
      } else if (mStretchMode == stretchRight) {
         mStretchLeftBeats = qBeat1 - qBeat0;
         mStretchRightBeats = 0;
      }
      mViewInfo->selectedRegion.setTimes(mStretchSel0, mStretchSel1);
      mStretching = true;
      mStretched = false;

      /* i18n-hint: (noun) The track that is used for MIDI notes which can be
      dragged to change their duration.*/
      MakeParentPushState(_("Stretch Note Track"),
      /* i18n-hint: In the history list, indicates a MIDI note has
      been dragged to change its duration (stretch it). Using either past
      or present tense is fine here.  If unsure, go for whichever is
      shorter.*/
      _("Stretch"));

      // Full refresh since the label area may need to indicate
      // newly selected tracks. (I'm really not sure if the label area
      // needs to be refreshed or how to just refresh non-label areas.-RBD)
      Refresh(false);

      // Make sure the ruler follows suit.
      mRuler->DrawSelection();

      // As well as the SelectionBar.
      DisplaySelection();

      return;
   }
#endif
   if (startNewSelection) {
      // If we didn't move a selection boundary, start a new selection
      SelectNone();
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      StartFreqSelection (event.m_y, rect.y, rect.height, pTrack);
#endif
      StartSelection(event.m_x, rect.x);
      mTracks->Select(pTrack);
      SetFocusedTrack(pTrack);
      //On-Demand: check to see if there is an OD thing associated with this track.
      if (pTrack->GetKind() == Track::Wave) {
         if(ODManager::IsInstanceCreated())
            ODManager::Instance()->DemandTrackUpdate((WaveTrack*)pTrack,mSelStart);
      }
      DisplaySelection();
   }
}


/// Reset our selection markers.
void TrackPanel::StartSelection(int mouseXCoordinate, int trackLeftEdge)
{
   mSelStartValid = true;
   mSelStart = std::max(0.0, mViewInfo->PositionToTime(mouseXCoordinate, trackLeftEdge));

   double s = mSelStart;

   if (mSnapManager) {
      mSnapLeft = -1;
      mSnapRight = -1;
      bool snappedPoint, snappedTime;
      if (mSnapManager->Snap(mCapturedTrack, mSelStart, false,
                             &s, &snappedPoint, &snappedTime)) {
         if (snappedPoint)
            mSnapLeft = mViewInfo->TimeToPosition(s, trackLeftEdge);
      }
   }

   mViewInfo->selectedRegion.setTimes(s, s);

   SonifyBeginModifyState();
   MakeParentModifyState(false);
   SonifyEndModifyState();
}

/// Extend the existing selection
void TrackPanel::ExtendSelection(int mouseXCoordinate, int trackLeftEdge,
                                 Track *pTrack)
{
   if (!mSelStartValid)
      // Must be dragging frequency bounds only.
      return;

   double selend = std::max(0.0, mViewInfo->PositionToTime(mouseXCoordinate, trackLeftEdge));
   clip_bottom(selend, 0.0);

   double origSel0, origSel1;
   double sel0, sel1;

   if (pTrack == NULL && mCapturedTrack != NULL)
      pTrack = mCapturedTrack;

   if (mSelStart < selend) {
      sel0 = mSelStart;
      sel1 = selend;
   }
   else {
      sel1 = mSelStart;
      sel0 = selend;
   }

   origSel0 = sel0;
   origSel1 = sel1;

   if (mSnapManager) {
      mSnapLeft = -1;
      mSnapRight = -1;
      bool snappedPoint, snappedTime;
      if (mSnapManager->Snap(mCapturedTrack, sel0, false,
                             &sel0, &snappedPoint, &snappedTime)) {
         if (snappedPoint)
            mSnapLeft = mViewInfo->TimeToPosition(sel0, trackLeftEdge);
      }
      if (mSnapManager->Snap(mCapturedTrack, sel1, true,
                             &sel1, &snappedPoint, &snappedTime)) {
         if (snappedPoint)
            mSnapRight = mViewInfo->TimeToPosition(sel1, trackLeftEdge);
      }

      // Check if selection endpoints are too close together to snap (unless
      // using snap-to-time -- then we always accept the snap results)
      if (mSnapLeft >= 0 && mSnapRight >= 0 && mSnapRight - mSnapLeft < 3 &&
            !snappedTime) {
         sel0 = origSel0;
         sel1 = origSel1;
         mSnapLeft = -1;
         mSnapRight = -1;
      }
   }

   mViewInfo->selectedRegion.setTimes(sel0, sel1);

   //On-Demand: check to see if there is an OD thing associated with this track.  If so we want to update the focal point for the task.
   if (pTrack && (pTrack->GetKind() == Track::Wave) && ODManager::IsInstanceCreated())
      ODManager::Instance()->DemandTrackUpdate((WaveTrack*)pTrack,sel0); //sel0 is sometimes less than mSelStart
}

void TrackPanel::UpdateSelectionDisplay()
{
   // Full refresh since the label area may need to indicate
   // newly selected tracks.
   Refresh(false);

   // Make sure the ruler follows suit.
   mRuler->DrawSelection();

   // As well as the SelectionBar.
   DisplaySelection();
}

void TrackPanel::UpdateAccessibility()
{
   if (mAx)
      mAx->Updated();
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
namespace {
   
inline double findMaxRatio(double center, double rate)
{
   const double minFrequency = 1.0;
   const double maxFrequency = (rate / 2.0);
   const double frequency =
      std::min(maxFrequency,
      std::max(minFrequency, center));
   return
      std::min(frequency / minFrequency, maxFrequency / frequency);
}

}

void TrackPanel::SnapCenterOnce(const WaveTrack *pTrack, bool up)
{
   const SpectrogramSettings &settings = pTrack->GetSpectrogramSettings();
   const int windowSize = settings.GetFFTLength();
   const double rate = pTrack->GetRate();
   const double nyq = rate / 2.0;
   const double binFrequency = rate / windowSize;

   double f1 = mViewInfo->selectedRegion.f1();
   double centerFrequency = mViewInfo->selectedRegion.fc();
   if (centerFrequency <= 0) {
      centerFrequency = up ? binFrequency : nyq;
      f1 = centerFrequency * sqrt(2.0);
   }

   const double ratio = f1 / centerFrequency;
   const int originalBin = floor(0.5 + centerFrequency / binFrequency);
   const int limitingBin = up ? floor(0.5 + nyq / binFrequency) : 1;

   // This is crude and wasteful, doing the FFT each time the command is called.
   // It would be better to cache the data, but then invalidation of the cache would
   // need doing in all places that change the time selection.
   StartSnappingFreqSelection(pTrack);
   double snappedFrequency = centerFrequency;
   int bin = originalBin;
   if (up) {
      while (snappedFrequency <= centerFrequency &&
             bin < limitingBin)
         snappedFrequency = mFrequencySnapper->FindPeak(++bin * binFrequency, NULL);
   }
   else {
      while (snappedFrequency >= centerFrequency &&
         bin > limitingBin)
         snappedFrequency = mFrequencySnapper->FindPeak(--bin * binFrequency, NULL);
   }

   mViewInfo->selectedRegion.setFrequencies
      (snappedFrequency / ratio, snappedFrequency * ratio);
}

void TrackPanel::StartSnappingFreqSelection (const WaveTrack *pTrack)
{
   static const sampleCount minLength = 8;

   const double rate = pTrack->GetRate();

   // Grab samples, just for this track, at these times
   std::vector<float> frequencySnappingData;
   const sampleCount start =
      pTrack->TimeToLongSamples(mViewInfo->selectedRegion.t0());
   const sampleCount end =
      pTrack->TimeToLongSamples(mViewInfo->selectedRegion.t1());
   const sampleCount length =
      std::min(sampleCount(frequencySnappingData.max_size()),
         std::min(sampleCount(10485760), // as in FreqWindow.cpp
                  end - start));
   const sampleCount effectiveLength = std::max(minLength, length);
   frequencySnappingData.resize(effectiveLength, 0.0f);
   pTrack->Get(
      reinterpret_cast<samplePtr>(&frequencySnappingData[0]),
      floatSample, start, length);

   // Use same settings as are now used for spectrogram display,
   // except, shrink the window as needed so we get some answers

   const SpectrogramSettings &settings = pTrack->GetSpectrogramSettings();
   int windowSize = settings.GetFFTLength();

   while(windowSize > effectiveLength)
      windowSize >>= 1;
   const int windowType = settings.windowType;

   mFrequencySnapper->Calculate(
      SpectrumAnalyst::Spectrum, windowType, windowSize, rate,
      &frequencySnappingData[0], length);

   // We can now throw away the sample data but we keep the spectrum.
}

void TrackPanel::MoveSnappingFreqSelection (int mouseYCoordinate,
                                            int trackTopEdge,
                                            int trackHeight, Track *pTrack)
{
   if (pTrack &&
       pTrack->GetSelected() &&
       isSpectralSelectionTrack(pTrack)) {
      WaveTrack *const wt = static_cast<WaveTrack*>(pTrack);
      // PRL:
      // What happens if center snapping selection began in one spectrogram track,
      // then continues inside another?  We do not then recalculate
      // the spectrum (as was done in StartSnappingFreqSelection)
      // but snap according to the peaks in the old track.
      // I am not worrying about that odd case.
      const double rate = wt->GetRate();
      const double frequency =
         PositionToFrequency(wt, false, mouseYCoordinate,
         trackTopEdge, trackHeight);
      const double snappedFrequency =
         mFrequencySnapper->FindPeak(frequency, NULL);
      const double maxRatio = findMaxRatio(snappedFrequency, rate);
      double ratio = 2.0; // An arbitrary octave on each side, at most
      {
         const double f0 = mViewInfo->selectedRegion.f0();
         const double f1 = mViewInfo->selectedRegion.f1();
         if (f1 >= f0 && f0 >= 0)
            // Preserve already chosen ratio instead
            ratio = sqrt(f1 / f0);
      }
      ratio = std::min(ratio, maxRatio);

      mFreqSelPin = snappedFrequency;
      mViewInfo->selectedRegion.setFrequencies(
         snappedFrequency / ratio, snappedFrequency * ratio);

      mFreqSelTrack = wt;
      // SelectNone();
      // mTracks->Select(pTrack);
      SetFocusedTrack(pTrack);
   }
}

void TrackPanel::StartFreqSelection (int mouseYCoordinate, int trackTopEdge,
                                     int trackHeight, Track *pTrack)
{
   mFreqSelTrack = 0;
   mFreqSelMode = FREQ_SEL_INVALID;
   mFreqSelPin = SelectedRegion::UndefinedFrequency;

   if (isSpectralSelectionTrack(pTrack)) {
      mFreqSelTrack = static_cast<WaveTrack*>(pTrack);
      mFreqSelMode = FREQ_SEL_FREE;
      mFreqSelPin =
         PositionToFrequency(mFreqSelTrack, false, mouseYCoordinate,
                             trackTopEdge, trackHeight);
      mViewInfo->selectedRegion.setFrequencies(mFreqSelPin, mFreqSelPin);
   }
}

void TrackPanel::ExtendFreqSelection(int mouseYCoordinate, int trackTopEdge,
                                     int trackHeight)
{
   // When dragWidth is true, and not dragging the center,
   // adjust both top and bottom about geometric mean.

   if (mFreqSelMode == FREQ_SEL_INVALID ||
       mFreqSelMode == FREQ_SEL_SNAPPING_CENTER)
      return;

   // Extension happens only when dragging in the same track in which we
   // started, and that is of a spectrogram display type.

   const WaveTrack* wt = mFreqSelTrack;
   const double rate =  wt->GetRate();
   const double frequency =
      PositionToFrequency(wt, true, mouseYCoordinate,
         trackTopEdge, trackHeight);

   // Dragging center?
   if (mFreqSelMode == FREQ_SEL_DRAG_CENTER) {
      if (frequency == rate || frequency < 1.0)
         // snapped to top or bottom
         mViewInfo->selectedRegion.setFrequencies(
            SelectedRegion::UndefinedFrequency,
            SelectedRegion::UndefinedFrequency);
      else {
         // mFreqSelPin holds the ratio of top to center
         const double maxRatio = findMaxRatio(frequency, rate);
         const double ratio = std::min(maxRatio, mFreqSelPin);
         mViewInfo->selectedRegion.setFrequencies(
            frequency / ratio, frequency * ratio);
      }
   }
   else if (mFreqSelMode == FREQ_SEL_PINNED_CENTER) {
      if (mFreqSelPin >= 0) {
         // Change both upper and lower edges leaving centre where it is.
         if (frequency == rate || frequency < 1.0)
            // snapped to top or bottom
            mViewInfo->selectedRegion.setFrequencies(
               SelectedRegion::UndefinedFrequency,
               SelectedRegion::UndefinedFrequency);
         else {
            // Given center and mouse position, find ratio of the larger to the
            // smaller, limit that to the frequency scale bounds, and adjust
            // top and bottom accordingly.
            const double maxRatio = findMaxRatio(mFreqSelPin, rate);
            double ratio = frequency / mFreqSelPin;
            if (ratio < 1.0)
               ratio = 1.0 / ratio;
            ratio = std::min(maxRatio, ratio);
            mViewInfo->selectedRegion.setFrequencies(
               mFreqSelPin / ratio, mFreqSelPin * ratio);
         }
      }
   }
   else {
      // Dragging of upper or lower.
      const bool bottomDefined =
         !(mFreqSelMode == FREQ_SEL_TOP_FREE && mFreqSelPin < 0);
      const bool topDefined =
         !(mFreqSelMode == FREQ_SEL_BOTTOM_FREE && mFreqSelPin < 0);
      if (!bottomDefined || (topDefined && mFreqSelPin < frequency)) {
         // Adjust top
         if (frequency == rate)
            // snapped high; upper frequency is undefined
            mViewInfo->selectedRegion.setF1(SelectedRegion::UndefinedFrequency);
         else
            mViewInfo->selectedRegion.setF1(std::max(1.0, frequency));

         mViewInfo->selectedRegion.setF0(mFreqSelPin);
      }
      else {
         // Adjust bottom
         if (frequency < 1.0)
            // snapped low; lower frequency is undefined
            mViewInfo->selectedRegion.setF0(SelectedRegion::UndefinedFrequency);
         else
            mViewInfo->selectedRegion.setF0(std::min(rate / 2.0, frequency));

         mViewInfo->selectedRegion.setF1(mFreqSelPin);
      }
   }
}

void TrackPanel::ToggleSpectralSelection()
{
   SelectedRegion &region = mViewInfo->selectedRegion;
   const double f0 = region.f0();
   const double f1 = region.f1();
   const bool haveSpectralSelection =
      !(f0 == SelectedRegion::UndefinedFrequency &&
        f1 == SelectedRegion::UndefinedFrequency);
   if (haveSpectralSelection)
   {
      mLastF0 = f0;
      mLastF1 = f1;
      region.setFrequencies
         (SelectedRegion::UndefinedFrequency, SelectedRegion::UndefinedFrequency);
   }
   else
      region.setFrequencies(mLastF0, mLastF1);
}

void TrackPanel::ResetFreqSelectionPin(double hintFrequency, bool logF)
{
   switch (mFreqSelMode) {
   case FREQ_SEL_INVALID:
   case FREQ_SEL_SNAPPING_CENTER:
      mFreqSelPin = -1.0;
      break;

   case FREQ_SEL_PINNED_CENTER:
      mFreqSelPin = mViewInfo->selectedRegion.fc();
      break;

   case FREQ_SEL_DRAG_CENTER:
      {
         // Re-pin the width
         const double f0 = mViewInfo->selectedRegion.f0();
         const double f1 = mViewInfo->selectedRegion.f1();
         if (f0 >= 0 && f1 >= 0)
            mFreqSelPin = sqrt(f1 / f0);
         else
            mFreqSelPin = -1.0;
      }
      break;

   case FREQ_SEL_FREE:
      // Pin which?  Farther from the hint which is the presumed
      // mouse position.
      {
         const double f0 = mViewInfo->selectedRegion.f0();
         const double f1 = mViewInfo->selectedRegion.f1();
         if (logF) {
            if (f1 < 0)
               mFreqSelPin = f0;
            else {
               const double logf1 = log(std::max(1.0, f1));
               const double logf0 = log(std::max(1.0, f0));
               const double logHint = log(std::max(1.0, hintFrequency));
               if (std::abs (logHint - logf1) < std::abs (logHint - logf0))
                  mFreqSelPin = f0;
               else
                  mFreqSelPin = f1;
            }
         }
         else {
            if (f1 < 0 ||
                std::abs (hintFrequency - f1) < std::abs (hintFrequency - f0))
               mFreqSelPin = f0;
            else
               mFreqSelPin = f1;
            }
      }
      break;

   case FREQ_SEL_TOP_FREE:
      mFreqSelPin = mViewInfo->selectedRegion.f0();
      break;

   case FREQ_SEL_BOTTOM_FREE:
      mFreqSelPin = mViewInfo->selectedRegion.f1();
      break;

   default:
      wxASSERT(false);
   }
}
#endif

#ifdef USE_MIDI
void TrackPanel::Stretch(int mouseXCoordinate, int trackLeftEdge,
                         Track *pTrack)
{
   if (mStretched) { // Undo stretch and redo it with new mouse coordinates
      // Drag handling was not originally implemented with Undo in mind --
      // there are saved pointers to tracks that are not supposed to change.
      // Undo will change tracks, so convert pTrack, mCapturedTrack to index
      // values, then look them up after the Undo
      TrackListIterator iter(mTracks);
      int pTrackIndex = pTrack->GetIndex();
      int capturedTrackIndex =
         (mCapturedTrack ? mCapturedTrack->GetIndex() : 0);

      GetProject()->OnUndo();

      // Undo brings us back to the pre-click state, but we want to
      // quantize selected region to integer beat boundaries. These
      // were saved in mStretchSel[12] variables:
      mViewInfo->selectedRegion.setTimes(mStretchSel0, mStretchSel1);

      mStretched = false;
      int index = 0;
      for (Track *t = iter.First(mTracks); t; t = iter.Next()) {
         if (index == pTrackIndex) pTrack = t;
         if (mCapturedTrack && index == capturedTrackIndex) mCapturedTrack = t;
         index++;
      }
   }

   if (pTrack == NULL && mCapturedTrack != NULL)
      pTrack = mCapturedTrack;

   if (!pTrack || pTrack->GetKind() != Track::Note) {
      return;
   }

   NoteTrack *pNt = (NoteTrack *) pTrack;
   double moveto = std::max(0.0, mViewInfo->PositionToTime(mouseXCoordinate, trackLeftEdge));

   // check to make sure tempo is not higher than 20 beats per second
   // (In principle, tempo can be higher, but not infinity.)
   double minPeriod = 0.05; // minimum beat period
   double qBeat0, qBeat1;
   pNt->NearestBeatTime(mViewInfo->selectedRegion.t0(), &qBeat0); // get beat
   pNt->NearestBeatTime(mViewInfo->selectedRegion.t1(), &qBeat1);

   // We could be moving 3 things: left edge, right edge, a point between
   switch (mStretchMode) {
   case stretchLeft: {
      // make sure target duration is not too short
      double dur = mViewInfo->selectedRegion.t1() - moveto;
      if (dur < mStretchRightBeats * minPeriod) {
         dur = mStretchRightBeats * minPeriod;
         moveto = mViewInfo->selectedRegion.t1() - dur;
      }
      if (pNt->StretchRegion(mStretchSel0, mStretchSel1, dur)) {
         pNt->SetOffset(pNt->GetOffset() + moveto - mStretchSel0);
         mViewInfo->selectedRegion.setT0(moveto);
      }
      break;
   }
   case stretchRight: {
      // make sure target duration is not too short
      double dur = moveto - mViewInfo->selectedRegion.t0();
      if (dur < mStretchLeftBeats * minPeriod) {
         dur = mStretchLeftBeats * minPeriod;
         moveto = mStretchSel0 + dur;
      }
      if (pNt->StretchRegion(mStretchSel0, mStretchSel1, dur)) {
         mViewInfo->selectedRegion.setT1(moveto);
      }
      break;
   }
   case stretchCenter: {
      // make sure both left and right target durations are not too short
      double left_dur = moveto - mViewInfo->selectedRegion.t0();
      double right_dur = mViewInfo->selectedRegion.t1() - moveto;
      double centerBeat;
      pNt->NearestBeatTime(mSelStart, &centerBeat);
      if (left_dur < mStretchLeftBeats * minPeriod) {
         left_dur = mStretchLeftBeats * minPeriod;
         moveto = mStretchSel0 + left_dur;
      }
      if (right_dur < mStretchRightBeats * minPeriod) {
         right_dur = mStretchRightBeats * minPeriod;
         moveto = mStretchSel1 - right_dur;
      }
      pNt->StretchRegion(mStretchStart, mStretchSel1, right_dur);
      pNt->StretchRegion(mStretchSel0, mStretchStart, left_dur);
      break;
   }
   default:
      wxASSERT(false);
      break;
   }
   MakeParentPushState(_("Stretch Note Track"), _("Stretch"),
      PUSH_CONSOLIDATE | PUSH_AUTOSAVE);
   mStretched = true;
   Refresh(false);
}
#endif

/// AS: If we're dragging to extend a selection (or actually,
///  if the screen is scrolling while you're selecting), we
///  handle it here.
void TrackPanel::SelectionHandleDrag(wxMouseEvent & event, Track *clickedTrack)
{
#ifdef EXPERIMENTAL_SCRUBBING_BASIC
   Scrubber &scrubber = GetProject()->GetScrubber();
   if (scrubber.IsScrubbing() ||
       GetProject()->GetScrubber().MaybeStartScrubbing(event))
      // Do nothing more, don't change selection
      return;
#endif

   // AS: If we're not in the process of selecting (set in
   //  the SelectionHandleClick above), fuhggeddaboudit.
   if (mMouseCapture!=IsSelecting)
      return;

   // Also fuhggeddaboudit if we're not dragging and not autoscrolling.
   if (!event.Dragging() && !mAutoScrolling)
      return;

   if (event.CmdDown()) {
      // Ctrl-drag has no meaning, fuhggeddaboudit
      return;
   }

   wxRect rect      = mCapturedRect;
   Track *pTrack = mCapturedTrack;

   if (!pTrack) {
      pTrack = FindTrack(event.m_x, event.m_y, false, false, &rect);
      rect.y += kTopMargin;
      rect.height -= kTopMargin + kBottomMargin;
   }

   // Also fuhggeddaboudit if not in a track.
   if (!pTrack)
      return;

   int x = mAutoScrolling ? mMouseMostRecentX : event.m_x;
   int y = mAutoScrolling ? mMouseMostRecentY : event.m_y;

   // JKC: Logic to prevent a selection smaller than 5 pixels to
   // prevent accidental dragging when selecting.
   // (if user really wants a tiny selection, they should zoom in).
   // Can someone make this value of '5' configurable in
   // preferences?
   const int minimumSizedSelection = 5; //measured in pixels

   // Might be dragging frequency bounds only, test
   if (mSelStartValid) {
      wxInt64 SelStart = mViewInfo->TimeToPosition(mSelStart, rect.x); //cvt time to pixels.
      // Abandon this drag if selecting < 5 pixels.
      if (wxLongLong(SelStart-x).Abs() < minimumSizedSelection
#ifdef USE_MIDI        // limiting selection size is good, and not starting
          && !mStretching // stretch unless mouse moves 5 pixels is good, but
#endif                 // once stretching starts, it's ok to move even 1 pixel
          )
          return;
   }

   // Handle which tracks are selected
   Track *sTrack = pTrack;
   if (Track *eTrack = FindTrack(x, y, false, false, NULL)) {
      // Swap the track pointers if needed
      if (eTrack->GetIndex() < sTrack->GetIndex()) {
         Track *t = eTrack;
         eTrack = sTrack;
         sTrack = t;
      }

      TrackListIterator iter(mTracks);
      sTrack = iter.StartWith(sTrack);
      do {
         mTracks->Select(sTrack);
         if (sTrack == eTrack) {
            break;
         }

         sTrack = iter.Next();
      } while (sTrack);
   }
#ifdef USE_MIDI
   if (mStretching) {
      // the following is also in ExtendSelection, called below
      // probably a good idea to "hoist" the code to before this "if" stmt
      if (clickedTrack == NULL && mCapturedTrack != NULL)
         clickedTrack = mCapturedTrack;
      Stretch(x, rect.x, clickedTrack);
      return;
   }
#endif

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
#ifndef SPECTRAL_EDITING_ESC_KEY
   if (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
      !mViewInfo->selectedRegion.isPoint())
      MoveSnappingFreqSelection(y, rect.y, rect.height, pTrack);
   else
#endif
   if (mFreqSelTrack == pTrack)
      ExtendFreqSelection(y, rect.y, rect.height);
#endif

   ExtendSelection(x, rect.x, clickedTrack);
   UpdateSelectionDisplay();
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
// Seems 4 is too small to work at the top.  Why?
enum { FREQ_SNAP_DISTANCE = 10 };

/// Converts a position (mouse Y coordinate) to
/// frequency, in Hz.
double TrackPanel::PositionToFrequency(const WaveTrack *wt,
                                       bool maySnap,
                                       wxInt64 mouseYCoordinate,
                                       wxInt64 trackTopEdge,
                                       int trackHeight) const
{
   const double rate = wt->GetRate();

   // Handle snapping
   if (maySnap &&
       mouseYCoordinate - trackTopEdge < FREQ_SNAP_DISTANCE)
      return rate;
   if (maySnap &&
       trackTopEdge + trackHeight - mouseYCoordinate < FREQ_SNAP_DISTANCE)
      return -1;

   const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
   float minFreq, maxFreq;
   wt->GetSpectrumBounds(&minFreq, &maxFreq);
   const NumberScale numberScale(settings.GetScale(minFreq, maxFreq, rate, false));
   const double p = double(mouseYCoordinate - trackTopEdge) / trackHeight;
   return numberScale.PositionToValue(1.0 - p);
}

/// Converts a frequency to screen y position.
wxInt64 TrackPanel::FrequencyToPosition(const WaveTrack *wt,
                                        double frequency,
                                        wxInt64 trackTopEdge,
                                        int trackHeight) const
{
   const double rate = wt->GetRate();

   const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
   float minFreq, maxFreq;
   wt->GetSpectrumBounds(&minFreq, &maxFreq);
   const NumberScale numberScale(settings.GetScale(minFreq, maxFreq, rate, false));
   const float p = numberScale.ValueToPosition(frequency);
   return trackTopEdge + wxInt64((1.0 - p) * trackHeight);
}
#endif

template<typename T>
inline void SetIfNotNull( T * pValue, const T Value )
{
   if( pValue == NULL )
      return;
   *pValue = Value;
}


TrackPanel::SelectionBoundary TrackPanel::ChooseTimeBoundary
(double selend, bool onlyWithinSnapDistance,
 wxInt64 *pPixelDist, double *pPinValue) const
{
   const double t0 = mViewInfo->selectedRegion.t0();
   const double t1 = mViewInfo->selectedRegion.t1();
   const wxInt64 posS = mViewInfo->TimeToPosition(selend);
   const wxInt64 pos0 = mViewInfo->TimeToPosition(t0);
   wxInt64 pixelDist = std::abs(posS - pos0);
   bool chooseLeft = true;

   if (mViewInfo->selectedRegion.isPoint())
      // Special case when selection is a point, and thus left
      // and right distances are the same
      chooseLeft = (selend < t0);
   else {
      const wxInt64 pos1 = mViewInfo->TimeToPosition(t1);
      const wxInt64 rightDist = std::abs(posS - pos1);
      if (rightDist < pixelDist)
         chooseLeft = false, pixelDist = rightDist;
   }

   SetIfNotNull(pPixelDist, pixelDist);

   if (onlyWithinSnapDistance &&
       pixelDist >= SELECTION_RESIZE_REGION) {
      SetIfNotNull( pPinValue, -1.0);
      return SBNone;
   }
   else if (chooseLeft) {
      SetIfNotNull( pPinValue, t1);
      return SBLeft;
   }
   else {
      SetIfNotNull( pPinValue, t0);
      return SBRight;
   }
}


TrackPanel::SelectionBoundary TrackPanel::ChooseBoundary
(wxMouseEvent & event, const Track *pTrack, const wxRect &rect,
bool mayDragWidth, bool onlyWithinSnapDistance,
 double *pPinValue) const
{
   // Choose one of four boundaries to adjust, or the center frequency.
   // May choose frequencies only if in a spectrogram view and
   // within the time boundaries.
   // May choose no boundary if onlyWithinSnapDistance is true.
   // Otherwise choose the eligible boundary nearest the mouse click.
   const double selend = mViewInfo->PositionToTime(event.m_x, rect.x);
   wxInt64 pixelDist = 0;
   SelectionBoundary boundary =
      ChooseTimeBoundary(selend, onlyWithinSnapDistance,
      &pixelDist, pPinValue);

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   const double t0 = mViewInfo->selectedRegion.t0();
   const double t1 = mViewInfo->selectedRegion.t1();
   const double f0 = mViewInfo->selectedRegion.f0();
   const double f1 = mViewInfo->selectedRegion.f1();
   const double fc = mViewInfo->selectedRegion.fc();
   double ratio = 0;

   bool chooseTime = true;
   bool chooseBottom = true;
   bool chooseCenter = false;
   // Consider adjustment of frequencies only if mouse is
   // within the time boundaries
   if (!mViewInfo->selectedRegion.isPoint() &&
       t0 <= selend && selend < t1 &&
       isSpectralSelectionTrack(pTrack)) {
      const WaveTrack *const wt = static_cast<const WaveTrack*>(pTrack);
      const wxInt64 bottomSel = (f0 >= 0)
         ? FrequencyToPosition(wt, f0, rect.y, rect.height)
         : rect.y + rect.height;
      const wxInt64 topSel = (f1 >= 0)
         ? FrequencyToPosition(wt, f1, rect.y, rect.height)
         : rect.y;
      wxInt64 signedBottomDist = int(event.m_y - bottomSel);
      wxInt64 verticalDist = std::abs(signedBottomDist);
      if (bottomSel == topSel)
         // Top and bottom are too close to resolve on screen
         chooseBottom = (signedBottomDist >= 0);
      else {
         const wxInt64 topDist = abs(int(event.m_y - topSel));
         if (topDist < verticalDist)
            chooseBottom = false, verticalDist = topDist;
      }
      if (fc > 0
#ifdef SPECTRAL_EDITING_ESC_KEY
         && mayDragWidth
#endif
         ) {
         const wxInt64 centerSel =
            FrequencyToPosition(wt, fc, rect.y, rect.height);
         const wxInt64 centerDist = abs(int(event.m_y - centerSel));
         if (centerDist < verticalDist)
            chooseCenter = true, verticalDist = centerDist,
            ratio = f1 / fc;
      }
      if (verticalDist >= 0 &&
          verticalDist < pixelDist) {
         pixelDist = verticalDist;
         chooseTime = false;
      }
   }

   if (!chooseTime) {
      // PRL:  Seems I need a larger tolerance to make snapping work
      // at top of track, not sure why
      if (onlyWithinSnapDistance &&
          pixelDist >= FREQ_SNAP_DISTANCE) {
         SetIfNotNull( pPinValue, -1.0);
         return SBNone;
      }
      else if (chooseCenter) {
         SetIfNotNull( pPinValue, ratio);
         return SBCenter;
      }
      else if (mayDragWidth && fc > 0) {
         SetIfNotNull(pPinValue, fc);
         return SBWidth;
      }
      else if (chooseBottom) {
         SetIfNotNull( pPinValue, f1 );
         return SBBottom;
      }
      else {
         SetIfNotNull(pPinValue, f0);
         return SBTop;
      }
   }
   else
#endif
   {
      return boundary;
   }
}

/// Determines if drag zooming is active
bool TrackPanel::IsDragZooming(int zoomStart, int zoomEnd)
{
   return (abs(zoomEnd - zoomStart) > DragThreshold);
}

/// Determines if the a modal tool is active
bool TrackPanel::IsMouseCaptured()
{
   return (mMouseCapture != IsUncaptured || mCapturedTrack != NULL
      || mUIHandle != NULL);
}


/// Vertical zooming (triggered by clicking in the
/// vertical ruler)
void TrackPanel::HandleVZoom(wxMouseEvent & event)
{
   if (event.ButtonDown() || event.ButtonDClick()) {
      HandleVZoomClick( event );
   }
   else if (event.Dragging()) {
      HandleVZoomDrag( event );
   }
   else if (event.ButtonUp()) {
      HandleVZoomButtonUp( event );
   }
   //TODO-MB: add timetrack zooming here!
}

/// VZoom click
void TrackPanel::HandleVZoomClick( wxMouseEvent & event )
{
   if (mCapturedTrack)
      return;
   mCapturedTrack = FindTrack(event.m_x, event.m_y, true, false,
                              &mCapturedRect);
   if (!mCapturedTrack)
      return;

   if (mCapturedTrack->GetKind() == Track::Wave
#ifdef USE_MIDI
            || mCapturedTrack->GetKind() == Track::Note
#endif
         )
   {
      mMouseCapture = IsVZooming;
      mZoomStart = event.m_y;
      mZoomEnd = event.m_y;
      // change note track to zoom like audio track
      //#ifdef USE_MIDI
      //      if (mCapturedTrack->GetKind() == Track::Note) {
      //          ((NoteTrack *) mCapturedTrack)->StartVScroll();
      //      }
      //#endif
   }
}

/// VZoom drag
void TrackPanel::HandleVZoomDrag( wxMouseEvent & event )
{
   mZoomEnd = event.m_y;
   if (IsDragZooming()){
      // changed Note track to work like audio track
      //#ifdef USE_MIDI
      //      if (mCapturedTrack && mCapturedTrack->GetKind() == Track::Note) {
      //         ((NoteTrack *) mCapturedTrack)->VScroll(mZoomStart, mZoomEnd);
      //      }
      //#endif
      Refresh(false);
   }
}

/// VZoom Button up.
/// There are three cases:
///   - Drag-zooming; we already have a min and max
///   - Zoom out; ensure we don't go too small.
///   - Zoom in; ensure we don't go too large.
void TrackPanel::HandleVZoomButtonUp( wxMouseEvent & event )
{
   if (!mCapturedTrack)
      return;

   mMouseCapture = IsUncaptured;

#ifdef USE_MIDI
   // handle vertical scrolling in Note Track. This is so different from
   // zooming in audio tracks that it is handled as a special case from
   // which we then return
   if (mCapturedTrack->GetKind() == Track::Note) {
      NoteTrack *nt = (NoteTrack *) mCapturedTrack;
      if (IsDragZooming()) {
         nt->ZoomTo(mZoomStart, mZoomEnd);
      } else if (event.ShiftDown() || event.RightUp()) {
         nt->ZoomOut(mZoomEnd);
      } else {
         nt->ZoomIn(mZoomEnd);
      }
      mZoomEnd = mZoomStart = 0;
      Refresh(false);
      mCapturedTrack = NULL;
      MakeParentModifyState(true);
      return;
   }
#endif


   // don't do anything if track is not wave
   if (mCapturedTrack->GetKind() != Track::Wave)
      return;

   /*
   if (event.RightUp() &&
       !(event.ShiftDown() || event.CmdDown())) {
      OnVRulerMenu(mCapturedTrack, &event);
      return;
   }
   */

   HandleWaveTrackVZoom(static_cast<WaveTrack*>(mCapturedTrack),
      event.ShiftDown(), event.RightUp());
   mCapturedTrack = NULL;
}

void TrackPanel::HandleWaveTrackVZoom(WaveTrack *track, bool shiftDown, bool rightUp)
{
   HandleWaveTrackVZoom(mTracks, mCapturedRect, mZoomStart, mZoomEnd,
      track, shiftDown, rightUp, false);
   mZoomEnd = mZoomStart = 0;
   UpdateVRuler(track);
   Refresh(false);
   MakeParentModifyState(true);
}

//static
void TrackPanel::HandleWaveTrackVZoom
(TrackList *tracks, const wxRect &rect,
 int zoomStart, int zoomEnd,
 WaveTrack *track, bool shiftDown, bool rightUp,
 bool fixedMousePoint)
{
   WaveTrack *const partner = static_cast<WaveTrack *>(tracks->GetLink(track));
   int height = track->GetHeight() - (kTopMargin + kBottomMargin);
   int ypos = rect.y + kBorderThickness;

   // Ensure start and end are in order (swap if not).
   if (zoomEnd < zoomStart)
      std::swap(zoomStart, zoomEnd);

   float min, max, minBand = 0;
   const double rate = track->GetRate();
   const float halfrate = rate / 2;
   const SpectrogramSettings &settings = track->GetSpectrogramSettings();
   NumberScale scale;
   const bool spectral = (track->GetDisplay() == WaveTrack::Spectrum);
   const bool spectrumLinear = spectral &&
      (track->GetSpectrogramSettings().scaleType == SpectrogramSettings::stLinear);

   if (spectral) {
      track->GetSpectrumBounds(&min, &max);
      scale = (settings.GetScale(min, max, rate, false));
      const int fftLength = settings.GetFFTLength();
      const float binSize = rate / fftLength;
      const int minBins =
         std::min(10, fftLength / 2); //minimum 10 freq bins, unless there are less
      minBand = minBins * binSize;
   }
   else
      track->GetDisplayBounds(&min, &max);

   if (IsDragZooming(zoomStart, zoomEnd)) {
      // Drag Zoom
      const float tmin = min, tmax = max;

      if (spectral) {
         double xmin = 1 - (zoomEnd - ypos) / (float)height;
         double xmax = 1 - (zoomStart - ypos) / (float)height;
         const float middle = (xmin + xmax) / 2;
         const float middleValue = scale.PositionToValue(middle);

         min = std::max(spectrumLinear ? 0.0f : 1.0f,
            std::min(middleValue - minBand / 2,
               scale.PositionToValue(xmin)
         ));
         max = std::min(halfrate,
            std::max(middleValue + minBand / 2,
               scale.PositionToValue(xmax)
         ));
      }
      else {
         const float p1 = (zoomStart - ypos) / (float)height;
         const float p2 = (zoomEnd - ypos) / (float)height;
         max = (tmax * (1.0-p1) + tmin * p1);
         min = (tmax * (1.0-p2) + tmin * p2);

         // Waveform view - allow zooming down to a range of ZOOMLIMIT
         if (max - min < ZOOMLIMIT) {     // if user attempts to go smaller...
            const float c = (min+max)/2;  // ...set centre of view to centre of dragged area and top/bottom to ZOOMLIMIT/2 above/below
            min = c - ZOOMLIMIT/2.0;
            max = c + ZOOMLIMIT/2.0;
         }
      }
   }
   else if (shiftDown || rightUp) {
      // Zoom OUT
      if (spectral) {
         if (shiftDown && rightUp) {
            // Zoom out full
            min = spectrumLinear ? 0.0f : 1.0f;
            max = halfrate;
         }
         else {
            // Zoom out

            const float p1 = (zoomStart - ypos) / (float)height;
            // (Used to zoom out centered at midline, ignoring the click, if linear view.
            //  I think it is better to be consistent.  PRL)
            // Center zoom-out at the midline
            const float middle = // spectrumLinear ? 0.5f :
               1.0f - p1;

            if (fixedMousePoint) {
               min = std::max(spectrumLinear ? 0.0f : 1.0f, scale.PositionToValue(-middle));
               max = std::min(halfrate, scale.PositionToValue(1.0f + p1));
            }
            else {
               min = std::max(spectrumLinear ? 0.0f : 1.0f, scale.PositionToValue(middle - 1.0f));
               max = std::min(halfrate, scale.PositionToValue(middle + 1.0f));
            }
         }
      }
      else {
         // Zoom out to -1.0...1.0 first, then, and only
         // then, if they click again, allow one more
         // zoom out.
         if (shiftDown && rightUp) {
            // Zoom out full
            min = -1.0;
            max = 1.0;
         }
         else {
            // Zoom out
            const WaveformSettings &settings = track->GetWaveformSettings();
            const bool linear = settings.isLinear();
            const float top = linear
               ? 2.0
               : (LINEAR_TO_DB(2.0) + settings.dBRange) / settings.dBRange;
            if (min <= -1.0 && max >= 1.0) {
               // Go to the maximal zoom-out
               min = -top;
               max = top;
            }
            else {
               // limit to +/- 1 range unless already outside that range...
               float minRange = (min < -1) ? -top : -1.0;
               float maxRange = (max > 1) ? top : 1.0;
               // and enforce vertical zoom limits.
               const float p1 = (zoomStart - ypos) / (float)height;
               if (fixedMousePoint) {
                  const float oldRange = max - min;
                  const float c = (max * (1.0 - p1) + min * p1);
                  min = std::min(maxRange - ZOOMLIMIT,
                     std::max(minRange, c - 2 * (1.0f - p1) * oldRange));
                  max = std::max(minRange + ZOOMLIMIT,
                     std::min(maxRange, c + 2 * p1 * oldRange));
               }
               else {
                  const float c = p1 * min + (1 - p1) * max;
                  const float l = (max - min);
                  min = std::min(maxRange - ZOOMLIMIT,
                     std::max(minRange, c - l));
                  max = std::max(minRange + ZOOMLIMIT,
                     std::min(maxRange, c + l));
               }
            }
         }
      }
   }
   else {
      // Zoom IN
      if (spectral) {
         // Center the zoom-in at the click
         const float p1 = (zoomStart - ypos) / (float)height;
         const float middle = 1.0f - p1;
         const float middleValue = scale.PositionToValue(middle);

         if (fixedMousePoint) {
            min = std::max(spectrumLinear ? 0.0f : 1.0f,
               std::min(middleValue - minBand * middle,
               scale.PositionToValue(0.5f * middle)
            ));
            max = std::min(halfrate,
               std::max(middleValue + minBand * p1,
               scale.PositionToValue(middle + 0.5f * p1)
            ));
         }
         else {
            min = std::max(spectrumLinear ? 0.0f : 1.0f,
               std::min(middleValue - minBand / 2,
               scale.PositionToValue(middle - 0.25f)
            ));
            max = std::min(halfrate,
               std::max(middleValue + minBand / 2,
               scale.PositionToValue(middle + 0.25f)
            ));
         }
      }
      else {
         // Zoom in centered on cursor
         if (min < -1.0 || max > 1.0) {
            min = -1.0;
            max = 1.0;
         }
         else {
            // Enforce maximum vertical zoom
            const float oldRange = max - min;
            const float l = std::max(ZOOMLIMIT, 0.5f * oldRange);
            const float ratio = l / (max - min);

            const float p1 = (zoomStart - ypos) / (float)height;
            const float c = (max * (1.0 - p1) + min * p1);
            if (fixedMousePoint)
               min = c - ratio * (1.0f - p1) * oldRange,
               max = c + ratio * p1 * oldRange;
            else
               min = c - 0.5 * l,
               max = c + 0.5 * l;
         }
      }
   }

   if (spectral) {
      track->SetSpectrumBounds(min, max);
      if (partner)
         partner->SetSpectrumBounds(min, max);
   }
   else {
      track->SetDisplayBounds(min, max);
      if (partner)
         partner->SetDisplayBounds(min, max);
   }
}

void TrackPanel::UpdateViewIfNoTracks()
{
   if (mTracks->IsEmpty())
   {
      // Be sure not to keep a dangling pointer
      SetCapturedTrack(NULL);

      // BG: There are no more tracks on screen
      //BG: Set zoom to normal
      mViewInfo->SetZoom(ZoomInfo::GetDefaultZoom());

      //STM: Set selection to 0,0
      //PRL: and default the rest of the selection information
      mViewInfo->selectedRegion = SelectedRegion();

      // PRL:  Following causes the time ruler to align 0 with left edge.
      // Bug 972
      mViewInfo->h = 0;

      mListener->TP_RedrawScrollbars();
      mListener->TP_HandleResize();
      mListener->TP_DisplayStatusMessage(wxT("")); //STM: Clear message if all tracks are removed
   }
}

// The tracks positions within the list have changed, so update the vertical
// ruler size for the track that triggered the event.
void TrackPanel::OnTrackListResized(wxCommandEvent & e)
{
   Track *t = (Track *) e.GetClientData();

   UpdateVRuler(t);

   e.Skip();
}

// Tracks have been added or removed from the list.  Handle adds as if
// a resize has taken place.
void TrackPanel::OnTrackListUpdated(wxCommandEvent & e)
{
   // Tracks may have been deleted, so check to see if the focused track was on of them.
   if (!mTracks->Contains(GetFocusedTrack())) {
      SetFocusedTrack(NULL);
   }

   if (e.GetClientData()) {
      OnTrackListResized(e);
      return;
   }

   e.Skip();
}

void TrackPanel::OnContextMenu(wxContextMenuEvent & WXUNUSED(event))
{
   OnTrackMenu();
}

/// This handles when the user clicks on the "Label" area
/// of a track, ie the part with all the buttons and the drop
/// down menu, etc.
// That is, TrackInfo and vertical ruler rect.
void TrackPanel::HandleLabelClick(wxMouseEvent & event)
{
   // AS: If not a click, ignore the mouse event.
   if (!event.ButtonDown() && !event.ButtonDClick()) {
      return;
   }

   // MIDI tracks use the right mouse button, but other tracks get confused
   // if they see anything other than a left click.
   bool isleft = event.Button(wxMOUSE_BTN_LEFT);

   bool unsafe = IsUnsafe();

   wxRect rect;

   Track *t = FindTrack(event.m_x, event.m_y, true, true, &rect);

   // VJ: Check sync-lock icon and the blank area to the left of the minimize button.
   // Have to do it here, because if track is shrunk such that these areas occlude controls,
   // e.g., mute/solo, don't want the "Funcs" below to set up handling.
   // Only result of doing so is to select the track. Don't care whether isleft.
   bool bTrackSelClick = TrackInfo::TrackSelFunc(t, rect, event.m_x, event.m_y);
   if (!bTrackSelClick)
   {
#ifdef USE_MIDI
      // DM: If it's a NoteTrack, it has special controls
      if (t->GetKind() == Track::Note)
      {
         wxRect midiRect;
#ifdef EXPERIMENTAL_MIDI_OUT
            // this is an awful hack: make a new rectangle at an offset because
            // MuteSoloFunc thinks buttons are located below some text, e.g.
            // "Mono, 44100Hz 32-bit float", but this is not true for a Note track
            wxRect muteSoloRect(rect);
            muteSoloRect.y -= 34; // subtract the height of wave track text
            if (MuteSoloFunc(t, muteSoloRect, event.m_x, event.m_y, false) ||
                MuteSoloFunc(t, muteSoloRect, event.m_x, event.m_y, true))
               return;

            // this is a similar hack: GainFunc expects a Wave track slider, so it's
            // looking in the wrong place. We pass it a bogus rectangle created when
            // the slider was placed to "fake" GainFunc into finding the slider in
            // its actual location.
            if (GainFunc(t, ((NoteTrack *) t)->GetGainPlacementRect(),
                         event, event.m_x, event.m_y))
               return;
#endif
         mTrackInfo.GetTrackControlsRect(rect, midiRect);
         if (midiRect.Contains(event.m_x, event.m_y) &&
             ((NoteTrack *) t)->LabelClick(midiRect, event.m_x, event.m_y,
                                      event.Button(wxMOUSE_BTN_RIGHT))) {
            Refresh(false);
            return;
         }
      }
#endif // USE_MIDI

   }

   if (!isleft) {
      return;
   }

   // DM: If they weren't clicking on a particular part of a track label,
   //  deselect other tracks and select this one.

   // JH: also, capture the current track for rearranging, so the user
   //  can drag the track up or down to swap it with others
   if (!unsafe) {
      mRearrangeCount = 0;
      SetCapturedTrack( t, IsRearranging );
      TrackPanel::CalculateRearrangingThresholds(event);
   }

   // AS: If the shift button is being held down, invert
   //  the selection on this track.
   if (event.ShiftDown()) {
      mTracks->Select(t, !t->GetSelected());
      Refresh(false);
      MixerBoard* pMixerBoard = this->GetMixerBoard();
      if (pMixerBoard && (t->GetKind() == Track::Wave))
         pMixerBoard->RefreshTrackCluster((WaveTrack*)t);
      return;
   }

   SelectNone();
   mTracks->Select(t);
   SetFocusedTrack(t);
   SelectTrackLength(t);

   this->Refresh(false);
   MixerBoard* pMixerBoard = this->GetMixerBoard();
   if (pMixerBoard)
      pMixerBoard->RefreshTrackClusters();

   if (!unsafe)
      MakeParentModifyState(true);
}

/// The user is dragging one of the tracks: change the track order
/// accordingly
void TrackPanel::HandleRearrange(wxMouseEvent & event)
{
   // are we finishing the drag?
   if (event.LeftUp()) {
      if (mRearrangeCount != 0) {
         wxString dir;
         dir = mRearrangeCount < 0 ? _("up") : _("down");
         MakeParentPushState(wxString::Format(_("Moved '%s' %s"),
            mCapturedTrack->GetName().c_str(),
            dir.c_str()),
            _("Move Track"));
      }

      SetCapturedTrack(NULL);
      SetCursor(*mArrowCursor);
      return;
   }

   // probably harmless during play?  However, we do disallow the click, so check this too.
   bool unsafe = IsUnsafe();
   if (unsafe)
      return;

   MixerBoard* pMixerBoard = this->GetMixerBoard(); // Update mixer board, too.
   if (event.m_y < mMoveUpThreshold || event.m_y < 0) {
      mTracks->MoveUp(mCapturedTrack);
      --mRearrangeCount;
#ifdef EXPERIMENTAL_MIDI_OUT
      if (pMixerBoard && (mCapturedTrack->GetKind() == Track::Wave ||
                          mCapturedTrack->GetKind() == Track::Note))
         pMixerBoard->MoveTrackCluster(mCapturedTrack, true /* up */);
#else
      if (pMixerBoard && (mCapturedTrack->GetKind() == Track::Wave))
         pMixerBoard->MoveTrackCluster((WaveTrack*)mCapturedTrack, true /* up */);
#endif
   }
   else if (event.m_y > mMoveDownThreshold || event.m_y > GetRect().GetHeight()) {
      mTracks->MoveDown(mCapturedTrack);
      ++mRearrangeCount;
      /* i18n-hint: a direction as in up or down.*/
#ifdef EXPERIMENTAL_MIDI_OUT
      if (pMixerBoard && (mCapturedTrack->GetKind() == Track::Wave ||
                          mCapturedTrack->GetKind() == Track::Note))
         pMixerBoard->MoveTrackCluster(mCapturedTrack, false /* down */);
#else
      if (pMixerBoard && (mCapturedTrack->GetKind() == Track::Wave))
         pMixerBoard->MoveTrackCluster((WaveTrack*)mCapturedTrack, false /* down */);
#endif
   }
   else
   {
      return;
   }

   // JH: if we moved up or down, recalculate the thresholds and make sure the
   // track is fully on-screen.
   TrackPanel::CalculateRearrangingThresholds(event);
   EnsureVisible(mCapturedTrack);
}

/// Figure out how far the user must drag the mouse up or down
/// before the track will swap with the one above or below
void TrackPanel::CalculateRearrangingThresholds(wxMouseEvent & event)
{
   wxASSERT(mCapturedTrack);

   // JH: this will probably need to be tweaked a bit, I'm just
   //   not sure what formula will have the best feel for the
   //   user.
   if (mTracks->CanMoveUp(mCapturedTrack))
      mMoveUpThreshold =
          event.m_y - mTracks->GetGroupHeight( mTracks->GetPrev(mCapturedTrack,true) );
   else
      mMoveUpThreshold = INT_MIN;

   if (mTracks->CanMoveDown(mCapturedTrack))
      mMoveDownThreshold =
          event.m_y + mTracks->GetGroupHeight( mTracks->GetNext(mCapturedTrack,true) );
   else
      mMoveDownThreshold = INT_MAX;
}

bool TrackInfo::TrackSelFunc(Track * WXUNUSED(t), wxRect rect, int x, int y)
{
   // First check the blank space to left of minimize button.
   wxRect selRect;
   GetMinimizeRect(rect, selRect); // for y and height
   selRect.x = rect.x;
   selRect.width = 16; // (kTrackInfoBtnSize)
   selRect.height++;
   if (selRect.Contains(x, y))
      return true;

   // Try the sync-lock rect.
   GetSyncLockIconRect(rect, selRect);
   selRect.height++;
   return selRect.Contains(x, y);
}

///  ButtonDown means they just clicked and haven't released yet.
///  We use this opportunity to save which track they clicked on,
///  and the initial height of the track, so as they drag we can
///  update the track size.
void TrackPanel::HandleResizeClick( wxMouseEvent & event )
{
   wxRect rTrack;
   wxRect rLabel;

   // DM: Figure out what track is about to be resized
   Track *track = FindTrack(event.m_x, event.m_y, false, false, &rTrack);

   if (!track) {
      // This will only return unlinked tracks or left channels of stereo tracks
      // or NULL:
      track = FindTrack(event.m_x, event.m_y, true, true, &rLabel);
      // If stereo, get the right channel.
      if (track && track->GetLinked())
         track = track->GetLink();
   }

   if (!track) {
      return;
   }

   mMouseClickY = event.m_y;

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   // To do: escape key
   if(MONO_WAVE_PAN(t)){
      //STM:  Determine whether we should rescale one or two tracks
      if (t->GetVirtualStereo()) {
         // mCapturedTrack is the lower track
         mInitialTrackHeight = t->GetHeight(true);
         mInitialUpperTrackHeight = t->GetHeight();
         SetCapturedTrack(t, IsResizingBelowLinkedTracks);
      }
      else {
         // mCapturedTrack is the upper track
         mInitialTrackHeight = t->GetHeight(true);
         mInitialUpperTrackHeight = t->GetHeight();
         SetCapturedTrack(t, IsResizingBetweenLinkedTracks);
      }
   }else{
      Track *prev = mTracks->GetPrev(t);
      Track *next = mTracks->GetNext(t);

      //STM:  Determine whether we should rescale one or two tracks
      if (prev && prev->GetLink() == t) {
         // mCapturedTrack is the lower track
         mInitialTrackHeight = t->GetHeight();
         mInitialMinimized = t->GetMinimized();
         mInitialUpperTrackHeight = prev->GetHeight();
         SetCapturedTrack(t, IsResizingBelowLinkedTracks);
      }
      else if (next && t->GetLink() == next) {
         // mCapturedTrack is the upper track
         mInitialTrackHeight = next->GetHeight();
         mInitialMinimized = next->GetMinimized();
         mInitialUpperTrackHeight = t->GetHeight();
         SetCapturedTrack(t, IsResizingBetweenLinkedTracks);
      }
      else {
         // DM: Save the initial mouse location and the initial height
         mInitialTrackHeight = t->GetHeight();
         mInitialMinimized = t->GetMinimized();
         SetCapturedTrack(t, IsResizing);
      }
   }
#else // EXPERIMENTAL_OUTPUT_DISPLAY
   Track *prev = mTracks->GetPrev(track);
   Track *next = mTracks->GetNext(track);

   //STM:  Determine whether we should rescale one or two tracks
   if (prev && prev->GetLink() == track) {
      // mCapturedTrack is the lower track
      mInitialTrackHeight = track->GetHeight();
      mInitialActualHeight = track->GetActualHeight();
      mInitialMinimized = track->GetMinimized();
      mInitialUpperTrackHeight = prev->GetHeight();
      mInitialUpperActualHeight = prev->GetActualHeight();
      SetCapturedTrack(track, IsResizingBelowLinkedTracks);
   }
   else if (next && track->GetLink() == next) {
      // mCapturedTrack is the upper track
      mInitialTrackHeight = next->GetHeight();
      mInitialActualHeight = next->GetActualHeight();
      mInitialMinimized = next->GetMinimized();
      mInitialUpperTrackHeight = track->GetHeight();
      mInitialUpperActualHeight = track->GetActualHeight();
      SetCapturedTrack(track, IsResizingBetweenLinkedTracks);
   }
   else {
      // DM: Save the initial mouse location and the initial height
      mInitialTrackHeight = track->GetHeight();
      mInitialActualHeight = track->GetActualHeight();
      mInitialMinimized = track->GetMinimized();
      SetCapturedTrack(track, IsResizing);
   }
#endif // EXPERIMENTAL_OUTPUT_DISPLAY
}

///  This happens when the button is released from a drag.
///  Since we actually took care of resizing the track when
///  we got drag events, all we have to do here is clean up.
///  We also modify the undo state (the action doesn't become
///  undo-able, but it gets merged with the previous undo-able
///  event).
void TrackPanel::HandleResizeButtonUp(wxMouseEvent & WXUNUSED(event))
{
   SetCapturedTrack( NULL );
   MakeParentRedrawScrollbars();
   MakeParentModifyState(false);
}

///  Resize dragging means that the mouse button IS down and has moved
///  from its initial location.  By the time we get here, we
///  have already received a ButtonDown() event and saved the
///  track being resized in mCapturedTrack.
void TrackPanel::HandleResizeDrag(wxMouseEvent & event)
{
   int delta = (event.m_y - mMouseClickY);

   // On first drag, jump out of minimized mode.  Initial height
   // will be height of minimized track.
   //
   // This used to be in HandleResizeClick(), but simply clicking
   // on a resize border would switch the minimized state.
   if (mCapturedTrack->GetMinimized()) {
      Track *link = mCapturedTrack->GetLink();

      mCapturedTrack->SetHeight(mCapturedTrack->GetHeight());
      mCapturedTrack->SetMinimized(false);

      if (link) {
         link->SetHeight(link->GetHeight());
         link->SetMinimized(false);
         // Initial values must be reset since they weren't based on the
         // minimized heights.
         mInitialUpperTrackHeight = link->GetHeight();
         mInitialTrackHeight = mCapturedTrack->GetHeight();
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      else if(MONO_WAVE_PAN(mCapturedTrack)){
         mCapturedTrack->SetMinimized(false);
         mInitialUpperTrackHeight = mCapturedTrack->GetHeight();
         mInitialTrackHeight = mCapturedTrack->GetHeight(true);
      }
#endif
   }

   //STM: We may be dragging one or two (stereo) tracks.
   // If two, resize proportionally if we are dragging the lower track, and
   // adjust compensatively if we are dragging the upper track.
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   switch( mMouseCapture )
   {
   case IsResizingBelowLinkedTracks:
      {
         if(MONO_WAVE_PAN(mCapturedTrack)){
            double proportion = static_cast < double >(mInitialTrackHeight)
                / (mInitialTrackHeight + mInitialUpperTrackHeight);

            int newTrackHeight = static_cast < int >
                (mInitialTrackHeight + delta * proportion);

            int newUpperTrackHeight = static_cast < int >
                (mInitialUpperTrackHeight + delta * (1.0 - proportion));

            //make sure neither track is smaller than its minimum height
            if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
               newTrackHeight = mCapturedTrack->GetMinimizedHeight();
            if (newUpperTrackHeight < mCapturedTrack->GetMinimizedHeight())
               newUpperTrackHeight = mCapturedTrack->GetMinimizedHeight();

            mCapturedTrack->SetHeight(newTrackHeight,true);
            mCapturedTrack->SetHeight(newUpperTrackHeight);
         }
         else{
            Track *prev = mTracks->GetPrev(mCapturedTrack);

            double proportion = static_cast < double >(mInitialTrackHeight)
                / (mInitialTrackHeight + mInitialUpperTrackHeight);

            int newTrackHeight = static_cast < int >
                (mInitialTrackHeight + delta * proportion);

            int newUpperTrackHeight = static_cast < int >
                (mInitialUpperTrackHeight + delta * (1.0 - proportion));

            //make sure neither track is smaller than its minimum height
            if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
               newTrackHeight = mCapturedTrack->GetMinimizedHeight();
            if (newUpperTrackHeight < prev->GetMinimizedHeight())
               newUpperTrackHeight = prev->GetMinimizedHeight();

            mCapturedTrack->SetHeight(newTrackHeight);
            prev->SetHeight(newUpperTrackHeight);
         }
         break;
      }
   case IsResizingBetweenLinkedTracks:
      {
         if(MONO_WAVE_PAN(mCapturedTrack)){
            int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
            int newTrackHeight = mInitialTrackHeight - delta;

            // make sure neither track is smaller than its minimum height
            if (newTrackHeight < mCapturedTrack->GetMinimizedHeight()) {
               newTrackHeight = mCapturedTrack->GetMinimizedHeight();
               newUpperTrackHeight =
                   mInitialUpperTrackHeight + mInitialTrackHeight - mCapturedTrack->GetMinimizedHeight();
            }
            if (newUpperTrackHeight < mCapturedTrack->GetMinimizedHeight()) {
               newUpperTrackHeight = mCapturedTrack->GetMinimizedHeight();
               newTrackHeight =
                   mInitialUpperTrackHeight + mInitialTrackHeight - mCapturedTrack->GetMinimizedHeight();
            }
            float temp = 1.0f;
            if(newUpperTrackHeight != 0.0f)
               temp = (float)newUpperTrackHeight/(float)(newUpperTrackHeight + newTrackHeight);

            mCapturedTrack->SetVirtualTrackPercentage(temp);
            mCapturedTrack->SetHeight(newUpperTrackHeight);
            mCapturedTrack->SetHeight(newTrackHeight,true);
         }
         else{
            Track *next = mTracks->GetNext(mCapturedTrack);
            int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
            int newTrackHeight = mInitialTrackHeight - delta;

            // make sure neither track is smaller than its minimum height
            if (newTrackHeight < next->GetMinimizedHeight()) {
               newTrackHeight = next->GetMinimizedHeight();
               newUpperTrackHeight =
                   mInitialUpperTrackHeight + mInitialTrackHeight - next->GetMinimizedHeight();
            }
            if (newUpperTrackHeight < mCapturedTrack->GetMinimizedHeight()) {
               newUpperTrackHeight = mCapturedTrack->GetMinimizedHeight();
               newTrackHeight =
                   mInitialUpperTrackHeight + mInitialTrackHeight - mCapturedTrack->GetMinimizedHeight();
            }

            mCapturedTrack->SetHeight(newUpperTrackHeight);
            next->SetHeight(newTrackHeight);
            break;
         }
         break;
      }
   case IsResizing:
      {
         int newTrackHeight = mInitialTrackHeight + delta;
         if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
            newTrackHeight = mCapturedTrack->GetMinimizedHeight();
         mCapturedTrack->SetHeight(newTrackHeight);
         break;
      }
   default:
      // don't refresh in this case.
      return;
   }
#else // EXPERIMENTAL_OUTPUT_DISPLAY
   switch( mMouseCapture )
   {
   case IsResizingBelowLinkedTracks:
      {
         Track *prev = mTracks->GetPrev(mCapturedTrack);

         double proportion = static_cast < double >(mInitialTrackHeight)
             / (mInitialTrackHeight + mInitialUpperTrackHeight);

         int newTrackHeight = static_cast < int >
             (mInitialTrackHeight + delta * proportion);

         int newUpperTrackHeight = static_cast < int >
             (mInitialUpperTrackHeight + delta * (1.0 - proportion));

         //make sure neither track is smaller than its minimum height
         if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
            newTrackHeight = mCapturedTrack->GetMinimizedHeight();
         if (newUpperTrackHeight < prev->GetMinimizedHeight())
            newUpperTrackHeight = prev->GetMinimizedHeight();

         mCapturedTrack->SetHeight(newTrackHeight);
         prev->SetHeight(newUpperTrackHeight);
         break;
      }
   case IsResizingBetweenLinkedTracks:
      {
         Track *next = mTracks->GetNext(mCapturedTrack);
         int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
         int newTrackHeight = mInitialTrackHeight - delta;

         // make sure neither track is smaller than its minimum height
         if (newTrackHeight < next->GetMinimizedHeight()) {
            newTrackHeight = next->GetMinimizedHeight();
            newUpperTrackHeight =
                mInitialUpperTrackHeight + mInitialTrackHeight - next->GetMinimizedHeight();
         }
         if (newUpperTrackHeight < mCapturedTrack->GetMinimizedHeight()) {
            newUpperTrackHeight = mCapturedTrack->GetMinimizedHeight();
            newTrackHeight =
                mInitialUpperTrackHeight + mInitialTrackHeight - mCapturedTrack->GetMinimizedHeight();
         }

         mCapturedTrack->SetHeight(newUpperTrackHeight);
         next->SetHeight(newTrackHeight);
         break;
      }
   case IsResizing:
      {
         int newTrackHeight = mInitialTrackHeight + delta;
         if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
            newTrackHeight = mCapturedTrack->GetMinimizedHeight();
         mCapturedTrack->SetHeight(newTrackHeight);
         break;
      }
   default:
      // don't refresh in this case.
      return;
   }
#endif // EXPERIMENTAL_OUTPUT_DISPLAY
   Refresh(false);
}

/// HandleResize gets called when:
///  - A mouse-down event occurs in the "resize region" of a track,
///    i.e. to change its vertical height.
///  - A mouse event occurs and mIsResizing==true (i.e. while
///    the resize is going on)
void TrackPanel::HandleResize(wxMouseEvent & event)
{
   if (event.LeftDown()) {
      HandleResizeClick( event );
   }
   else if (event.LeftUp())
   {
      HandleResizeButtonUp( event );
   }
   else if (event.Dragging()) {
      HandleResizeDrag( event );
   }
}

/// Handle mouse wheel rotation (for zoom in/out, vertical and horizontal scrolling)
void TrackPanel::HandleWheelRotation(wxMouseEvent & event)
{
   // Delegate wheel handling to the cell under the mouse, if it knows how.
   {
      wxRect inner;
      TrackPanelCell *pCell = NULL;
      Track *pTrack = NULL;
      FindCell(event, inner, pCell, pTrack);
      if (pCell) {
         unsigned result =
            pCell->HandleWheelRotation
            (TrackPanelMouseEvent(event, inner, pCell), GetProject());
         ProcessUIHandleResult(this, pTrack, pTrack, result);
         if (!(result & RefreshCode::Cancelled))
            return;
      }
   }

   if (GetTracks()->IsEmpty())
      // Scrolling and Zoom in and out commands are disabled when there are no tracks.
      // This should be disabled too for consistency.  Otherwise
      // you do see changes in the time ruler.
      return;

   // Special case of pointer in the vertical ruler
   {
      wxRect rect;
      Track *const pTrack = FindTrack(event.m_x, event.m_y, true, false, &rect);
      if (pTrack && event.m_x >= GetVRulerOffset()) {
         HandleWheelRotationInVRuler(event, pTrack, rect);
         return;
      }
   }

   double steps = event.m_wheelRotation /
      (event.m_wheelDelta > 0 ? (double)event.m_wheelDelta : 120.0);

   if (event.ShiftDown()
#ifdef EXPERIMENTAL_SCRUBBING_SMOOTH_SCROLL
       // Don't pan during smooth scrolling.  That would conflict with keeping
       // the play indicator centered.
       && !GetProject()->GetScrubber().IsScrollScrubbing()
#endif
      )
   {
      // MM: Scroll left/right when used with Shift key down
      mListener->TP_ScrollWindow(
         mViewInfo->OffsetTimeByPixels(
            mViewInfo->PositionToTime(0), 50.0 * -steps));
   }
   else if (event.CmdDown())
   {
#if 0
         // JKC: Alternative scroll wheel zooming code
         // using AudacityProject zooming, which is smarter,
         // it keeps selections on screen and centred if it can,
         // also this ensures mousewheel and zoom buttons give same result.
         double ZoomFactor = pow(2.0, steps);
         AudacityProject *p = GetProject();
         if( steps > 0 )
            p->ZoomInByFactor( ZoomFactor );
         else
            p->ZoomOutByFactor( ZoomFactor );
#endif
      // MM: Zoom in/out when used with Control key down
      // We're converting pixel positions to times,
      // counting pixels from the left edge of the track.
      int trackLeftEdge = GetLeftOffset();

      // Time corresponding to mouse position
      wxCoord xx;
      double center_h;
#ifdef EXPERIMENTAL_SCRUBBING_SMOOTH_SCROLL
      if (GetProject()->GetScrubber().IsScrollScrubbing()) {
         // Expand or contract about the center, ignoring mouse position
         center_h = mViewInfo->h + (GetScreenEndTime() - mViewInfo->h) / 2.0;
         xx = mViewInfo->TimeToPosition(center_h, trackLeftEdge);
      }
      else
#endif
      {
         xx = event.m_x;
         center_h = mViewInfo->PositionToTime(xx, trackLeftEdge);
      }
      // Time corresponding to last (most far right) audio.
      double audioEndTime = mTracks->GetEndTime();

      // When zooming in in empty space, it's easy to 'lose' the waveform.
      // This prevents it.
      // IF zooming in
      if (steps > 0)
      {
         // IF mouse is to right of audio
         if (center_h > audioEndTime)
            // Zooming brings far right of audio to mouse.
            center_h = audioEndTime;
      }

      mViewInfo->ZoomBy(pow(2.0, steps));

      double new_center_h = mViewInfo->PositionToTime(xx, trackLeftEdge);
      mViewInfo->h += (center_h - new_center_h);

      MakeParentRedrawScrollbars();
      Refresh(false);
   }
   else
   {
#ifdef EXPERIMENTAL_SCRUBBING_SCROLL_WHEEL
      if (GetProject()->GetScrubber().IsScrubbing()) {
         GetProject()->GetScrubber().HandleScrollWheel(steps);
      }
      else
#endif
      {
         // MM: Scroll up/down when used without modifier keys
         double lines = steps * 4 + mVertScrollRemainder;
         mVertScrollRemainder = lines - floor(lines);
         lines = floor(lines);
         mListener->TP_ScrollUpDown((int)-lines);
      }
   }
}

void TrackPanel::HandleWheelRotationInVRuler
   (wxMouseEvent &event, Track *pTrack, const wxRect &rect)
{
   double steps = event.m_wheelRotation /
      (event.m_wheelDelta > 0 ? (double)event.m_wheelDelta : 120.0);

   if (pTrack->GetKind() == Track::Wave) {
      WaveTrack *const wt = static_cast<WaveTrack*>(pTrack);
      WaveTrack *const partner = static_cast<WaveTrack*>(wt->GetLink());
      const bool isDB =
         wt->GetDisplay() == WaveTrack::Waveform &&
         wt->GetWaveformSettings().scaleType == WaveformSettings::stLogarithmic;
      if (isDB && event.ShiftDown()) {
         // Special cases for Waveform dB only

         // Vary the bottom of the dB scale, but only if the midline is visible
         float min, max;
         wt->GetDisplayBounds(&min, &max);
         if (!(min < 0.0 && max > 0.0))
            return;

         WaveformSettings &settings = wt->GetIndependentWaveformSettings();
         float olddBRange = settings.dBRange;
         if (event.m_wheelRotation < 0)
            // Zoom out
            settings.NextLowerDBRange();
         else
            settings.NextHigherDBRange();
         float newdBRange = settings.dBRange;

         if (partner) {
            WaveformSettings &settings = partner->GetIndependentWaveformSettings();
            if (event.m_wheelRotation < 0)
               // Zoom out
               settings.NextLowerDBRange();
            else
               settings.NextHigherDBRange();
         }

         if (!event.CmdDown()) {
            // extra-special case that varies the db limit without changing
            // magnification
            const float extreme = (LINEAR_TO_DB(2) + newdBRange) / newdBRange;
            max = std::min(extreme, max * olddBRange / newdBRange);
            min = std::max(-extreme, min * olddBRange / newdBRange);
            wt->SetLastdBRange();
            wt->SetDisplayBounds(min, max);
            if (partner) {
               partner->SetLastdBRange();
               partner->SetDisplayBounds(min, max);
            }
         }
      }
      else if (event.CmdDown() && !event.ShiftDown()) {
         HandleWaveTrackVZoom(
            mTracks, rect, event.m_y, event.m_y,
            wt, false, (event.m_wheelRotation < 0),
            true);
      }
      else if (!(event.CmdDown() || event.ShiftDown())) {
         // Scroll some fixed number of pixels, independent of zoom level or track height:
         static const float movement = 10.0f;
         const int height = wt->GetHeight() - (kTopMargin + kBottomMargin);
         const bool spectral = (wt->GetDisplay() == WaveTrack::Spectrum);
         if (spectral) {
            const float delta = steps * movement / height;
            SpectrogramSettings &settings = wt->GetIndependentSpectrogramSettings();
            const bool isLinear = settings.scaleType == SpectrogramSettings::stLinear;
            float bottom, top;
            wt->GetSpectrumBounds(&bottom, &top);
            const double rate = wt->GetRate();
            const float bound = rate / 2;
            const NumberScale numberScale(settings.GetScale(bottom, top, rate, false));
            float newTop =
               std::min(bound, numberScale.PositionToValue(1.0f + delta));
            const float newBottom =
               std::max((isLinear ? 0.0f : 1.0f),
                        numberScale.PositionToValue(numberScale.ValueToPosition(newTop) - 1.0f));
            newTop =
               std::min(bound,
                        numberScale.PositionToValue(numberScale.ValueToPosition(newBottom) + 1.0f));

            wt->SetSpectrumBounds(newBottom, newTop);
            if (partner)
               partner->SetSpectrumBounds(newBottom, newTop);
         }
         else {
            float topLimit = 2.0;
            if (isDB) {
               const float dBRange = wt->GetWaveformSettings().dBRange;
               topLimit = (LINEAR_TO_DB(topLimit) + dBRange) / dBRange;
            }
            const float bottomLimit = -topLimit;
            float top, bottom;
            wt->GetDisplayBounds(&bottom, &top);
            const float range = top - bottom;
            const float delta = range * steps * movement / height;
            float newTop = std::min(topLimit, top + delta);
            const float newBottom = std::max(bottomLimit, newTop - range);
            newTop = std::min(topLimit, newBottom + range);
            wt->SetDisplayBounds(newBottom, newTop);
            if (partner)
               partner->SetDisplayBounds(newBottom, newTop);
         }
      }
      else
         return;

      UpdateVRuler(pTrack);
      Refresh(false);
      MakeParentModifyState(true);
   }
   else {
      // To do: time track?  Note track?
   }
   return;
}

/// Filter captured keys typed into LabelTracks.
void TrackPanel::OnCaptureKey(wxCommandEvent & event)
{
   Track * const t = GetFocusedTrack();
   if (t && t->GetKind() == Track::Label) {
      wxKeyEvent *kevent = (wxKeyEvent *)event.GetEventObject();
      event.Skip(!((LabelTrack *)t)->CaptureKey(*kevent));
   }
   else
   if (t) {
      wxKeyEvent *kevent = static_cast<wxKeyEvent *>(event.GetEventObject());
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->CaptureKey(*kevent, *mViewInfo, this);
      ProcessUIHandleResult(this, t, t, refreshResult);
   }
   else
      event.Skip();
}

void TrackPanel::OnKeyDown(wxKeyEvent & event)
{
   switch (event.GetKeyCode())
   {
   case WXK_ESCAPE:
      HandleEscapeKey(true);
      break;

   case WXK_ALT:
      HandleAltKey(true);
      break;

   case WXK_SHIFT:
      HandleShiftKey(true);
      break;

   case WXK_CONTROL:
      HandleControlKey(true);
      break;

      // Allow PageUp and PageDown keys to
      //scroll the Track Panel left and right
   case WXK_PAGEUP:
      HandlePageUpKey();
      return;

   case WXK_PAGEDOWN:
      HandlePageDownKey();
      return;
   }

   Track *const t = GetFocusedTrack();

   if (t && t->GetKind() == Track::Label) {
      LabelTrack *lt = (LabelTrack *)t;
      double bkpSel0 = mViewInfo->selectedRegion.t0(),
         bkpSel1 = mViewInfo->selectedRegion.t1();

      // Pass keystroke to labeltrack's handler and add to history if any
      // updates were done
      if (lt->OnKeyDown(mViewInfo->selectedRegion, event))
         MakeParentPushState(_("Modified Label"),
         _("Label Edit"),
         PUSH_CONSOLIDATE);

      // Make sure caret is in view
      int x;
      if (lt->CalcCursorX(this, &x)) {
         ScrollIntoView(x);
      }

      // If selection modified, refresh
      // Otherwise, refresh track display if the keystroke was handled
      if (bkpSel0 != mViewInfo->selectedRegion.t0() ||
         bkpSel1 != mViewInfo->selectedRegion.t1())
         Refresh(false);
      else if (!event.GetSkipped())
         RefreshTrack(t);
   }
   else
   if (t) {
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->KeyDown(event, *mViewInfo, this);
      ProcessUIHandleResult(this, t, t, refreshResult);
   }
   else
      event.Skip();
}

void TrackPanel::OnChar(wxKeyEvent & event)
{
   switch (event.GetKeyCode())
   {
   case WXK_ESCAPE:
   case WXK_ALT:
   case WXK_SHIFT:
   case WXK_CONTROL:
   case WXK_PAGEUP:
   case WXK_PAGEDOWN:
      return;
   }

   Track *const t = GetFocusedTrack();
   if (t && t->GetKind() == Track::Label) {
      double bkpSel0 = mViewInfo->selectedRegion.t0(),
         bkpSel1 = mViewInfo->selectedRegion.t1();
      // Pass keystroke to labeltrack's handler and add to history if any
      // updates were done
      if (((LabelTrack *)t)->OnChar(mViewInfo->selectedRegion, event))
         MakeParentPushState(_("Modified Label"),
         _("Label Edit"),
         PUSH_CONSOLIDATE);

      // If selection modified, refresh
      // Otherwise, refresh track display if the keystroke was handled
      if (bkpSel0 != mViewInfo->selectedRegion.t0() ||
         bkpSel1 != mViewInfo->selectedRegion.t1())
         Refresh(false);
      else if (!event.GetSkipped())
         RefreshTrack(t);
   }
   else
   if (t) {
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->Char(event, *mViewInfo, this);
      ProcessUIHandleResult(this, t, t, refreshResult);
   }
   else
      event.Skip();
}

void TrackPanel::OnKeyUp(wxKeyEvent & event)
{
   switch (event.GetKeyCode())
   {
   case WXK_ESCAPE:
      HandleEscapeKey(false);
      break;
   case WXK_ALT:
      HandleAltKey(false);
      break;

   case WXK_SHIFT:
      HandleShiftKey(false);
      break;

   case WXK_CONTROL:
      HandleControlKey(false);
      break;
   }

   Track * const t = GetFocusedTrack();
   if (t) {
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->KeyUp(event, *mViewInfo, this);
      ProcessUIHandleResult(this, t, t, refreshResult);
   }
   else
      event.Skip();
}

/// Should handle the case when the mouse capture is lost.
void TrackPanel::OnCaptureLost(wxMouseCaptureLostEvent & WXUNUSED(event))
{
   wxMouseEvent e(wxEVT_LEFT_UP);

   e.m_x = mMouseMostRecentX;
   e.m_y = mMouseMostRecentY;

   OnMouseEvent(e);
}

/// This handles just generic mouse events.  Then, based
/// on our current state, we forward the mouse events to
/// various interested parties.
void TrackPanel::OnMouseEvent(wxMouseEvent & event)
{
   if (event.m_wheelRotation != 0)
      HandleWheelRotation(event);

   if (!mAutoScrolling) {
      mMouseMostRecentX = event.m_x;
      mMouseMostRecentY = event.m_y;
   }

   if (event.LeftDown()) {
      mCapturedTrack = NULL;

      // The activate event is used to make the
      // parent window 'come alive' if it didn't have focus.
      wxActivateEvent e;
      GetParent()->GetEventHandler()->ProcessEvent(e);

      // wxTimers seem to be a little unreliable, so this
      // "primes" it to make sure it keeps going for a while...

      // When this timer fires, we call TrackPanel::OnTimer and
      // possibly update the screen for offscreen scrolling.
      mTimer.Stop();
      mTimer.Start(kTimerInterval, FALSE);
   }

   if (event.ButtonDown()) {
      SetFocus();
   }
   if (event.ButtonUp()) {
      if (HasCapture())
         ReleaseMouse();
   }

   if (event.Leaving() && !event.ButtonIsDown(wxMOUSE_BTN_ANY))
   {
      // PRL:  was this test really needed?  It interfered with my refactoring
      // that tried to eliminate those enum values.
      // I think it was never true, that mouse capture was pan or gain sliding,
      // but no mouse button was down.
      // if (mMouseCapture != IsPanSliding && mMouseCapture != IsGainSliding)
      {
         SetCapturedTrack(NULL);
#if defined(__WXMAC__)
         // We must install the cursor ourselves since the window under
         // the mouse is no longer this one and wx2.8.12 makes that check.
         // Should re-evaluate with wx3.
         wxSTANDARD_CURSOR->MacInstall();
#endif
      }
   }

   if (mUIHandle) {
      wxRect inner;
      TrackPanelCell *pCell;
      Track *pTrack;
      FindCell(event, inner, pCell, pTrack);

      if (event.Dragging()) {
         // UIHANDLE DRAG
         const UIHandle::Result refreshResult =
            mUIHandle->Drag(TrackPanelMouseEvent(event, inner, pCell), GetProject());
         ProcessUIHandleResult(this, mpClickedTrack, pTrack, refreshResult);
         if (refreshResult & RefreshCode::Cancelled) {
            // Drag decided to abort itself
            mUIHandle = NULL;
            mpClickedTrack = NULL;
            if (HasCapture())
               ReleaseMouse();
            // Should this be done?  As for cancelling?
            // HandleCursor(event);
         }
         else {
            HitTestPreview preview =
               mUIHandle->Preview(TrackPanelMouseEvent(event, inner, pCell), GetProject());
            mListener->TP_DisplayStatusMessage(preview.message);
            if (preview.cursor)
               SetCursor(*preview.cursor);
         }
      }
      else if (event.ButtonUp()) {
         // UIHANDLE RELEASE
         UIHandle::Result refreshResult =
            mUIHandle->Release(TrackPanelMouseEvent(event, inner, pCell), GetProject(), this);
         ProcessUIHandleResult(this, mpClickedTrack, pTrack, refreshResult);
         mUIHandle = NULL;
         mpClickedTrack = NULL;
         // ReleaseMouse() already done above
         // Should this be done?  As for cancelling?
         // HandleCursor(event);
      }
   }
   else switch( mMouseCapture ) {
   case IsVZooming:
      HandleVZoom(event);
      break;
   case IsResizing:
   case IsResizingBetweenLinkedTracks:
   case IsResizingBelowLinkedTracks:
      HandleResize(event);
      HandleCursor(event);
      break;
   case IsRearranging:
      HandleRearrange(event);
      break;
   case IsAdjustingLabel:
      HandleGlyphDragRelease(static_cast<LabelTrack *>(mCapturedTrack), event);
      break;
   case IsSelectingLabelText:
      HandleTextDragRelease(static_cast<LabelTrack *>(mCapturedTrack), event);
      break;
   default: //includes case of IsUncaptured
      // This is where most button-downs are detected
      HandleTrackSpecificMouseEvent(event);
      break;
   }

   if (event.ButtonDown() && IsMouseCaptured()) {
      if (!HasCapture())
         CaptureMouse();
   }

   //EnsureVisible should be called after the up-click.
   if (event.ButtonUp()) {
      wxRect rect;

      Track *t = FindTrack(event.m_x, event.m_y, false, false, &rect);
      if (t)
         EnsureVisible(t);
   }
}

/// Event has happened on a track and it has been determined to be a label track.
bool TrackPanel::HandleLabelTrackClick(LabelTrack * lTrack, wxRect &rect, wxMouseEvent & event)
{
   if (!event.ButtonDown())
      return false;

   if(event.LeftDown())
   {
      /// \todo This method is one of a large number of methods in
      /// TrackPanel which suitably modified belong in other classes.
      TrackListIterator iter(mTracks);
      Track *n = iter.First();

      while (n) {
         if (n->GetKind() == Track::Label && lTrack != n) {
            ((LabelTrack *)n)->ResetFlags();
            ((LabelTrack *)n)->Unselect();
         }
         n = iter.Next();
      }
   }

   mCapturedRect = rect;

   lTrack->HandleClick(event, mCapturedRect, *mViewInfo, &mViewInfo->selectedRegion);

   if (lTrack->IsAdjustingLabel())
   {
      SetCapturedTrack(lTrack, IsAdjustingLabel);

      //If we are adjusting a label on a labeltrack, do not do anything
      //that follows. Instead, redraw the track.
      RefreshTrack(lTrack);
      return true;
   }

   // IF the user clicked a label, THEN select all other tracks by Label
   if (lTrack->IsSelected()) {
      mTracks->Select(lTrack);
      SelectTracksByLabel(lTrack);
      DisplaySelection();

      // Not starting a drag
      SetCapturedTrack(NULL, IsUncaptured);

      if(mCapturedTrack == NULL)
         SetCapturedTrack(lTrack, IsSelectingLabelText);
      // handle shift+mouse left button
      if (event.ShiftDown() && event.ButtonDown()) {
         // if the mouse is clicked in text box, set flags
         if (lTrack->OverTextBox(lTrack->GetLabel(lTrack->getSelectedIndex()), event.m_x, event.m_y)) {
            lTrack->SetInBox(true);
            lTrack->SetDragXPos(event.m_x);
            lTrack->SetResetCursorPos(true);
            RefreshTrack(lTrack);
            return true;
         }
      }
      return true;
   }

   // handle shift+ctrl down
   /*if (event.ShiftDown()) { // && event.ControlDown()) {
      lTrack->SetHighlightedByKey(true);
      Refresh(false);
      return;
   }*/

   // return false, there is more to do...
   return false;
}

/// Event has happened on a track and it has been determined to be a label track.
void TrackPanel::HandleGlyphDragRelease(LabelTrack * lTrack, wxMouseEvent & event)
{
   if (!lTrack)
      return;

   /// \todo This method is one of a large number of methods in
   /// TrackPanel which suitably modified belong in other classes.
   if (event.Dragging()) {
      ;
   } else if (event.LeftUp() && mCapturedTrack && (mCapturedTrack->GetKind() == Track::Label)) {
      SetCapturedTrack(NULL);
   }

   if (lTrack->HandleGlyphDragRelease(event, mCapturedRect,
      *mViewInfo, &mViewInfo->selectedRegion)) {
      MakeParentPushState(_("Modified Label"),
         _("Label Edit"),
         PUSH_CONSOLIDATE);
   }

   //If we are adjusting a label on a labeltrack, do not do anything
   //that follows. Instead, redraw the track.
   RefreshTrack(lTrack);
   return;
}

/// Event has happened on a track and it has been determined to be a label track.
void TrackPanel::HandleTextDragRelease(LabelTrack * lTrack, wxMouseEvent & event)
{
   if (!lTrack)
      return;

   lTrack->HandleTextDragRelease(event);

   /// \todo This method is one of a large number of methods in
   /// TrackPanel which suitably modified belong in other classes.
   if (event.Dragging()) {
      ;
   } else if (event.LeftUp() && mCapturedTrack && (mCapturedTrack->GetKind() == Track::Label)) {
      SetCapturedTrack(NULL);
   }

   // handle dragging
   if (event.Dragging()) {
      // locate the initial mouse position
      if (event.LeftIsDown()) {
         if (mLabelTrackStartXPos == -1) {
            mLabelTrackStartXPos = event.m_x;
            mLabelTrackStartYPos = event.m_y;

            if ((lTrack->getSelectedIndex() != -1) &&
               lTrack->OverTextBox(
               lTrack->GetLabel(lTrack->getSelectedIndex()),
               mLabelTrackStartXPos,
               mLabelTrackStartYPos))
            {
               mLabelTrackStartYPos = -1;
            }
         }
         // if initial mouse position in the text box
         // then only drag text
         if (mLabelTrackStartYPos == -1) {
            RefreshTrack(lTrack);
            return;
         }
      }
   }

   // handle mouse left button up
   if (event.LeftUp()) {
      mLabelTrackStartXPos = -1;
   }
}

// AS: I don't really understand why this code is sectioned off
//  from the other OnMouseEvent code.
void TrackPanel::HandleTrackSpecificMouseEvent(wxMouseEvent & event)
{
   Track * pTrack;
   TrackPanelCell *pCell;
   wxRect inner;
   FindCell(event, inner, pCell, pTrack);

   //call HandleResize if I'm over the border area
   // (Note:  add back bottom border thickness)
   if (event.LeftDown() &&
       (within(event.m_y, inner.y + inner.height + kBorderThickness, TRACK_RESIZE_REGION))) {
      HandleResize(event);
      HandleCursor(event);
      return;
   }

   // AS: If the user clicked outside all tracks, make nothing
   //  selected.
   if ((event.ButtonDown() || event.ButtonDClick()) && !pTrack) {
      SelectNone();
      Refresh(false);
      return;
   }

   if (mUIHandle == NULL &&
       event.m_x < GetLeftOffset()) {
      //Determine if user clicked on the track's left-hand label
      if (!mUIHandle &&
         event.m_x < GetLeftOffset() &&
         pTrack &&
         (event.ButtonDown() || event.ButtonDClick())) {
         if (pCell)
            mUIHandle = pCell->HitTest(TrackPanelMouseEvent(event, inner), GetProject()).handle;
      }

      if (mUIHandle) {
         // UIHANDLE CLICK
         UIHandle::Result refreshResult =
            mUIHandle->Click(TrackPanelMouseEvent(event, inner, pCell), GetProject());
         if (refreshResult & RefreshCode::Cancelled)
            mUIHandle = NULL;
         else
            mpClickedTrack = pTrack;
         ProcessUIHandleResult(this, pTrack, pTrack, refreshResult);
      }

      else {
         if (event.m_x >= GetVRulerOffset()) {
            if (!event.Dragging()) // JKC: Only want the mouse down event.
               HandleVZoom(event);
         }
         else
            HandleLabelClick(event);
      }
      HandleCursor(event);
      return;
   }

   // To do: remove the following special things
   // so that we can coalesce the code for track and non-track clicks

   //Determine if user clicked on a label track.
   //If so, use MouseDown handler for the label track.
   if (pTrack && (pTrack->GetKind() == Track::Label))
   {
      if (HandleLabelTrackClick((LabelTrack *)pTrack, inner, event))
         return;
   }

#ifdef EXPERIMENTAL_SCRUBBING_BASIC
   if (GetProject()->GetScrubber().IsScrubbing() &&
       GetRect().Contains(event.GetPosition()) &&
       (!pTrack ||
        pTrack->GetKind() == Track::Wave)) {
      if (event.LeftDown()) {
         GetProject()->GetScrubber().SetSeeking();
         return;
      }
      else if (event.LeftIsDown())
         return;
   }
#endif

   bool handled = false;

   ToolsToolBar * pTtb = mListener->TP_GetToolsToolBar();
   if( !handled && pTtb != NULL )
   {
      if (mUIHandle == NULL &&
          pCell &&
          (event.ButtonDown() || event.ButtonDClick()) &&
          NULL != (mUIHandle =
             pCell->HitTest(TrackPanelMouseEvent(event, inner), GetProject()).handle)) {
         // UIHANDLE CLICK
         UIHandle::Result refreshResult =
            mUIHandle->Click(TrackPanelMouseEvent(event, inner, pCell), GetProject());
         if (refreshResult & RefreshCode::Cancelled)
            mUIHandle = NULL;
         else
            mpClickedTrack = pTrack;
         ProcessUIHandleResult(this, pTrack, pTrack, refreshResult);
      }
      else {
         int toolToUse = DetermineToolToUse(pTtb, event);

         switch (toolToUse) {
         case selectTool:
            HandleSelect(event);
            break;
         }
      }
   }

   if ((event.Moving() || event.LeftUp())  &&
       (mMouseCapture == IsUncaptured ))
//       (mMouseCapture != IsSelecting ) &&
//       (mMouseCapture != IsEnveloping) &&
//       (mMouseCapture != IsSliding) )
   {
      HandleCursor(event);
   }
   if (event.LeftUp()) {
      mCapturedTrack = NULL;
   }
}

/// If we are in multimode, looks at the type of track and where we are on it to
/// determine what object we are hovering over and hence what tool to use.
/// @param pTtb - A pointer to the tools tool bar
/// @param event - Mouse event, with info about position and what mouse buttons are down.
int TrackPanel::DetermineToolToUse( ToolsToolBar * pTtb, wxMouseEvent & event)
{
   int currentTool = pTtb->GetCurrentTool();

   // Unless in Multimode keep using the current tool.
   if( !pTtb->IsDown(multiTool) )
      return currentTool;

   // We NEVER change tools whilst we are dragging.
   if( event.Dragging() || event.LeftUp() )
      return currentTool;

   // Just like dragging.
   // But, this event might be the final button up
   // so keep the same tool.
//   if( mIsSliding || mIsSelecting || mIsEnveloping )
   if( mMouseCapture != IsUncaptured )
      return currentTool;

   // So now we have to find out what we are near to..
   wxRect rect;

   Track *pTrack = FindTrack(event.m_x, event.m_y, false, false, &rect);
   if( !pTrack )
      return currentTool;

   int trackKind = pTrack->GetKind();
   currentTool = selectTool; // the default.

   if( trackKind == Track::Label ){
      currentTool = selectTool;
   } else if( trackKind != Track::Wave) {
      currentTool = selectTool;
   // So we are in a wave track.
   //FIXME: Not necessarily. Haven't checked Track::Note (#if defined(USE_MIDI)).
   // From here on the order in which we hit test determines
   // which tool takes priority in the rare cases where it
   // could be more than one.
   }

   //Use the false argument since in multimode we don't
   //want the button indicating which tool is in use to be updated.
   pTtb->SetCurrentTool( currentTool, false );
   return currentTool;
}


#ifdef USE_MIDI
bool TrackPanel::HitTestStretch(Track *track, wxRect &rect, wxMouseEvent & event)
{
   // later, we may want a different policy, but for now, stretch is
   // selected when the cursor is near the center of the track and
   // within the selection
   if (!track || !track->GetSelected() || track->GetKind() != Track::Note ||
       IsUnsafe()) {
      return false;
   }
   int center = rect.y + rect.height / 2;
   int distance = abs(event.m_y - center);
   const int yTolerance = 10;
   wxInt64 leftSel = mViewInfo->TimeToPosition(mViewInfo->selectedRegion.t0(), rect.x);
   wxInt64 rightSel = mViewInfo->TimeToPosition(mViewInfo->selectedRegion.t1(), rect.x);
   // Something is wrong if right edge comes before left edge
   wxASSERT(!(rightSel < leftSel));
   return (leftSel <= event.m_x && event.m_x <= rightSel &&
           distance < yTolerance);
}
#endif

double TrackPanel::GetMostRecentXPos()
{
   return mViewInfo->PositionToTime(mMouseMostRecentX, GetLabelWidth());
}

void TrackPanel::RefreshTrack(Track *trk, bool refreshbacking)
{
   Track *link = trk->GetLink();

   if (link && !trk->GetLinked()) {
      trk = link;
      link = trk->GetLink();
   }

   // subtract insets and shadows from the rectangle, but not border
   // This matters because some separators do paint over the border
   wxRect rect(kLeftInset,
            -mViewInfo->vpos + trk->GetY() + kTopInset,
            GetRect().GetWidth() - kLeftInset - kRightInset - kShadowThickness,
            trk->GetHeight() - kTopInset - kShadowThickness);

   if (link) {
      rect.height += link->GetHeight();
   }

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   else if(MONO_WAVE_PAN(trk)){
      rect.height += trk->GetHeight(true);
   }
#endif

   if( refreshbacking )
   {
      mRefreshBacking = true;
   }

   Refresh( false, &rect );
}


/// This method overrides Refresh() of wxWindow so that the
/// boolean play indictaor can be set to false, so that an old play indicator that is
/// no longer there won't get  XORed (to erase it), thus redrawing it on the
/// TrackPanel
void TrackPanel::Refresh(bool eraseBackground /* = TRUE */,
                         const wxRect *rect /* = NULL */)
{
   // Tell OnPaint() to refresh the backing bitmap.
   //
   // Originally I had the check within the OnPaint() routine and it
   // was working fine.  That was until I found that, even though a full
   // refresh was requested, Windows only set the onscreen portion of a
   // window as damaged.
   //
   // So, if any part of the trackpanel was off the screen, full refreshes
   // didn't work and the display got corrupted.
   if( !rect || ( *rect == GetRect() ) )
   {
      mRefreshBacking = true;
   }
   wxWindow::Refresh(eraseBackground, rect);
   DisplaySelection();
}

/// Draw the actual track areas.  We only draw the borders
/// and the little buttons and menues and whatnot here, the
/// actual contents of each track are drawn by the TrackArtist.
void TrackPanel::DrawTracks(wxDC * dc)
{
   wxRegion region = GetUpdateRegion();

   wxRect clip = GetRect();

   wxRect panelRect = clip;
   panelRect.y = -mViewInfo->vpos;

   wxRect tracksRect = panelRect;
   tracksRect.x += GetLabelWidth();
   tracksRect.width -= GetLabelWidth();

   ToolsToolBar *pTtb = mListener->TP_GetToolsToolBar();
   bool bMultiToolDown = pTtb->IsDown(multiTool);
   bool envelopeFlag   = pTtb->IsDown(envelopeTool) || bMultiToolDown;
   bool bigPointsFlag  = pTtb->IsDown(drawTool) || bMultiToolDown;
   bool sliderFlag     = bMultiToolDown;

   // The track artist actually draws the stuff inside each track
   mTrackArtist->DrawTracks(mTracks, GetProject()->GetFirstVisible(),
                            *dc, region, tracksRect, clip,
                            mViewInfo->selectedRegion, *mViewInfo,
                            envelopeFlag, bigPointsFlag, sliderFlag);

   DrawEverythingElse(dc, region, clip);
}

/// Draws 'Everything else'.  In particular it draws:
///  - Drop shadow for tracks and vertical rulers.
///  - Zooming Indicators.
///  - Fills in space below the tracks.
void TrackPanel::DrawEverythingElse(wxDC * dc,
                                    const wxRegion &region,
                                    const wxRect & clip)
{
   // We draw everything else

   wxRect focusRect(-1, -1, 0, 0);
   wxRect trackRect = clip;
   trackRect.height = 0;   // for drawing background in no tracks case.

   VisibleTrackIterator iter(GetProject());
   for (Track *t = iter.First(); t; t = iter.Next()) {
      trackRect.y = t->GetY() - mViewInfo->vpos;
      trackRect.height = t->GetHeight();

      // If this track is linked to the next one, display a common
      // border for both, otherwise draw a normal border
      wxRect rect = trackRect;
      bool skipBorder = false;
      Track *l = t->GetLink();

      if (t->GetLinked()) {
         rect.height += l->GetHeight();
      }
      else if (l && trackRect.y >= 0) {
         skipBorder = true;
      }

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if(MONO_WAVE_PAN(t)){
         rect.height += t->GetHeight(true);
      }
#endif

      // If the previous track is linked to this one but isn't on the screen
      // (and thus would have been skipped by VisibleTrackIterator) we need to
      // draw that track's border instead.
      Track *borderTrack = t;
      wxRect borderRect = rect, borderTrackRect = trackRect;

      if (l && !t->GetLinked() && trackRect.y < 0)
      {
         borderTrack = l;

         borderTrackRect.y = l->GetY() - mViewInfo->vpos;
         borderTrackRect.height = l->GetHeight();

         borderRect = borderTrackRect;
         borderRect.height += t->GetHeight();
      }

      if (!skipBorder) {
         if (mAx->IsFocused(t)) {
            focusRect = borderRect;
         }
         DrawOutside(borderTrack, dc, borderRect, borderTrackRect);
      }

      // Believe it or not, we can speed up redrawing if we don't
      // redraw the vertical ruler when only the waveform data has
      // changed.  An example is during recording.

#if DEBUG_DRAW_TIMING
//      wxRect rbox = region.GetBox();
//      wxPrintf(wxT("Update Region: %d %d %d %d\n"),
//             rbox.x, rbox.y, rbox.width, rbox.height);
#endif

      if (region.Contains(0, trackRect.y, GetLeftOffset(), trackRect.height)) {
         wxRect rect = trackRect;
         rect.x += GetVRulerOffset();
         rect.y += kTopMargin;
         rect.width = GetVRulerWidth();
         rect.height -= (kTopMargin + kBottomMargin);
         mTrackArtist->DrawVRuler(t, dc, rect);
      }

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if(MONO_WAVE_PAN(t)){
         trackRect.y = t->GetY(true) - mViewInfo->vpos;
         trackRect.height = t->GetHeight(true);
         if (region.Contains(0, trackRect.y, GetLeftOffset(), trackRect.height)) {
            wxRect rect = trackRect;
            rect.x += GetVRulerOffset();
            rect.y += kTopMargin;
            rect.width = GetVRulerWidth();
            rect.height -= (kTopMargin + kBottomMargin);
            mTrackArtist->DrawVRuler(t, dc, rect);
         }
      }
#endif
   }

   if (mUIHandle)
      mUIHandle->DrawExtras(UIHandle::Cells, dc, region, clip);

   if (mMouseCapture == IsVZooming && IsDragZooming()
       // note track zooming now works like audio track
       //#ifdef USE_MIDI
       //       && mCapturedTrack && mCapturedTrack->GetKind() != Track::Note
       //#endif
       ) {
      DrawZooming(dc, clip);
   }

   // Paint over the part below the tracks
   trackRect.y += trackRect.height;
   if (trackRect.y < clip.GetBottom()) {
      AColor::TrackPanelBackground(dc, false);
      dc->DrawRectangle(trackRect.x,
                        trackRect.y,
                        trackRect.width,
                        clip.height - trackRect.y);
   }

   // Sometimes highlight is not drawn on backing bitmap. I thought
   // it was because FindFocus did not return "this" on Mac, but
   // when I removed that test, yielding this condition:
   //     if (GetFocusedTrack() != NULL) {
   // the highlight was reportedly drawn even when something else
   // was the focus and no highlight should be drawn. -RBD
   if (GetFocusedTrack() != NULL && wxWindow::FindFocus() == this) {
      HighlightFocusedTrack(dc, focusRect);
   }

   if (mUIHandle)
      mUIHandle->DrawExtras(UIHandle::Panel, dc, region, clip);

   // Draw snap guidelines if we have any
   if (mSnapManager && (mSnapLeft >= 0 || mSnapRight >= 0)) {
      AColor::SnapGuidePen(dc);
      if (mSnapLeft >= 0) {
         AColor::Line(*dc, (int)mSnapLeft, 0, mSnapLeft, 30000);
      }
      if (mSnapRight >= 0) {
         AColor::Line(*dc, (int)mSnapRight, 0, mSnapRight, 30000);
      }
   }
}

/// Draw zooming indicator that shows the region that will
/// be zoomed into when the user clicks and drags with a
/// zoom cursor.  Handles both vertical and horizontal
/// zooming.
void TrackPanel::DrawZooming(wxDC * dc, const wxRect & clip)
{
   wxRect rect;

   dc->SetBrush(*wxTRANSPARENT_BRUSH);
   dc->SetPen(*wxBLACK_DASHED_PEN);

   if (mMouseCapture==IsVZooming) {
      rect.y = std::min(mZoomStart, mZoomEnd);
      rect.height = 1 + abs(mZoomEnd - mZoomStart);

      rect.x = GetVRulerOffset();
      rect.SetRight(GetSize().x - kRightMargin); // extends into border rect
   }
   else {
      rect.x = std::min(mZoomStart, mZoomEnd);
      rect.width = 1 + abs(mZoomEnd - mZoomStart);

      rect.y = -1;
      rect.height = clip.height + 2;
   }

   dc->DrawRectangle(rect);
}


// Make this #include go away!
#include "tracks/ui/TrackControls.h"

void TrackPanel::DrawOutside(Track * t, wxDC * dc, const wxRect & rec,
                             const wxRect & trackRect)
{
   wxRect rect = rec;
   int labelw = GetLabelWidth();
   int vrul = GetVRulerOffset();

   DrawOutsideOfTrack(t, dc, rect);

   rect.x += kLeftInset;
   rect.y += kTopInset;
   rect.width -= kLeftInset * 2;
   rect.height -= kTopInset;

   mTrackInfo.SetTrackInfoFont(dc);
   dc->SetTextForeground(theTheme.Colour(clrTrackPanelText));

   bool bIsWave = (t->GetKind() == Track::Wave);
#ifdef USE_MIDI
   bool bIsNote = (t->GetKind() == Track::Note);
#endif
   // don't enable bHasMuteSolo for Note track because it will draw in the
   // wrong place.
   mTrackInfo.DrawBackground(dc, rect, t->GetSelected(), bIsWave, labelw, vrul);

   // Vaughan, 2010-08-24: No longer doing this.
   // Draw sync-lock tiles in ruler area.
   //if (t->IsSyncLockSelected()) {
   //   wxRect tileFill = rect;
   //   tileFill.x = GetVRulerOffset();
   //   tileFill.width = GetVRulerWidth();
   //   TrackArtist::DrawSyncLockTiles(dc, tileFill);
   //}

   DrawBordersAroundTrack(t, dc, rect, labelw, vrul);
   DrawShadow(t, dc, rect);

   rect.width = mTrackInfo.GetTrackInfoWidth();

   // Need to know which button, if any, to draw as pressed.
   const MouseCaptureEnum mouseCapture =
      mMouseCapture ? mMouseCapture
      // This public global variable is a hack for now, which should go away
      // when TrackPanelCell gets a virtual function into which we move this
      // drawing code.  It's enough work for 2.1.2 just to move all the click and
      // drag code out of TrackPanel.  -- PRL
      : MouseCaptureEnum(TrackControls::gCaptureState);
   const bool captured = (t == mCapturedTrack || t == mpClickedTrack);

   mTrackInfo.DrawCloseBox(dc, rect, (captured && mouseCapture == IsClosing));
   mTrackInfo.DrawTitleBar(dc, rect, t, (captured && mouseCapture == IsPopping));

   mTrackInfo.DrawMinimize(dc, rect, t, (captured && mouseCapture == IsMinimizing));

   // Draw the sync-lock indicator if this track is in a sync-lock selected group.
   if (t->IsSyncLockSelected())
   {
      wxRect syncLockIconRect;
      mTrackInfo.GetSyncLockIconRect(rect, syncLockIconRect);
      wxBitmap syncLockBitmap(theTheme.Image(bmpSyncLockIcon));
      // Icon is 12x12 and syncLockIconRect is 16x16.
      dc->DrawBitmap(syncLockBitmap,
                     syncLockIconRect.x + 3,
                     syncLockIconRect.y + 2,
                     true);
   }

   mTrackInfo.DrawBordersWithin( dc, rect, bIsWave );

   if (bIsWave) {
      mTrackInfo.DrawMuteSolo(dc, rect, t, (captured && mouseCapture == IsMuting), false, HasSoloButton());
      mTrackInfo.DrawMuteSolo(dc, rect, t, (captured && mouseCapture == IsSoloing), true, HasSoloButton());

      mTrackInfo.DrawSliders(dc, (WaveTrack *)t, rect);
      if (!t->GetMinimized()) {

         int offset = 8;
         if (rect.y + 22 + 12 < rec.y + rec.height - 19)
            dc->DrawText(TrackSubText(t),
                         trackRect.x + offset,
                         trackRect.y + 22);

         if (rect.y + 38 + 12 < rec.y + rec.height - 19)
            dc->DrawText(GetSampleFormatStr(((WaveTrack *) t)->GetSampleFormat()),
                         trackRect.x + offset,
                         trackRect.y + 38);
      }
   }

#ifdef USE_MIDI
   else if (bIsNote) {
      // Note tracks do not have text, e.g. "Mono, 44100Hz, 32-bit float", so
      // Mute & Solo button goes higher. To preserve existing AudioTrack code,
      // we move the buttons up by pretending track is higher (at lower y)
      rect.y -= 34;
      rect.height += 34;
      wxRect midiRect;
      mTrackInfo.GetTrackControlsRect(trackRect, midiRect);
      // Offset by height of Solo/Mute buttons:
      midiRect.y += 15;
      midiRect.height -= 21; // allow room for minimize button at bottom

#ifdef EXPERIMENTAL_MIDI_OUT
         // the offset 2 is just to leave a little space between channel buttons
         // and velocity slider (if any)
         int h = ((NoteTrack *) t)->DrawLabelControls(*dc, midiRect) + 2;

         // Draw some lines for MuteSolo buttons:
         if (rect.height > 84) {
            AColor::Line(*dc, rect.x+48 , rect.y+50, rect.x+48, rect.y + 66);
            // bevel below mute/solo
            AColor::Line(*dc, rect.x, rect.y + 66, mTrackInfo.GetTrackInfoWidth(), rect.y + 66);
         }
         mTrackInfo.DrawMuteSolo(dc, rect, t,
               (captured && mouseCapture == IsMuting), false, HasSoloButton());
         mTrackInfo.DrawMuteSolo(dc, rect, t,
               (captured && mouseCapture == IsSoloing), true, HasSoloButton());

         // place a volume control below channel buttons (this will
         // control an offset to midi velocity).
         // DrawVelocitySlider places slider assuming this is a Wave track
         // and using a large offset to leave room for other things,
         // so here we make a fake rectangle as if it is for a Wave
         // track, but it is offset to place the slider properly in
         // a Note track. This whole placement thing should be redesigned
         // to lay out different types of tracks and controls
         wxRect gr; // gr is gain rectangle where slider is drawn
         mTrackInfo.GetGainRect(rect, gr);
         rect.y = rect.y + h - gr.y; // ultimately want slider at rect.y + h
         rect.height = rect.height - h + gr.y;
         // save for mouse hit detect:
         ((NoteTrack *) t)->SetGainPlacementRect(rect);
         mTrackInfo.DrawVelocitySlider(dc, (NoteTrack *) t, rect);
#endif
   }
#endif // USE_MIDI
}

void TrackPanel::DrawOutsideOfTrack(Track * t, wxDC * dc, const wxRect & rect)
{
   // Fill in area outside of the track
   AColor::TrackPanelBackground(dc, false);
   wxRect side;

   // Area between panel border and left track border
   side = rect;
   side.width = kLeftInset;
   dc->DrawRectangle(side);

   // Area between panel border and top track border
   side = rect;
   side.height = kTopInset;
   dc->DrawRectangle(side);

   // Area between panel border and right track border
   side = rect;
   side.x += side.width - kTopInset;
   side.width = kTopInset;
   dc->DrawRectangle(side);

   // Area between tracks of stereo group
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if (t->GetLinked() || MONO_WAVE_PAN(t)) {
      side = rect;
      side.y += t->GetHeight() - 1;
      side.height = kTopInset + 1;
      dc->DrawRectangle(side);
   }
#else
   if (t->GetLinked()) {
      // Paint the channel separator over (what would be) the shadow of the top
      // channel, and the top inset of the bottom channel
      side = rect;
      side.y += t->GetHeight() - kShadowThickness;
      side.height = kTopInset + kShadowThickness;
      dc->DrawRectangle(side);
   }
#endif
}

void TrackPanel::AddOverlay(TrackPanelOverlay *pOverlay)
{
   mOverlays->push_back(pOverlay);
}

bool TrackPanel::RemoveOverlay(TrackPanelOverlay *pOverlay)
{
   const size_t oldSize = mOverlays->size();
   std::remove(mOverlays->begin(), mOverlays->end(), pOverlay);
   return oldSize != mOverlays->size();
}

void TrackPanel::ClearOverlays()
{
   mOverlays->clear();
}

void TrackPanel::SetBackgroundCell(TrackPanelCell *pCell)
{
   mpBackground = pCell;
}

void TrackPanel::DrawOverlays(bool repaint)
{
   size_t n_pairs = mOverlays->size();

   std::vector< std::pair<wxRect, bool> > pairs;
   pairs.reserve(n_pairs);

   // Find out the rectangles and outdatedness for each overlay
   typedef std::vector<TrackPanelOverlay*>::const_iterator iter;
   wxSize size(mBackingDC.GetSize());
   for (iter it = mOverlays->begin(), end = mOverlays->end(); it != end; ++it) {
      pairs.push_back((*it)->GetRectangle(size));
#if defined(__WXMAC__)
      // On OSX, if a HiDPI resolution is being used, a vertical line will actually take up
      // more than 1 pixel (even though it is drawn as 1), so we restore the surrounding
      // pixels as well.  (This is because the wxClientDC doesn't know about the scaling.)
      wxRect &rect = pairs.rbegin()->first;
      if (rect.width > 0)
         rect.Inflate(1, 0);
#endif
   }

   // See what requires redrawing.  If repainting, all.
   // If not, then whatever is outdated, and whatever will be damaged by
   // undrawing.
   // By redrawing only what needs it, we avoid flashing things like
   // the cursor that are drawn with invert.
   if (!repaint) {
      bool done;
      do {
         done = true;
         for (size_t ii = 0; ii < n_pairs; ++ii) {
            for (size_t jj = ii + 1; jj < n_pairs; ++jj) {
               if (pairs[ii].second != pairs[jj].second &&
                  pairs[ii].first.Intersects(pairs[jj].first)) {
                  done = false;
                  pairs[ii].second = pairs[jj].second = true;
               }
            }
         }
      } while (!done);
   }

   bool done = true;
   std::vector< std::pair<wxRect, bool> >::const_iterator it2 = pairs.begin();
   for (iter it = mOverlays->begin(), end = mOverlays->end(); it != end; ++it, ++it2) {
      if (repaint || it2->second) {
         done = false;
         wxClientDC dc(this);
         (*it)->Erase(dc, mBackingDC);
      }
   }

   if (!done) {
      it2 = pairs.begin();
      for (iter it = mOverlays->begin(), end = mOverlays->end(); it != end; ++it, ++it2) {
         if (repaint || it2->second) {
            wxClientDC dc(this);
            TrackPanelCellIterator begin(this, true);
            TrackPanelCellIterator end(this, false);
            (*it)->Draw(dc, begin, end);
         }
      }
   }
}

/// Draw a three-level highlight gradient around the focused track.
void TrackPanel::HighlightFocusedTrack(wxDC * dc, const wxRect & rect)
{
   wxRect theRect = rect;
   theRect.x += kLeftInset;
   theRect.y += kTopInset;
   theRect.width -= kLeftInset * 2;
   theRect.height -= kTopInset;

   dc->SetBrush(*wxTRANSPARENT_BRUSH);

   AColor::TrackFocusPen(dc, 0);
   dc->DrawRectangle(theRect.x - 1, theRect.y - 1, theRect.width + 2, theRect.height + 2);

   AColor::TrackFocusPen(dc, 1);
   dc->DrawRectangle(theRect.x - 2, theRect.y - 2, theRect.width + 4, theRect.height + 4);

   AColor::TrackFocusPen(dc, 2);
   dc->DrawRectangle(theRect.x - 3, theRect.y - 3, theRect.width + 6, theRect.height + 6);
}

void TrackPanel::UpdateVRulers()
{
   TrackListOfKindIterator iter(Track::Wave, mTracks);
   for (Track *t = iter.First(); t; t = iter.Next()) {
      UpdateTrackVRuler(t);
   }

   UpdateVRulerSize();
}

void TrackPanel::UpdateVRuler(Track *t)
{
   UpdateTrackVRuler(t);

   UpdateVRulerSize();
}

void TrackPanel::UpdateTrackVRuler(Track *t)
{
   wxASSERT(t);
   if (!t)
      return;

   wxRect rect(GetVRulerOffset(),
            kTopMargin,
            GetVRulerWidth(),
            t->GetHeight() - (kTopMargin + kBottomMargin));

   mTrackArtist->UpdateVRuler(t, rect);
   Track *l = t->GetLink();
   if (l)
   {
      rect.height = l->GetHeight() - (kTopMargin + kBottomMargin);
      mTrackArtist->UpdateVRuler(l, rect);
   }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   else if(MONO_WAVE_PAN(t)){
      rect.height = t->GetHeight(true) - (kTopMargin + kBottomMargin);
      mTrackArtist->UpdateVRuler(t, rect);
   }
#endif
}

void TrackPanel::UpdateVRulerSize()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   if (t) {
      wxSize s = t->vrulerSize;
      while (t) {
         s.IncTo(t->vrulerSize);
         t = iter.Next();
      }
      if (vrulerSize != s) {
         vrulerSize = s;
         mRuler->SetLeftOffset(GetLeftOffset());  // bevel on AdornedRuler
         mRuler->Refresh();
      }
   }
   Refresh(false);
}

/// The following method moves to the previous track
/// selecting and unselecting depending if you are on the start of a
/// block or not.

/// \todo Merge related methods, TrackPanel::OnPrevTrack and
/// TrackPanel::OnNextTrack.
void TrackPanel::OnPrevTrack( bool shift )
{
   TrackListIterator iter( mTracks );
   Track* t = GetFocusedTrack();
   if( t == NULL )   // if there isn't one, focus on last
   {
      t = iter.Last();
      SetFocusedTrack( t );
      EnsureVisible( t );
      MakeParentModifyState(false);
      return;
   }

   Track* p = NULL;
   bool tSelected = false;
   bool pSelected = false;
   if( shift )
   {
      p = mTracks->GetPrev( t, true ); // Get previous track
      if( p == NULL )   // On first track
      {
         // JKC: wxBell() is probably for accessibility, so a blind
         // user knows they were at the top track.
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            p = iter.Last();
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      tSelected = t->GetSelected();
      if (p)
         pSelected = p->GetSelected();
      if( tSelected && pSelected )
      {
         mTracks->Select( t, false );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
      if( tSelected && !pSelected )
      {
         mTracks->Select( p, true );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && pSelected )
      {
         mTracks->Select( p, false );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && !pSelected )
      {
         mTracks->Select( t, true );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
   }
   else
   {
      p = mTracks->GetPrev( t, true ); // Get next track
      if( p == NULL )   // On last track so stay there?
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            for( Track *d = iter.First(); d; d = iter.Next( true ) )
            {
               p = d;
            }
            SetFocusedTrack( p );   // Wrap to the first track
            EnsureVisible( p );
            MakeParentModifyState(false);
            return;
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      else
      {
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
   }
}

/// The following method moves to the next track,
/// selecting and unselecting depending if you are on the start of a
/// block or not.
void TrackPanel::OnNextTrack( bool shift )
{
   Track *t;
   Track *n;
   TrackListIterator iter( mTracks );
   bool tSelected,nSelected;

   t = GetFocusedTrack();   // Get currently focused track
   if( t == NULL )   // if there isn't one, focus on first
   {
      t = iter.First();
      SetFocusedTrack( t );
      EnsureVisible( t );
      MakeParentModifyState(false);
      return;
   }

   if( shift )
   {
      n = mTracks->GetNext( t, true ); // Get next track
      if( n == NULL )   // On last track so stay there
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            n = iter.First();
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      tSelected = t->GetSelected();
      nSelected = n->GetSelected();
      if( tSelected && nSelected )
      {
         mTracks->Select( t, false );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
      if( tSelected && !nSelected )
      {
         mTracks->Select( n, true );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && nSelected )
      {
         mTracks->Select( n, false );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && !nSelected )
      {
         mTracks->Select( t, true );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
   }
   else
   {
      n = mTracks->GetNext( t, true ); // Get next track
      if( n == NULL )   // On last track so stay there
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            n = iter.First();
            SetFocusedTrack( n );   // Wrap to the first track
            EnsureVisible( n );
            MakeParentModifyState(false);
            return;
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      else
      {
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
   }
}

void TrackPanel::OnFirstTrack()
{
   Track *t = GetFocusedTrack();
   if (!t)
      return;

   TrackListIterator iter(mTracks);
   Track *f = iter.First();
   if (t != f)
   {
      SetFocusedTrack(f);
      MakeParentModifyState(false);
   }
   EnsureVisible(f);
}

void TrackPanel::OnLastTrack()
{
   Track *t = GetFocusedTrack();
   if (!t)
      return;

   TrackListIterator iter(mTracks);
   Track *l = iter.Last();
   if (t != l)
   {
      SetFocusedTrack(l);
      MakeParentModifyState(false);
   }
   EnsureVisible(l);
}

void TrackPanel::OnToggle()
{
   Track *t;

   t = GetFocusedTrack();   // Get currently focused track
   if (!t)
      return;

   mTracks->Select( t, !t->GetSelected() );
   EnsureVisible( t );
   MakeParentModifyState(false);

   mAx->Updated();

   return;
}

// Make sure selection edge is in view
void TrackPanel::ScrollIntoView(double pos)
{
   int w;
   GetTracksUsableArea( &w, NULL );

   int pixel = mViewInfo->TimeToPosition(pos);
   if (pixel < 0 || pixel >= w)
   {
      mListener->TP_ScrollWindow
         (mViewInfo->OffsetTimeByPixels(pos, -(w / 2)));
      Refresh(false);
   }
}

void TrackPanel::ScrollIntoView(int x)
{
   ScrollIntoView(mViewInfo->PositionToTime(x, GetLeftOffset()));
}

void TrackPanel::OnTrackMenu(Track *t)
{
   if(!t) {
      t = GetFocusedTrack();
      if(!t)
         return;
   }

   {
      TrackPanelCell *const pCell = t->GetTrackControl();
      const wxRect rect(FindTrackRect(t, true));
      const UIHandle::Result refreshResult =
         pCell->DoContextMenu(UsableControlRect(rect), this, NULL);
      ProcessUIHandleResult(this, t, t, refreshResult);
      // TODO:  Hide following lines inside the above.
   }

   mPopupMenuTarget = t;

   Track *next = mTracks->GetNext(t);

   wxMenu *theMenu = NULL;

   if (t->GetKind() == Track::Wave) {
      theMenu = mWaveTrackMenu;
      const bool isMono = !t->GetLinked();
      const bool canMakeStereo =
         (next && isMono && !next->GetLinked() &&
          next->GetKind() == Track::Wave);

      if (isMono != mShowMono) {
         if (isMono) {
            HideStereoItems(theMenu);
            ShowMonoItems(theMenu, mChannelItemsInsertionPoint);
         }
         else {
            HideMonoItems(theMenu);
            ShowStereoItems(theMenu, mChannelItemsInsertionPoint);
         }
         mShowMono = isMono;
      }

      if (isMono) {
         theMenu->Enable(OnMergeStereoID, canMakeStereo);

         // We only need to set check marks. Clearing checks causes problems on Linux (bug 851)
         switch (t->GetChannel()) {
         case Track::LeftChannel:
            theMenu->Check(OnChannelLeftID, true);
            break;
         case Track::RightChannel:
            theMenu->Check(OnChannelRightID, true);
            break;
         default:
            theMenu->Check(OnChannelMonoID, true);
         }
      }

      WaveTrack *const track = (WaveTrack *)t;
      const int display = track->GetDisplay();
      theMenu->Check(
         (display == WaveTrack::Waveform)
         ? (track->GetWaveformSettings().isLinear() ? OnWaveformID : OnWaveformDBID)
         : OnSpectrumID,
         true
      );
      theMenu->Enable(OnSpectrogramSettingsID, display == WaveTrack::Spectrum);

      SetMenuCheck(*mRateMenu, IdOfRate((int) track->GetRate()));
      SetMenuCheck(*mFormatMenu, IdOfFormat(track->GetSampleFormat()));

      bool unsafe = IsUnsafe();
      for (int i = OnRate8ID; i <= OnFloatID; i++) {
         theMenu->Enable(i, !unsafe);
      }
   }

   if (theMenu) {
      //We need to find the location of the menu rectangle.
      wxRect rect = FindTrackRect(t,true);
      wxRect titleRect;
      mTrackInfo.GetTitleBarRect(rect,titleRect);

      PopupMenu(theMenu, titleRect.x + 1,
                  titleRect.y + titleRect.height + 1);
   }

   mPopupMenuTarget = NULL;

   SetCapturedTrack(NULL);

   Refresh(false);
}

void TrackPanel::OnVRulerMenu(Track *t, wxMouseEvent *pEvent)
{
   if (!t) {
      t = GetFocusedTrack();
      if (!t)
         return;
   }

   if (t->GetKind() != Track::Wave)
      return;

   WaveTrack *const wt = static_cast<WaveTrack*>(t);

   const int display = wt->GetDisplay();
   wxMenu *theMenu;
   if (display == WaveTrack::Waveform) {
      theMenu = mRulerWaveformMenu;
      const int id =
         OnFirstWaveformScaleID + int(wt->GetWaveformSettings().scaleType);
      theMenu->Check(id, true);
   }
   else {
      theMenu = mRulerSpectrumMenu;
      const int id =
         OnFirstSpectrumScaleID + int(wt->GetSpectrogramSettings().scaleType);
      theMenu->Check(id, true);
   }

   int x, y;
   if (pEvent)
      x = pEvent->m_x, y = pEvent->m_y;
   else {
      // If no event given, pop up the menu at the same height
      // as for the track control menu
      const wxRect rect = FindTrackRect(wt, true);
      wxRect titleRect;
      mTrackInfo.GetTitleBarRect(rect, titleRect);
      x = GetVRulerOffset(), y = titleRect.y + titleRect.height + 1;
   }

   // So that IsDragZooming() returns false, and if we zoom in, we do so
   // centered where the mouse is now:
   mZoomStart = mZoomEnd = pEvent->m_y;

   mPopupMenuTarget = wt;
   PopupMenu(theMenu, x, y);
   mPopupMenuTarget = NULL;
}


Track * TrackPanel::GetFirstSelectedTrack()
{

   TrackListIterator iter(mTracks);

   Track * t;
   for ( t = iter.First();t!=NULL;t=iter.Next())
      {
         //Find the first selected track
         if(t->GetSelected())
            {
               return t;
            }

      }
   //if nothing is selected, return the first track
   t = iter.First();

   if(t)
      return t;
   else
      return NULL;
}

void TrackPanel::EnsureVisible(Track * t)
{
   TrackListIterator iter(mTracks);
   Track *it = NULL;
   Track *nt = NULL;

   SetFocusedTrack(t);

   int trackTop = 0;
   int trackHeight =0;

   for (it = iter.First(); it; it = iter.Next()) {
      trackTop += trackHeight;
      trackHeight =  it->GetHeight();

      //find the second track if this is stereo
      if (it->GetLinked()) {
         nt = iter.Next();
         trackHeight += nt->GetHeight();
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      else if(MONO_WAVE_PAN(it)){
         trackHeight += it->GetHeight(true);
      }
#endif
      else {
         nt = it;
      }

      //We have found the track we want to ensure is visible.
      if ((it == t) || (nt == t)) {

         //Get the size of the trackpanel.
         int width, height;
         GetSize(&width, &height);

         if (trackTop < mViewInfo->vpos) {
            height = mViewInfo->vpos - trackTop + mViewInfo->scrollStep;
            height /= mViewInfo->scrollStep;
            mListener->TP_ScrollUpDown(-height);
         }
         else if (trackTop + trackHeight > mViewInfo->vpos + height) {
            height = (trackTop + trackHeight) - (mViewInfo->vpos + height);
            height = (height + mViewInfo->scrollStep + 1) / mViewInfo->scrollStep;
            mListener->TP_ScrollUpDown(height);
         }

         break;
      }
   }
   Refresh(false);
}

void TrackPanel::DrawBordersAroundTrack(Track * t, wxDC * dc,
                                        const wxRect & rect, const int vrul,
                                        const int labelw)
{
   // Border around track and label area
   dc->SetBrush(*wxTRANSPARENT_BRUSH);
   dc->SetPen(*wxBLACK_PEN);
   dc->DrawRectangle(rect.x, rect.y, rect.width - 1, rect.height - 1);

   AColor::Line(*dc, labelw, rect.y, labelw, rect.y + rect.height - 1);       // between vruler and TrackInfo

   // The lines at bottom of 1st track and top of second track of stereo group
   // Possibly replace with DrawRectangle to add left border.
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if (t->GetLinked() || MONO_WAVE_PAN(t)) {
      int h1 = rect.y + t->GetHeight() - kTopInset;
      AColor::Line(*dc, vrul, h1 - 2, rect.x + rect.width - 1, h1 - 2);
      AColor::Line(*dc, vrul, h1 + kTopInset, rect.x + rect.width - 1, h1 + kTopInset);
   }
#else
   if (t->GetLinked()) {
      // The given rect has had the top inset subtracted
      int h1 = rect.y + t->GetHeight() - kTopInset;
      // h1 is the top coordinate of the second tracks' rectangle
      // Draw (part of) the bottom border of the top channel and top border of the bottom
      AColor::Line(*dc, vrul, h1 - kBottomMargin, rect.x + rect.width - 1, h1 - kBottomMargin);
      AColor::Line(*dc, vrul, h1 + kTopInset, rect.x + rect.width - 1, h1 + kTopInset);
   }
#endif
}

void TrackPanel::DrawShadow(Track * /* t */ , wxDC * dc, const wxRect & rect)
{
   int right = rect.x + rect.width - 1;
   int bottom = rect.y + rect.height - 1;

   // shadow
   dc->SetPen(*wxBLACK_PEN);

   // bottom
   AColor::Line(*dc, rect.x, bottom, right, bottom);
   // right
   AColor::Line(*dc, right, rect.y, right, bottom);

   // background
   AColor::Dark(dc, false);

   // bottom
   AColor::Line(*dc, rect.x, bottom, rect.x + 1, bottom);
   // right
   AColor::Line(*dc, right, rect.y, right, rect.y + 1);
}

/// Returns the string to be displayed in the track label
/// indicating whether the track is mono, left, right, or
/// stereo and what sample rate it's using.
wxString TrackPanel::TrackSubText(Track * t)
{
   wxString s = wxString::Format(wxT("%dHz"),
                                 (int) (((WaveTrack *) t)->GetRate() +
                                        0.5));
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if (t->GetLinked() && t->GetChannel() != Track::MonoChannel)
      s = _("Stereo, ") + s;
#else
   if (t->GetLinked())
      s = _("Stereo, ") + s;
#endif
   else {
      if (t->GetChannel() == Track::MonoChannel)
         s = _("Mono, ") + s;
      else if (t->GetChannel() == Track::LeftChannel)
         s = _("Left, ") + s;
      else if (t->GetChannel() == Track::RightChannel)
         s = _("Right, ") + s;
   }

   return s;
}

/// Handle the menu options that change a track between
/// left channel, right channel, and mono.
static int channels[] = { Track::LeftChannel, Track::RightChannel,
   Track::MonoChannel
};

static const wxChar *channelmsgs[] = { _("Left Channel"), _("Right Channel"),
   _("Mono")
};

void TrackPanel::OnChannelChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnChannelLeftID && id <= OnChannelMonoID);
   wxASSERT(mPopupMenuTarget);
   mPopupMenuTarget->SetChannel(channels[id - OnChannelLeftID]);
   MakeParentPushState(wxString::Format(_("Changed '%s' to %s"),
                        mPopupMenuTarget->GetName().c_str(),
                        channelmsgs[id - OnChannelLeftID]),
                        _("Channel"));
   Refresh(false);
}

/// Swap the left and right channels of a stero track...
void TrackPanel::OnSwapChannels(wxCommandEvent & WXUNUSED(event))
{
   Track *partner = mPopupMenuTarget->GetLink();
   Track *const focused = GetFocusedTrack();
   const bool hasFocus =
      (focused == mPopupMenuTarget || focused == partner);

   SplitStereo(true);
   mPopupMenuTarget->SetChannel(Track::RightChannel);
   partner->SetChannel(Track::LeftChannel);

   (mTracks->MoveUp(partner));
   partner->SetLinked(true);

   MixerBoard* pMixerBoard = this->GetMixerBoard();
   if (pMixerBoard) {
      pMixerBoard->UpdateTrackClusters();
   }

   if (hasFocus)
      SetFocusedTrack(partner);

   MakeParentPushState(wxString::Format(_("Swapped Channels in '%s'"),
                                        mPopupMenuTarget->GetName().c_str()),
                       _("Swap Channels"));

}

/// Split a stereo track into two tracks...
void TrackPanel::OnSplitStereo(wxCommandEvent & WXUNUSED(event))
{
   SplitStereo(true);
   MakeParentPushState(wxString::Format(_("Split stereo track '%s'"),
                                        mPopupMenuTarget->GetName().c_str()),
                       _("Split"));
}

/// Split a stereo track into two mono tracks...
void TrackPanel::OnSplitStereoMono(wxCommandEvent & WXUNUSED(event))
{
   SplitStereo(false);
   MakeParentPushState(wxString::Format(_("Split Stereo to Mono '%s'"),
                                        mPopupMenuTarget->GetName().c_str()),
                       _("Split to Mono"));
}

/// Split a stereo track into two tracks...
void TrackPanel::SplitStereo(bool stereo)
{
   wxASSERT(mPopupMenuTarget);

   if (!stereo)
      mPopupMenuTarget->SetChannel(Track::MonoChannel);

   Track *partner = mPopupMenuTarget->GetLink();

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if(!stereo && MONO_WAVE_PAN(mPopupMenuTarget))
      ((WaveTrack*)mPopupMenuTarget)->SetVirtualState(true,true);
   if(!stereo && MONO_WAVE_PAN(partner))
      ((WaveTrack*)partner)->SetVirtualState(true,true);
#endif

   if (partner)
   {
      partner->SetName(mPopupMenuTarget->GetName());
      if (!stereo)
         partner->SetChannel(Track::MonoChannel);  // Keep original stereo track name.

      //On Demand - have each channel add it's own.
      if (ODManager::IsInstanceCreated() && partner->GetKind() == Track::Wave)
         ODManager::Instance()->MakeWaveTrackIndependent((WaveTrack*)partner);
   }

   mPopupMenuTarget->SetLinked(false);
   //make sure neither track is smaller than its minimum height
   if (mPopupMenuTarget->GetHeight() < mPopupMenuTarget->GetMinimizedHeight())
      mPopupMenuTarget->SetHeight(mPopupMenuTarget->GetMinimizedHeight());
   if (partner)
   {
      if (partner->GetHeight() < partner->GetMinimizedHeight())
         partner->SetHeight(partner->GetMinimizedHeight());

      // Make tracks the same height
      if (mPopupMenuTarget->GetHeight() != partner->GetHeight())
      {
         mPopupMenuTarget->SetHeight(((mPopupMenuTarget->GetHeight())+(partner->GetHeight())) / 2.0);
         partner->SetHeight(mPopupMenuTarget->GetHeight());
      }
   }

   Refresh(false);
}

/// Merge two tracks into one stereo track ??
void TrackPanel::OnMergeStereo(wxCommandEvent & WXUNUSED(event))
{
   wxASSERT(mPopupMenuTarget);
   mPopupMenuTarget->SetLinked(true);
   Track *partner = mPopupMenuTarget->GetLink();

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if(MONO_WAVE_PAN(mPopupMenuTarget))
      ((WaveTrack*)mPopupMenuTarget)->SetVirtualState(false);
   if(MONO_WAVE_PAN(partner))
      ((WaveTrack*)partner)->SetVirtualState(false);
#endif

   if (partner) {
      // Set partner's parameters to match target.
      partner->Merge(*mPopupMenuTarget);

      mPopupMenuTarget->SetChannel(Track::LeftChannel);
      partner->SetChannel(Track::RightChannel);

      // Set new track heights and minimized state
      bool bBothMinimizedp=((mPopupMenuTarget->GetMinimized())&&(partner->GetMinimized()));
      mPopupMenuTarget->SetMinimized(false);
      partner->SetMinimized(false);
      int AverageHeight=(mPopupMenuTarget->GetHeight() + partner->GetHeight())/ 2;
      mPopupMenuTarget->SetHeight(AverageHeight);
      partner->SetHeight(AverageHeight);
      mPopupMenuTarget->SetMinimized(bBothMinimizedp);
      partner->SetMinimized(bBothMinimizedp);

      //On Demand - join the queues together.
      if(ODManager::IsInstanceCreated() && partner->GetKind() == Track::Wave && mPopupMenuTarget->GetKind() == Track::Wave )
         if(!ODManager::Instance()->MakeWaveTrackDependent((WaveTrack*)partner,(WaveTrack*)mPopupMenuTarget))
         {
            ;
            //TODO: in the future, we will have to check the return value of MakeWaveTrackDependent -
            //if the tracks cannot merge, it returns false, and in that case we should not allow a merging.
            //for example it returns false when there are two different types of ODTasks on each track's queue.
            //we will need to display this to the user.
         }

      MakeParentPushState(wxString::Format(_("Made '%s' a stereo track"),
                                           mPopupMenuTarget->GetName().
                                           c_str()),
                          _("Make Stereo"));
   } else
      mPopupMenuTarget->SetLinked(false);

   Refresh(false);
}

class ViewSettingsDialog : public PrefsDialog
{
public:
   ViewSettingsDialog
      (wxWindow *parent, const wxString &title, PrefsDialog::Factories &factories,
       int page)
      : PrefsDialog(parent, title, factories)
      , mPage(page)
   {
   }

   virtual long GetPreferredPage()
   {
      return mPage;
   }

   virtual void SavePreferredPage()
   {
   }

private:
   const int mPage;
};

void TrackPanel::OnSpectrogramSettings(wxCommandEvent &)
{
   WaveTrack *const wt = static_cast<WaveTrack*>(mPopupMenuTarget);
   // WaveformPrefsFactory waveformFactory(wt);
   SpectrumPrefsFactory spectrumFactory(wt);

   PrefsDialog::Factories factories;
   // factories.push_back(&waveformFactory);
   factories.push_back(&spectrumFactory);
   const int page = (wt->GetDisplay() == WaveTrack::Spectrum)
      ? 1 : 0;

   wxString title(wt->GetName() + wxT(": "));
   ViewSettingsDialog dialog(this, title, factories, page);

   if (0 != dialog.ShowModal())
      // Redraw
      Refresh(false);
}

///  Set the Display mode based on the menu choice in the Track Menu.
///  Note that gModes MUST BE IN THE SAME ORDER AS THE MENU CHOICES!!
///  const wxChar *gModes[] = { wxT("waveform"), wxT("waveformDB"),
///  wxT("spectrum"), wxT("pitch") };
void TrackPanel::OnSetDisplay(wxCommandEvent & event)
{
   int idInt = event.GetId();
   wxASSERT(idInt >= OnWaveformID && idInt <= OnSpectrumID);
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   bool linear = false;
   WaveTrack::WaveTrackDisplay id;
   switch (idInt) {
   default:
   case OnWaveformID:
      linear = true, id = WaveTrack::Waveform; break;
   case OnWaveformDBID:
      id = WaveTrack::Waveform; break;
   case OnSpectrumID:
      id = WaveTrack::Spectrum; break;
   }
   WaveTrack *wt = (WaveTrack *) mPopupMenuTarget;
   const bool wrongType = wt->GetDisplay() != id;
   const bool wrongScale =
      (id == WaveTrack::Waveform &&
       wt->GetWaveformSettings().isLinear() != linear);
   if (wrongType || wrongScale) {
      wt->SetLastScaleType();
      wt->SetDisplay(WaveTrack::WaveTrackDisplay(id));
      if (wrongScale)
         wt->GetIndependentWaveformSettings().scaleType = linear
         ? WaveformSettings::stLinear
         : WaveformSettings::stLogarithmic;

      WaveTrack *l = static_cast<WaveTrack *>(wt->GetLink());
      if (l) {
         l->SetLastScaleType();
         l->SetDisplay(WaveTrack::WaveTrackDisplay(id));
         if (wrongScale)
            l->GetIndependentWaveformSettings().scaleType = linear
            ? WaveformSettings::stLinear
            : WaveformSettings::stLogarithmic;
   }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if (wt->GetDisplay() == WaveTrack::WaveformDisplay) {
         wt->SetVirtualState(false);
      }else if (id == WaveTrack::WaveformDisplay) {
         wt->SetVirtualState(true);
      }
#endif
      UpdateVRuler(wt);

      MakeParentModifyState(true);
      Refresh(false);
   }
}

/// Sets the sample rate for a track, and if it is linked to
/// another track, that one as well.
void TrackPanel::SetRate(Track * pTrack, double rate)
{
   ((WaveTrack *) pTrack)->SetRate(rate);
   Track *partner = mTracks->GetLink(pTrack);
   if (partner)
      ((WaveTrack *) partner)->SetRate(rate);
   // Separate conversion of "rate" enables changing the decimals without affecting i18n
   wxString rateString = wxString::Format(wxT("%.3f"), rate);
   MakeParentPushState(wxString::Format(_("Changed '%s' to %s Hz"),
                                        pTrack->GetName().c_str(), rateString.c_str()),
                       _("Rate Change"));
}

/// Handles the selection from the Format submenu of the
/// track menu.
void TrackPanel::OnFormatChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= On16BitID && id <= OnFloatID);
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   sampleFormat newFormat = int16Sample;

   switch (id) {
   case On16BitID:
      newFormat = int16Sample;
      break;
   case On24BitID:
      newFormat = int24Sample;
      break;
   case OnFloatID:
      newFormat = floatSample;
      break;
   default:
      // ERROR -- should not happen
      wxASSERT(false);
      break;
   }
   if (newFormat == ((WaveTrack*)mPopupMenuTarget)->GetSampleFormat())
      return; // Nothing to do.

   bool bResult = ((WaveTrack*)mPopupMenuTarget)->ConvertToSampleFormat(newFormat);
   wxASSERT(bResult); // TO DO: Actually handle this.
   Track *partner = mTracks->GetLink(mPopupMenuTarget);
   if (partner)
   {
      bResult = ((WaveTrack*)partner)->ConvertToSampleFormat(newFormat);
      wxASSERT(bResult); // TO DO: Actually handle this.
   }

   MakeParentPushState(wxString::Format(_("Changed '%s' to %s"),
                                        mPopupMenuTarget->GetName().
                                        c_str(),
                                        GetSampleFormatStr(newFormat)),
                       _("Format Change"));

   SetMenuCheck( *mFormatMenu, id );
   MakeParentRedrawScrollbars();
   Refresh(false);
}

/// Converts a format enumeration to a wxWidgets menu item Id.
int TrackPanel::IdOfFormat( int format )
{
   switch (format) {
   case int16Sample:
      return On16BitID;
   case int24Sample:
      return On24BitID;
   case floatSample:
      return OnFloatID;
   default:
      // ERROR -- should not happen
      wxASSERT( false );
      break;
   }
   return OnFloatID;// Compiler food.
}

/// Puts a check mark at a given position in a menu.
void TrackPanel::SetMenuCheck( wxMenu & menu, int newId )
{
   wxMenuItemList & list = menu.GetMenuItems();
   wxMenuItem * item;
   int id;

   for ( wxMenuItemList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext() )
   {
      item = node->GetData();
      id = item->GetId();
      // We only need to set check marks. Clearing checks causes problems on Linux (bug 851)
      if (id==newId)
         menu.Check( id, true );
   }
}

const int nRates=12;

///  gRates MUST CORRESPOND DIRECTLY TO THE RATES AS LISTED IN THE MENU!!
///  IN THE SAME ORDER!!
static int gRates[nRates] = { 8000, 11025, 16000, 22050, 44100, 48000, 88200, 96000,
                       176400, 192000, 352800, 384000 };

/// This method handles the selection from the Rate
/// submenu of the track menu, except for "Other" (/see OnRateOther).
void TrackPanel::OnRateChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnRate8ID && id <= OnRate384ID);
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   SetMenuCheck( *mRateMenu, id );
   SetRate(mPopupMenuTarget, gRates[id - OnRate8ID]);

   MakeParentRedrawScrollbars();

   Refresh(false);
}

/// Converts a sampling rate to a wxWidgets menu item id
int TrackPanel::IdOfRate( int rate )
{
   for(int i=0;i<nRates;i++) {
      if( gRates[i] == rate )
         return i+OnRate8ID;
   }
   return OnRateOtherID;
}

void TrackPanel::OnRateOther(wxCommandEvent &event)
{
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   int newRate;

   /// \todo Remove artificial constants!!
   /// \todo Make a real dialog box out of this!!
   while (true)
   {
      wxDialog dlg(this, wxID_ANY, wxString(_("Set Rate")));
      dlg.SetName(dlg.GetTitle());
      ShuttleGui S(&dlg, eIsCreating);
      wxString rate;
      wxArrayString rates;
      wxComboBox *cb;

      rate.Printf(wxT("%ld"), lrint(((WaveTrack *) mPopupMenuTarget)->GetRate()));

      rates.Add(wxT("8000"));
      rates.Add(wxT("11025"));
      rates.Add(wxT("16000"));
      rates.Add(wxT("22050"));
      rates.Add(wxT("44100"));
      rates.Add(wxT("48000"));
      rates.Add(wxT("88200"));
      rates.Add(wxT("96000"));
      rates.Add(wxT("176400"));
      rates.Add(wxT("192000"));
      rates.Add(wxT("352800"));
      rates.Add(wxT("384000"));

      S.StartVerticalLay(true);
      {
         S.SetBorder(10);
         S.StartHorizontalLay(wxEXPAND, false);
         {
            cb = S.AddCombo(_("New sample rate (Hz):"),
                            rate,
                            &rates);
#if defined(__WXMAC__)
            // As of wxMac-2.8.12, setting manually is required
            // to handle rates not in the list.  See: Bug #427
            cb->SetValue(rate);
#endif
         }
         S.EndHorizontalLay();
         S.AddStandardButtons();
      }
      S.EndVerticalLay();

      dlg.SetClientSize(dlg.GetSizer()->CalcMin());
      dlg.Center();

      if (dlg.ShowModal() != wxID_OK)
      {
         return;  // user cancelled dialog
      }

      long lrate;
      if (cb->GetValue().ToLong(&lrate) && lrate >= 1 && lrate <= 1000000)
      {
         newRate = (int)lrate;
         break;
      }

      wxMessageBox(_("The entered value is invalid"), _("Error"),
                   wxICON_ERROR, this);
   }

   SetMenuCheck( *mRateMenu, event.GetId() );
   SetRate(mPopupMenuTarget, newRate);

   MakeParentRedrawScrollbars();
   Refresh(false);
}

void TrackPanel::OnWaveformScaleType(wxCommandEvent &evt)
{
   WaveTrack *const wt = static_cast<WaveTrack *>(mPopupMenuTarget);
   WaveTrack *const partner = static_cast<WaveTrack*>(wt->GetLink());
   const WaveformSettings::ScaleType newScaleType =
      WaveformSettings::ScaleType(
         std::max(0,
            std::min(int(WaveformSettings::stNumScaleTypes) - 1,
               evt.GetId() - OnFirstWaveformScaleID
      )));
   if (wt->GetWaveformSettings().scaleType != newScaleType) {
      wt->GetIndependentWaveformSettings().scaleType = newScaleType;
      if (partner)
         partner->GetIndependentWaveformSettings().scaleType = newScaleType;
      UpdateVRuler(wt); // Is this really needed?
      MakeParentModifyState(true);
      Refresh(false);
   }
}

void TrackPanel::OnSpectrumScaleType(wxCommandEvent &evt)
{
   WaveTrack *const wt = static_cast<WaveTrack *>(mPopupMenuTarget);
   WaveTrack *const partner = static_cast<WaveTrack*>(wt->GetLink());
   const SpectrogramSettings::ScaleType newScaleType =
      SpectrogramSettings::ScaleType(
         std::max(0,
            std::min(int(SpectrogramSettings::stNumScaleTypes) - 1,
               evt.GetId() - OnFirstSpectrumScaleID
      )));
   if (wt->GetSpectrogramSettings().scaleType != newScaleType) {
      wt->GetIndependentSpectrogramSettings().scaleType = newScaleType;
      if (partner)
         partner->GetIndependentSpectrogramSettings().scaleType = newScaleType;
      UpdateVRuler(wt); // Is this really needed?
      MakeParentModifyState(true);
      Refresh(false);
   }
}

void TrackPanel::OnZoomInVertical(wxCommandEvent &)
{
   HandleWaveTrackVZoom(static_cast<WaveTrack*>(mPopupMenuTarget), false, false);
}

void TrackPanel::OnZoomOutVertical(wxCommandEvent &)
{
   HandleWaveTrackVZoom(static_cast<WaveTrack*>(mPopupMenuTarget), true, false);
}

void TrackPanel::OnZoomFitVertical(wxCommandEvent &)
{
   HandleWaveTrackVZoom(static_cast<WaveTrack*>(mPopupMenuTarget), true, true);
}

/// Determines which track is under the mouse
///  @param mouseX - mouse X position.
///  @param mouseY - mouse Y position.
///  @param label  - true iff the X Y position is relative to side-panel with the labels in it.
///  @param link - true iff we should consider a hit in any linked track as a hit.
///  @param *trackRect - returns track rectangle.
Track *TrackPanel::FindTrack(int mouseX, int mouseY, bool label, bool link,
                              wxRect * trackRect)
{
   // If label is true, resulting rectangle OMITS left and top insets.
   // If label is false, resulting rectangle INCLUDES right and top insets.

   wxRect rect;
   rect.x = 0;
   rect.y = -mViewInfo->vpos;
   rect.y += kTopInset;
   GetSize(&rect.width, &rect.height);

   if (label) {
      rect.width = GetLeftOffset();
   } else {
      rect.x = GetLeftOffset();
      rect.width -= GetLeftOffset();
   }

   VisibleTrackIterator iter(GetProject());
   for (Track * t = iter.First(); t; t = iter.Next()) {
      rect.y = t->GetY() - mViewInfo->vpos + kTopInset;
      rect.height = t->GetHeight();

      if (link && t->GetLink()) {
         Track *l = t->GetLink();
         int h = l->GetHeight();
         if (!t->GetLinked()) {
            t = l;
            rect.y = t->GetY() - mViewInfo->vpos + kTopInset;
         }
         rect.height += h;
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      else if(link && MONO_WAVE_PAN(t))
      {
         rect.height += t->GetHeight(true);
      }
#endif
      //Determine whether the mouse is inside
      //the current rectangle.  If so, recalculate
      //the proper dimensions and return.
      if (rect.Contains(mouseX, mouseY)) {
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
         t->SetVirtualStereo(false);
#endif
         if (trackRect) {
            rect.y -= kTopInset;
            if (label) {
               rect.x += kLeftInset;
               rect.width -= kLeftInset;
               rect.y += kTopInset;
               rect.height -= kTopInset;
            }
            *trackRect = rect;
         }

         return t;
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if(!link && MONO_WAVE_PAN(t)){
         rect.y = t->GetY(true) - mViewInfo->vpos + kTopInset;
         rect.height = t->GetHeight(true);
         if (rect.Contains(mouseX, mouseY)) {
            t->SetVirtualStereo(true);
            if (trackRect) {
               rect.y -= kTopInset;
               if (label) {
                  rect.x += kLeftInset;
                  rect.width -= kLeftInset;
                  rect.y += kTopInset;
                  rect.height -= kTopInset;
               }
               *trackRect = rect;
            }
            return t;
         }
      }
#endif // EXPERIMENTAL_OUTPUT_DISPLAY
   }

   return NULL;
}

/// This finds the rectangle of a given track, either the
/// of the label 'adornment' or the track itself
wxRect TrackPanel::FindTrackRect(Track * target, bool label)
{
   if (!target) {
      return wxRect(0,0,0,0);
   }

   wxRect rect(0,
            target->GetY() - mViewInfo->vpos,
            GetSize().GetWidth(),
            target->GetHeight());

   // The check for a null linked track is necessary because there's
   // a possible race condition between the time the 2 linked tracks
   // are added and when wxAccessible methods are called.  This is
   // most evident when using Jaws.
   if (target->GetLinked() && target->GetLink()) {
      rect.height += target->GetLink()->GetHeight();
   }

   if (label) {
      rect.x += kLeftInset;
      rect.width -= kLeftInset;
      rect.y += kTopInset;
      rect.height -= kTopInset;
   }

   return rect;
}

int TrackPanel::GetVRulerWidth() const
{
   return vrulerSize.x;
}

/// Displays the bounds of the selection in the status bar.
void TrackPanel::DisplaySelection()
{
   if (!mListener)
      return;

   // DM: Note that the Selection Bar can actually MODIFY the selection
   // if snap-to mode is on!!!
   mListener->TP_DisplaySelection();
}

Track *TrackPanel::GetFocusedTrack()
{
   return mAx->GetFocus();
}

void TrackPanel::SetFocusedTrack( Track *t )
{
   // Make sure we always have the first linked track of a stereo track
   if (t && !t->GetLinked() && t->GetLink())
      t = (WaveTrack*)t->GetLink();

   if (AudacityProject::GetKeyboardCaptureHandler()) {
      AudacityProject::ReleaseKeyboard(this);
   }

   if (t && t->GetKind() == Track::Label) {
      AudacityProject::CaptureKeyboard(this);
   }

   mAx->SetFocus( t );
   Refresh( false );
}

void TrackPanel::OnSetFocus(wxFocusEvent & WXUNUSED(event))
{
   SetFocusedTrack( GetFocusedTrack() );
   Refresh( false );
}

void TrackPanel::OnKillFocus(wxFocusEvent & WXUNUSED(event))
{
   if (AudacityProject::HasKeyboardCapture(this))
   {
      AudacityProject::ReleaseKeyboard(this);
   }
   Refresh( false);
}

/**********************************************************************

  TrackInfo code is destined to move out of this file.
  Code should become a lot cleaner when we have sizers.

**********************************************************************/

TrackInfo::TrackInfo(TrackPanel * pParentIn)
{
   pParent = pParentIn;

   wxRect rect(0, 0, 1000, 1000);
   wxRect sliderRect;

   GetGainRect(rect, sliderRect);

   /* i18n-hint: Title of the Gain slider, used to adjust the volume */
   mGain = new LWSlider(pParent, _("Gain"),
                        wxPoint(sliderRect.x, sliderRect.y),
                        wxSize(sliderRect.width, sliderRect.height),
                        DB_SLIDER);
   mGain->SetDefaultValue(1.0);

   GetPanRect(rect, sliderRect);

   /* i18n-hint: Title of the Pan slider, used to move the sound left or right */
   mPan = new LWSlider(pParent, _("Pan"),
                       wxPoint(sliderRect.x, sliderRect.y),
                       wxSize(sliderRect.width, sliderRect.height),
                       PAN_SLIDER);
   mPan->SetDefaultValue(0.0);

   int fontSize = 10;
   mFont.Create(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

   int allowableWidth = GetTrackInfoWidth() - 2; // 2 to allow for left/right borders
   int textWidth, textHeight;
   do {
      mFont.SetPointSize(fontSize);
      pParent->GetTextExtent(_("Stereo, 999999Hz"),
                             &textWidth,
                             &textHeight,
                             NULL,
                             NULL,
                             &mFont);
      fontSize--;
   } while (textWidth >= allowableWidth);
}

TrackInfo::~TrackInfo()
{
   delete mGain;
   delete mPan;
}

static const int kTrackInfoWidth = 100;
static const int kTrackInfoBtnSize = 16; // widely used dimension, usually height

int TrackInfo::GetTrackInfoWidth() const
{
   return kTrackInfoWidth;
}

void TrackInfo::GetCloseBoxRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x;
   dest.y = rect.y;
   dest.width = kTrackInfoBtnSize;
   dest.height = kTrackInfoBtnSize;
}

void TrackInfo::GetTitleBarRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + kTrackInfoBtnSize; // to right of CloseBoxRect
   dest.y = rect.y;
   dest.width = kTrackInfoWidth - rect.x - kTrackInfoBtnSize; // to right of CloseBoxRect
   dest.height = kTrackInfoBtnSize;
}

void TrackInfo::GetMuteSoloRect(const wxRect & rect, wxRect & dest, bool solo, bool bHasSoloButton)
{
   dest.x = rect.x ;
   dest.y = rect.y + 50;
   dest.width = 48;
   dest.height = kTrackInfoBtnSize;

   if( !bHasSoloButton )
   {
      dest.width +=48;
   }
   else if (solo)
   {
      dest.x += 48;
   }
}

void TrackInfo::GetGainRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + 7;
   dest.y = rect.y + 70;
   dest.width = 84;
   dest.height = 25;
}

void TrackInfo::GetPanRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + 7;
   dest.y = rect.y + 100;
   dest.width = 84;
   dest.height = 25;
}

void TrackInfo::GetMinimizeRect(const wxRect & rect, wxRect &dest)
{
   const int kBlankWidth = kTrackInfoBtnSize + 4;
   dest.x = rect.x + kBlankWidth;
   dest.y = rect.y + rect.height - 19;
   // Width is kTrackInfoWidth less space on left for track select and on right for sync-lock icon.
   dest.width = kTrackInfoWidth - (2 * kBlankWidth);
   dest.height = kTrackInfoBtnSize;
}

void TrackInfo::GetSyncLockIconRect(const wxRect & rect, wxRect &dest)
{
   dest.x = rect.x + kTrackInfoWidth - kTrackInfoBtnSize - 4; // to right of minimize button
   dest.y = rect.y + rect.height - 19;
   dest.width = kTrackInfoBtnSize;
   dest.height = kTrackInfoBtnSize;
}


/// \todo Probably should move to 'Utils.cpp'.
void TrackInfo::SetTrackInfoFont(wxDC * dc) const
{
   dc->SetFont(mFont);
}

void TrackInfo::DrawBordersWithin(wxDC* dc, const wxRect & rect, bool bHasMuteSolo) const
{
   AColor::Dark(dc, false); // same color as border of toolbars (ToolBar::OnPaint())

   // below close box and title bar
   AColor::Line(*dc, rect.x, rect.y + kTrackInfoBtnSize, kTrackInfoWidth, rect.y + kTrackInfoBtnSize);

   // between close box and title bar
   AColor::Line(*dc, rect.x + kTrackInfoBtnSize, rect.y, rect.x + kTrackInfoBtnSize, rect.y + kTrackInfoBtnSize);

   if( bHasMuteSolo && (rect.height > (66+18) ))
   {
      AColor::Line(*dc, rect.x, rect.y + 50, kTrackInfoWidth, rect.y + 50);   // above mute/solo
      AColor::Line(*dc, rect.x + 48 , rect.y + 50, rect.x + 48, rect.y + 66);    // between mute/solo
      AColor::Line(*dc, rect.x, rect.y + 66, kTrackInfoWidth, rect.y + 66);   // below mute/solo
   }

   // left of and above minimize button
   wxRect minimizeRect;
   this->GetMinimizeRect(rect, minimizeRect);
   AColor::Line(*dc, minimizeRect.x - 1, minimizeRect.y,
                  minimizeRect.x - 1, minimizeRect.y + minimizeRect.height);
   AColor::Line(*dc, minimizeRect.x, minimizeRect.y - 1,
                  minimizeRect.x + minimizeRect.width, minimizeRect.y - 1);
}

void TrackInfo::DrawBackground(wxDC * dc, const wxRect & rect, bool bSelected,
   bool WXUNUSED(bHasMuteSolo), const int labelw, const int WXUNUSED(vrul)) const
{
   // fill in label
   wxRect fill = rect;
   fill.width = labelw-4;
   AColor::MediumTrackInfo(dc, bSelected);
   dc->DrawRectangle(fill);

   // Vaughan, 2010-09-16: No more bevels around controls area. Now only around buttons.
   //if( bHasMuteSolo )
   //{
   //   fill=wxRect( rect.x+1, rect.y+17, vrul-6, 32);
   //   AColor::BevelTrackInfo( *dc, true, fill );
   //
   //   fill=wxRect( rect.x+1, rect.y+67, fill.width, rect.height-87);
   //   AColor::BevelTrackInfo( *dc, true, fill );
   //}
   //else
   //{
   //   fill=wxRect( rect.x+1, rect.y+17, vrul-6, rect.height-37);
   //   AColor::BevelTrackInfo( *dc, true, fill );
   //}
}

void TrackInfo::GetTrackControlsRect(const wxRect & rect, wxRect & dest)
{
   wxRect top;
   wxRect bot;

   GetTitleBarRect(rect, top);
   GetMinimizeRect(rect, bot);

   dest.x = rect.x;
   dest.width = kTrackInfoWidth - dest.x;
   dest.y = top.GetBottom() + 2; // BUG
   dest.height = bot.GetTop() - top.GetBottom() - 2;
}


void TrackInfo::DrawCloseBox(wxDC * dc, const wxRect & rect, bool down) const
{
   wxRect bev;
   GetCloseBoxRect(rect, bev);

#ifdef EXPERIMENTAL_THEMING
   wxPen pen( theTheme.Colour( clrTrackPanelText ));
   dc->SetPen( pen );
#else
   dc->SetPen(*wxBLACK_PEN);
#endif

   // Draw the "X"
   const int s = 6;

   int ls = bev.x + ((bev.width - s) / 2);
   int ts = bev.y + ((bev.height - s) / 2);
   int rs = ls + s;
   int bs = ts + s;

   AColor::Line(*dc, ls,     ts, rs,     bs);
   AColor::Line(*dc, ls + 1, ts, rs + 1, bs);
   AColor::Line(*dc, rs,     ts, ls,     bs);
   AColor::Line(*dc, rs + 1, ts, ls + 1, bs);

   bev.Inflate(-1, -1);
   AColor::BevelTrackInfo(*dc, !down, bev);
}

void TrackInfo::DrawTitleBar(wxDC * dc, const wxRect & rect, Track * t,
                              bool down) const
{
   wxRect bev;
   GetTitleBarRect(rect, bev);
   bev.Inflate(-1, -1);

   // Draw title text
   SetTrackInfoFont(dc);
   wxString titleStr = t->GetName();
   int allowableWidth = kTrackInfoWidth - 38 - kLeftInset;

   wxCoord textWidth, textHeight;
   dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   while (textWidth > allowableWidth) {
      titleStr = titleStr.Left(titleStr.Length() - 1);
      dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   }

   // wxGTK leaves little scraps (antialiasing?) of the
   // characters if they are repeatedly drawn.  This
   // happens when holding down mouse button and moving
   // in and out of the title bar.  So clear it first.
   AColor::MediumTrackInfo(dc, t->GetSelected());
   dc->DrawRectangle(bev);
   dc->DrawText(titleStr, bev.x + 2, bev.y + (bev.height - textHeight) / 2);

   // Pop-up triangle
#ifdef EXPERIMENTAL_THEMING
   wxColour c = theTheme.Colour( clrTrackPanelText );
#else
   wxColour c = *wxBLACK;
#endif

   dc->SetPen(c);
   dc->SetBrush(c);

   int s = 10; // Width of dropdown arrow...height is half of width
   AColor::Arrow(*dc,
                 bev.GetRight() - s - 3, // 3 to offset from right border
                 bev.y + ((bev.height - (s / 2)) / 2),
                 s);

   AColor::BevelTrackInfo(*dc, !down, bev);
}

/// Draw the Mute or the Solo button, depending on the value of solo.
void TrackInfo::DrawMuteSolo(wxDC * dc, const wxRect & rect, Track * t,
                              bool down, bool solo, bool bHasSoloButton) const
{
   wxRect bev;
   if( solo && !bHasSoloButton )
      return;
   GetMuteSoloRect(rect, bev, solo, bHasSoloButton);
   bev.Inflate(-1, -1);

   if (bev.y + bev.height >= rect.y + rect.height - 19)
      return; // don't draw mute and solo buttons, because they don't fit into track label

   AColor::MediumTrackInfo( dc, t->GetSelected());
   if( solo )
   {
      if( t->GetSolo() )
      {
         AColor::Solo(dc, t->GetSolo(), t->GetSelected());
      }
   }
   else
   {
      if( t->GetMute() )
      {
         AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
      }
   }
   //(solo) ? AColor::Solo(dc, t->GetSolo(), t->GetSelected()) :
   //    AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
   dc->SetPen( *wxTRANSPARENT_PEN );//No border!
   dc->DrawRectangle(bev);

   wxCoord textWidth, textHeight;
   wxString str = (solo) ?
      /* i18n-hint: This is on a button that will silence this track.*/
      _("Solo") :
      /* i18n-hint: This is on a button that will silence all the other tracks.*/
      _("Mute");

   SetTrackInfoFont(dc);
   dc->GetTextExtent(str, &textWidth, &textHeight);
   dc->DrawText(str, bev.x + (bev.width - textWidth) / 2, bev.y + (bev.height - textHeight) / 2);

   AColor::BevelTrackInfo(*dc, (solo?t->GetSolo():t->GetMute()) == down, bev);
}

// Draw the minimize button *and* the sync-lock track icon, if necessary.
void TrackInfo::DrawMinimize(wxDC * dc, const wxRect & rect, Track * t, bool down) const
{
   wxRect bev;
   GetMinimizeRect(rect, bev);

   // Clear background to get rid of previous arrow
   AColor::MediumTrackInfo(dc, t->GetSelected());
   dc->DrawRectangle(bev);

#ifdef EXPERIMENTAL_THEMING
   wxColour c = theTheme.Colour(clrTrackPanelText);
   dc->SetBrush(c);
   dc->SetPen(c);
#else
   AColor::Dark(dc, t->GetSelected());
#endif

   AColor::Arrow(*dc,
                 bev.x - 5 + bev.width / 2,
                 bev.y - 2 + bev.height / 2,
                 10,
                 t->GetMinimized());

   AColor::BevelTrackInfo(*dc, !down, bev);
}

#ifdef EXPERIMENTAL_MIDI_OUT
void TrackInfo::DrawVelocitySlider(wxDC *dc, NoteTrack *t, wxRect rect) const
{
    wxRect gainRect;
    int index = t->GetIndex();
    EnsureSufficientSliders(index);
    GetGainRect(rect, gainRect);
    if (gainRect.y + gainRect.height < rect.y + rect.height - 19) {
       mGains[index]->SetStyle(VEL_SLIDER);
       GainSlider(index)->Move(wxPoint(gainRect.x, gainRect.y));
       GainSlider(index)->Set(t->GetGain());
       GainSlider(index)->OnPaint(*dc, t->GetSelected());
    }
}
#endif

void TrackInfo::DrawSliders(wxDC *dc, WaveTrack *t, wxRect rect) const
{
   wxRect sliderRect;

   GetGainRect(rect, sliderRect);
   if (sliderRect.y + sliderRect.height < rect.y + rect.height - 19) {
      GainSlider(t)->OnPaint(*dc);
   }

   GetPanRect(rect, sliderRect);
   if (sliderRect.y + sliderRect.height < rect.y + rect.height - 19) {
      PanSlider(t)->OnPaint(*dc);
   }
}

LWSlider * TrackInfo::GainSlider(WaveTrack *t) const
{
   wxRect rect(kLeftInset, t->GetY() - pParent->GetViewInfo()->vpos + kTopInset, 1, t->GetHeight());
   wxRect sliderRect;
   GetGainRect(rect, sliderRect);

   mGain->Move(wxPoint(sliderRect.x, sliderRect.y));
   mGain->Set(t->GetGain());

   return mGain;
}

LWSlider * TrackInfo::PanSlider(WaveTrack *t) const
{
   wxRect rect(kLeftInset, t->GetY() - pParent->GetViewInfo()->vpos + kTopInset, 1, t->GetHeight());
   wxRect sliderRect;
   GetPanRect(rect, sliderRect);

   mPan->Move(wxPoint(sliderRect.x, sliderRect.y));
   mPan->Set(t->GetPan());

   return mPan;
}

TrackPanelCellIterator::TrackPanelCellIterator(TrackPanel *trackPanel, bool begin)
   : mPanel(trackPanel)
   , mIter(trackPanel->GetProject())
   , mpCell(begin ? mIter.First() : NULL)
{
}

TrackPanelCellIterator &TrackPanelCellIterator::operator++ ()
{
   // To do, iterate over the other cells that are not tracks
   mpCell = mIter.Next();
   return *this;
}

TrackPanelCellIterator TrackPanelCellIterator::operator++ (int)
{
   TrackPanelCellIterator copy(*this);
   ++ *this;
   return copy;
}

TrackPanelCellIterator::value_type TrackPanelCellIterator::operator* () const
{
   Track *const pTrack = dynamic_cast<Track*>(mpCell);
   if (!pTrack)
      // to do: handle cells that are not tracks
      return std::make_pair((Track*)NULL, wxRect());

   // Convert virtual coordinate to physical
   int width;
   mPanel->GetTracksUsableArea(&width, NULL);
   int y = pTrack->GetY() - mPanel->GetViewInfo()->vpos;
   return std::make_pair(
      mpCell,
      wxRect(
         mPanel->GetLeftOffset(),
         y + kTopMargin,
         width,
         pTrack->GetHeight() - (kTopMargin + kBottomMargin)
      )
   );
}

static TrackPanel * TrackPanelFactory(wxWindow * parent,
   wxWindowID id,
   const wxPoint & pos,
   const wxSize & size,
   TrackList * tracks,
   ViewInfo * viewInfo,
   TrackPanelListener * listener,
   AdornedRulerPanel * ruler)
{
   return new TrackPanel(
      parent,
      id,
      pos,
      size,
      tracks,
      viewInfo,
      listener,
      ruler);
}


// Declare the static factory function.
// We defined it in the class.
TrackPanel *(*TrackPanel::FactoryFunction)(
              wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              TrackList * tracks,
              ViewInfo * viewInfo,
              TrackPanelListener * listener,
              AdornedRulerPanel * ruler) = TrackPanelFactory;

TrackPanelCell::~TrackPanelCell()
{
}

unsigned TrackPanelCell::HandleWheelRotation
(const TrackPanelMouseEvent &, AudacityProject *)
{
   return RefreshCode::Cancelled;
}

unsigned TrackPanelCell::DoContextMenu
   (const wxRect &, wxWindow*, wxPoint *)
{
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::CaptureKey(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::KeyDown(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::KeyUp(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::Char(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}
