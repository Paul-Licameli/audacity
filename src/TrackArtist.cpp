/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackArtist.cpp

  Dominic Mazzoni


*******************************************************************//*!

\class TrackArtist
\brief   This class handles the actual rendering of WaveTracks (both
  waveforms and spectra), NoteTracks, LabelTracks and TimeTracks.

  It's actually a little harder than it looks, because for
  waveforms at least it needs to cache the samples that are
  currently on-screen.

<b>How Audacity Redisplay Works \n
 Roger Dannenberg</b> \n
Oct 2010 \n

In my opinion, the bitmap should contain only the waveform, note, and
label images along with gray selection highlights. The track info
(sliders, buttons, title, etc.), track selection highlight, cursor, and
indicator should be drawn in the normal way, and clipping regions should
be used to avoid excessive copying of bitmaps (say, when sliders move),
or excessive redrawing of track info widgets (say, when scrolling occurs).
This is a fairly tricky code change since it requires careful specification
of what and where redraw should take place when any state changes. One
surprising finding is that NoteTrack display is slow compared to WaveTrack
display. Each note takes some time to gather attributes and select colors,
and while audio draws two amplitudes per horizontal pixels, large MIDI
scores can have more notes than horizontal pixels. This can make slider
changes very sluggish, but this can also be a problem with many
audio tracks.

*//*******************************************************************/

#include "Audacity.h" // for USE_* macros and HAVE_ALLOCA_H
#include "TrackArtist.h"

#include "AllThemeResources.h"
#include "Theme.h"
#include "TrackPanelDrawingContext.h"

#include "prefs/GUISettings.h"

TrackArtist::TrackArtist( AudacityProject &project,
   TrackPanel &parent, const ZoomInfo &zoomInfo,
   const SelectedRegion &selectedRegion )
   : project{ project }
   , parent{ parent }
   , zoomInfo{ zoomInfo }
   , selectedRegion{ selectedRegion }
{
   mdBrange = ENV_DB_RANGE;
   mShowClipping = false;

   SetColours(0);

   UpdatePrefs();
}

TrackArtist::~TrackArtist()
{
}

TrackArtist * TrackArtist::Get( TrackPanelDrawingContext &context )
{
   return static_cast< TrackArtist* >( context.pUserData );
}

void TrackArtist::SetColours( int iColorIndex)
{
   theTheme.SetBrushColour( blankBrush,      clrBlank );
   theTheme.SetBrushColour( unselectedBrush, clrUnselected);
   theTheme.SetBrushColour( selectedBrush,   clrSelected);
   theTheme.SetBrushColour( sampleBrush,     clrSample);
   theTheme.SetBrushColour( selsampleBrush,  clrSelSample);
   theTheme.SetBrushColour( dragsampleBrush, clrDragSample);
   theTheme.SetBrushColour( blankSelectedBrush, clrBlankSelected);

   theTheme.SetPenColour(   blankPen,        clrBlank);
   theTheme.SetPenColour(   unselectedPen,   clrUnselected);
   theTheme.SetPenColour(   selectedPen,     clrSelected);
   theTheme.SetPenColour(   muteSamplePen,   clrMuteSample);
   theTheme.SetPenColour(   odProgressDonePen, clrProgressDone);
   theTheme.SetPenColour(   odProgressNotYetPen, clrProgressNotYet);
   theTheme.SetPenColour(   shadowPen,       clrShadow);
   theTheme.SetPenColour(   clippedPen,      clrClipped);
   theTheme.SetPenColour(   muteClippedPen,  clrMuteClipped);
   theTheme.SetPenColour(   blankSelectedPen,clrBlankSelected);

   theTheme.SetPenColour(   selsamplePen,    clrSelSample);
   theTheme.SetPenColour(   muteRmsPen,      clrMuteRms);

   switch( iColorIndex %4 )
   {
      default:
      case 0:
         theTheme.SetPenColour(   samplePen,       clrSample);
         theTheme.SetPenColour(   rmsPen,          clrRms);
         break;
      case 1: // RED
         samplePen.SetColour( wxColor( 160,10,10 ) );
         rmsPen.SetColour( wxColor( 230,80,80 ) );
         break;
      case 2: // GREEN
         samplePen.SetColour( wxColor( 35,110,35 ) );
         rmsPen.SetColour( wxColor( 75,200,75 ) );
         break;
      case 3: //BLACK
         samplePen.SetColour( wxColor( 0,0,0 ) );
         rmsPen.SetColour( wxColor( 100,100,100 ) );
         break;

   }
}

int TrackArtist::ShowClippingPrefsID()
{
   static int value = wxNewId();
   return value;
}

void TrackArtist::UpdateSelectedPrefs( int id )
{
   if( id == ShowClippingPrefsID())
      mShowClipping = gPrefs->Read(wxT("/GUI/ShowClipping"), mShowClipping);
}

void TrackArtist::UpdatePrefs()
{
   mdBrange = gPrefs->Read(ENV_DB_KEY, mdBrange);

   mbShowTrackNameInTrack =
      gPrefs->ReadBool(wxT("/GUI/ShowTrackNameInWaveform"), false);

   UpdateSelectedPrefs( ShowClippingPrefsID() );

   SetColours(0);
}

