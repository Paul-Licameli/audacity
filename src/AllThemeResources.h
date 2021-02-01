/**********************************************************************

  Audacity: A Digital Audio Editor

  AllThemeResources.h

  James Crook

  Audacity is free software.
  License: GPL v2 - see LICENSE.txt

********************************************************************//**

\file AllThemeResources.h

This file contains definitions of all images, cursors, colours, fonts
and grids used by Audacity.

This will be split up into separate include files to reduce the amount
of recompilation on a change.

Meantime, do NOT DELETE any of these declarations, even if they're
unused, as they're all offset by prior declarations.

To add an image, you give its size and name like so:

\code
   DEFINE_IMAGE( bmpPause, wxImage( 16, 16 ), L"Pause");
\endcode

If you do this and run the program the image will be black to start
with, but you can go into ThemePrefs and load it (load components)
from there.  Audacity will look for a file called "Pause.png".

 - Now save into ImageCache.
 - From here on you can get the image by loading ImageCache.
 - To burn it into the program defaults, use the
 'Output Sourcery' button.

\see \ref Themability in DOxygen documentation for more details.

*//*******************************************************************/

// Note: No '#ifndef/#define' pair on this header file.
// we want to include it multiple times in Theme.cpp.




#include "MacroMagic.h"
#define XPMS_RETIRED

   SET_THEME_FLAGS(  resFlagPaired  );
   DEFINE_IMAGE( bmpPause, wxImage( 16, 16 ), L"Pause");
   DEFINE_IMAGE( bmpPauseDisabled, wxImage( 16, 16 ), L"PauseDisabled");
   DEFINE_IMAGE( bmpPlay, wxImage( 16, 16 ), L"Play");
   DEFINE_IMAGE( bmpPlayDisabled, wxImage( 16, 16 ), L"PlayDisabled");
   DEFINE_IMAGE( bmpLoop, wxImage( 16, 16 ), L"Loop");
   DEFINE_IMAGE( bmpLoopDisabled, wxImage( 16, 16 ), L"LoopDisabled");
   DEFINE_IMAGE( bmpCutPreview, wxImage( 16, 16 ), L"CutPreview");
   DEFINE_IMAGE( bmpCutPreviewDisabled, wxImage( 16, 16 ), L"CutPreviewDisabled");
   DEFINE_IMAGE( bmpStop, wxImage( 16, 16 ), L"Stop");
   DEFINE_IMAGE( bmpStopDisabled, wxImage( 16, 16 ), L"StopDisabled");
   DEFINE_IMAGE( bmpRewind, wxImage( 16, 16 ), L"Rewind");
   DEFINE_IMAGE( bmpRewindDisabled, wxImage( 16, 16 ), L"RewindDisabled");
   DEFINE_IMAGE( bmpFFwd, wxImage( 16, 16 ), L"FFwd");
   DEFINE_IMAGE( bmpFFwdDisabled, wxImage( 16, 16 ), L"FFwdDisabled");
   DEFINE_IMAGE( bmpRecord, wxImage( 16, 16 ), L"Record");
   DEFINE_IMAGE( bmpRecordDisabled, wxImage( 16, 16 ), L"RecordDisabled");
   DEFINE_IMAGE( bmpRecordBeside, wxImage( 16, 16 ), L"RecordBeside");
   DEFINE_IMAGE( bmpRecordBesideDisabled, wxImage( 16, 16 ), L"RecordBesideDisabled");
   DEFINE_IMAGE( bmpRecordBelow, wxImage( 16, 16 ), L"RecordBelow");
   DEFINE_IMAGE( bmpRecordBelowDisabled, wxImage( 16, 16 ), L"RecordBelowDisabled");
   DEFINE_IMAGE( bmpScrub, wxImage( 18, 16 ), L"Scrub");
   DEFINE_IMAGE( bmpScrubDisabled, wxImage( 18, 16 ), L"ScrubDisabled");
   DEFINE_IMAGE( bmpSeek, wxImage( 26, 16 ), L"Seek");
   DEFINE_IMAGE( bmpSeekDisabled, wxImage( 26, 16 ), L"SeekDisabled");


   SET_THEME_FLAGS(  resFlagNewLine  );
   DEFINE_IMAGE( bmpIBeam, wxImage( 27, 27 ), L"IBeam");
   DEFINE_IMAGE( bmpZoom, wxImage( 27, 27 ), L"Zoom");
   DEFINE_IMAGE( bmpEnvelope, wxImage( 27, 27 ), L"Envelope");
   DEFINE_IMAGE( bmpTimeShift, wxImage( 27, 27 ), L"TimeShift");
   DEFINE_IMAGE( bmpDraw, wxImage( 27, 27 ), L"Draw");
   DEFINE_IMAGE( bmpMulti, wxImage( 27, 27 ), L"Multi");
   DEFINE_IMAGE( bmpMic, wxImage( 25, 25 ), L"Mic");
   DEFINE_IMAGE( bmpSpeaker, wxImage( 25, 25 ), L"Speaker");

   SET_THEME_FLAGS(  resFlagPaired  );
   DEFINE_IMAGE( bmpZoomFit, wxImage( 27, 27 ), L"ZoomFit");
   DEFINE_IMAGE( bmpZoomFitDisabled, wxImage( 27, 27 ), L"ZoomFitDisabled");
   DEFINE_IMAGE( bmpZoomIn, wxImage( 27, 27 ), L"ZoomIn");
   DEFINE_IMAGE( bmpZoomInDisabled, wxImage( 27, 27 ), L"ZoomInDisabled");
   DEFINE_IMAGE( bmpZoomOut, wxImage( 27, 27 ), L"ZoomOut");
   DEFINE_IMAGE( bmpZoomOutDisabled, wxImage( 27, 27 ), L"ZoomOutDisabled");
   DEFINE_IMAGE( bmpZoomSel, wxImage( 27, 27 ), L"ZoomSel");
   DEFINE_IMAGE( bmpZoomSelDisabled, wxImage( 27, 27 ), L"ZoomSelDisabled");
   DEFINE_IMAGE( bmpZoomToggle, wxImage( 27, 27 ), L"ZoomToggle");
   DEFINE_IMAGE( bmpZoomToggleDisabled, wxImage( 27, 27 ), L"ZoomToggleDisabled");
   DEFINE_IMAGE( bmpCut, wxImage( 26, 24 ), L"Cut");
   DEFINE_IMAGE( bmpCutDisabled, wxImage( 26, 24 ), L"CutDisabled");
   DEFINE_IMAGE( bmpCopy, wxImage( 26, 24 ), L"Copy");
   DEFINE_IMAGE( bmpCopyDisabled, wxImage( 26, 24 ), L"CopyDisabled");
   DEFINE_IMAGE( bmpPaste, wxImage( 26, 24 ), L"Paste");
   DEFINE_IMAGE( bmpPasteDisabled, wxImage( 26, 24 ), L"PasteDisabled");
   DEFINE_IMAGE( bmpTrim, wxImage( 26, 24 ), L"Trim");
   DEFINE_IMAGE( bmpTrimDisabled, wxImage( 26, 24 ), L"TrimDisabled");
   DEFINE_IMAGE( bmpSilence, wxImage( 26, 24 ), L"Silence");
   DEFINE_IMAGE( bmpSilenceDisabled, wxImage( 26, 24 ), L"SilenceDisabled");
   DEFINE_IMAGE( bmpUndo, wxImage( 26, 24 ), L"Undo");
   DEFINE_IMAGE( bmpUndoDisabled, wxImage( 26, 24 ), L"UndoDisabled");
   DEFINE_IMAGE( bmpRedo, wxImage( 26, 24 ), L"Redo");
   DEFINE_IMAGE( bmpRedoDisabled, wxImage( 26, 24 ), L"RedoDisabled");

   SET_THEME_FLAGS(  resFlagPaired | resFlagNewLine  );
   DEFINE_IMAGE( bmpTnStartOn, wxImage( 27, 27 ), L"TnStartOn");
   DEFINE_IMAGE( bmpTnStartOnDisabled, wxImage( 27, 27 ), L"TnStartOnDisabled");
   DEFINE_IMAGE( bmpTnStartOff, wxImage( 27, 27 ), L"TnStartOff");
   DEFINE_IMAGE( bmpTnStartOffDisabled, wxImage( 27, 27 ), L"TnStartOffDisabled");
   DEFINE_IMAGE( bmpTnEndOn, wxImage( 27, 27 ), L"TnEndOn");
   DEFINE_IMAGE( bmpTnEndOnDisabled, wxImage( 27, 27 ), L"TnEndOnDisabled");
   DEFINE_IMAGE( bmpTnEndOff, wxImage( 27, 27 ), L"TnEndOff");
   DEFINE_IMAGE( bmpTnEndOffDisabled, wxImage( 27, 27 ), L"TnEndOffDisabled");
   DEFINE_IMAGE( bmpTnCalibrate, wxImage( 27, 27 ), L"TnCalibrate");
   DEFINE_IMAGE( bmpTnCalibrateDisabled, wxImage( 27, 27 ), L"TnCalibrateDisabled");
   DEFINE_IMAGE( bmpTnAutomateSelection, wxImage( 27, 27 ), L"TnAutomateSelection");
   DEFINE_IMAGE( bmpTnAutomateSelectionDisabled, wxImage( 27, 27 ), L"TnAutomateSelectionDisabled");
   DEFINE_IMAGE( bmpTnMakeTag, wxImage( 27, 27 ), L"TnMakeTag");
   DEFINE_IMAGE( bmpTnMakeTagDisabled, wxImage( 27, 27 ), L"TnMakeTagDisabled");
   DEFINE_IMAGE( bmpTnSelectSound, wxImage( 24, 24 ), L"TnSelectSound");
   DEFINE_IMAGE( bmpTnSelectSoundDisabled, wxImage( 24, 24 ), L"TnSelectSoundDisabled");
   DEFINE_IMAGE( bmpTnSelectSilence, wxImage( 24, 24 ), L"TnSelectSilence");
   DEFINE_IMAGE( bmpTnSelectSilenceDisabled, wxImage( 24, 24 ), L"TnSelectSilenceDisabled");
   DEFINE_IMAGE( bmpOptions, wxImage( 24, 24 ), L"Options");
   DEFINE_IMAGE( bmpOptionsDisabled, wxImage( 24, 24 ), L"OptionsDisabled");

   SET_THEME_FLAGS(  resFlagNone  );
   DEFINE_IMAGE( bmpLabelGlyph0, wxImage( 15, 23 ), L"LabelGlyph0");
   DEFINE_IMAGE( bmpLabelGlyph1, wxImage( 15, 23 ), L"LabelGlyph1");
   DEFINE_IMAGE( bmpLabelGlyph2, wxImage( 15, 23 ), L"LabelGlyph2");
   DEFINE_IMAGE( bmpLabelGlyph3, wxImage( 15, 23 ), L"LabelGlyph3");
   DEFINE_IMAGE( bmpLabelGlyph4, wxImage( 15, 23 ), L"LabelGlyph4");
   DEFINE_IMAGE( bmpLabelGlyph5, wxImage( 15, 23 ), L"LabelGlyph5");
   DEFINE_IMAGE( bmpLabelGlyph6, wxImage( 15, 23 ), L"LabelGlyph6");
   DEFINE_IMAGE( bmpLabelGlyph7, wxImage( 15, 23 ), L"LabelGlyph7");
   DEFINE_IMAGE( bmpLabelGlyph8, wxImage( 15, 23 ), L"LabelGlyph8");
   DEFINE_IMAGE( bmpLabelGlyph9, wxImage( 15, 23 ), L"LabelGlyph9");
   DEFINE_IMAGE( bmpLabelGlyph10, wxImage( 15, 23 ), L"LabelGlyph10");
   DEFINE_IMAGE( bmpLabelGlyph11, wxImage( 15, 23 ), L"LabelGlyph11");

   SET_THEME_FLAGS(  resFlagNewLine  );
   DEFINE_IMAGE( bmpSyncLockSelTile, wxImage(20, 22), L"SyncLockSelTile");
   DEFINE_IMAGE( bmpSyncLockTracksDown, wxImage( 20, 20 ), L"SyncLockTracksDown");
   DEFINE_IMAGE( bmpSyncLockTracksUp, wxImage( 20, 20 ), L"SyncLockTracksUp");
   DEFINE_IMAGE( bmpSyncLockTracksDisabled, wxImage( 20, 20 ), L"SyncLockTracksDisabled");
   DEFINE_IMAGE( bmpSyncLockIcon, wxImage(12, 12), L"SyncLockIcon");
   DEFINE_IMAGE( bmpEditEffects, wxImage(21, 20), L"EditEffects");
   DEFINE_IMAGE( bmpToggleScrubRuler, wxImage( 20, 20 ), L"ToggleScrubRuler");
   DEFINE_IMAGE( bmpHelpIcon, wxImage( 21, 21 ), L"HelpIcon");

   SET_THEME_FLAGS(  resFlagNone  );
   DEFINE_IMAGE( bmpPlayPointer, wxImage( 20, 20 ), L"PlayPointer");
   DEFINE_IMAGE( bmpPlayPointerPinned, wxImage( 20, 20 ), L"PlayPointerPinned");
   DEFINE_IMAGE( bmpRecordPointer, wxImage( 20, 20 ), L"RecordPointer");
   DEFINE_IMAGE( bmpRecordPointerPinned, wxImage( 20, 20 ), L"RecordPointerPinned");
   DEFINE_IMAGE( bmpGrabberDropLoc, wxImage( 20, 20 ), L"GrabberDropLoc");
   DEFINE_IMAGE( bmpSliderThumb, wxImage( 20, 20 ), L"SliderThumb");
   DEFINE_IMAGE( bmpSliderThumbHilited, wxImage( 20, 20 ), L"SliderThumbHilited");
   DEFINE_IMAGE( bmpSliderThumbRotated, wxImage( 20, 20 ), L"SliderThumbRotated");
   DEFINE_IMAGE( bmpSliderThumbRotatedHilited, wxImage( 20, 20 ), L"SliderThumbRotatedHilited");

   SET_THEME_FLAGS(  resFlagNewLine  );
   DEFINE_IMAGE( bmpUpButtonExpand, wxImage( 96, 18 ), L"UpButtonExpand");
   DEFINE_IMAGE( bmpDownButtonExpand, wxImage( 96, 18 ), L"DownButtonExpand");
   DEFINE_IMAGE( bmpHiliteUpButtonExpand, wxImage( 96, 18 ), L"HiliteUpButtonExpand");
   DEFINE_IMAGE( bmpHiliteButtonExpand, wxImage( 96, 18 ), L"HiliteButtonExpand");

   SET_THEME_FLAGS(  resFlagNewLine  );
   DEFINE_IMAGE( bmpUpButtonExpandSel, wxImage( 96, 18 ), L"UpButtonExpandSel");
   DEFINE_IMAGE( bmpDownButtonExpandSel, wxImage( 96, 18 ), L"DownButtonExpandSel");
   DEFINE_IMAGE( bmpHiliteUpButtonExpandSel, wxImage( 96, 18 ), L"HiliteUpButtonExpandSel");
   DEFINE_IMAGE( bmpHiliteButtonExpandSel, wxImage( 96, 18 ), L"HiliteButtonExpandSel");

   SET_THEME_FLAGS(  resFlagNone  );
   DEFINE_IMAGE( bmpUpButtonLarge, wxImage( 48, 48 ), L"UpButtonLarge");
   DEFINE_IMAGE( bmpDownButtonLarge, wxImage( 48, 48 ), L"DownButtonLarge");
   DEFINE_IMAGE( bmpHiliteUpButtonLarge, wxImage( 48, 48 ), L"HiliteUpButtonLarge");
   DEFINE_IMAGE( bmpHiliteButtonLarge, wxImage( 48, 48 ), L"HiliteButtonLarge");

   SET_THEME_FLAGS(  resFlagNewLine  );
   DEFINE_IMAGE( bmpMacUpButton, wxImage( 36, 36 ), L"MacUpButton");
   DEFINE_IMAGE( bmpMacDownButton, wxImage( 36, 36 ), L"MacDownButton");
   DEFINE_IMAGE( bmpMacHiliteUpButton, wxImage( 36, 36 ), L"MacHiliteUpButton");
   DEFINE_IMAGE( bmpMacHiliteButton, wxImage( 36, 36 ), L"MacHiliteButton");

   SET_THEME_FLAGS(  resFlagNone  );
   DEFINE_IMAGE( bmpUpButtonSmall, wxImage( 27, 27 ), L"UpButtonSmall");
   DEFINE_IMAGE( bmpDownButtonSmall, wxImage( 27, 27 ), L"DownButtonSmall");
   DEFINE_IMAGE( bmpHiliteUpButtonSmall, wxImage( 27, 27 ), L"HiliteUpButtonSmall");
   DEFINE_IMAGE( bmpHiliteButtonSmall, wxImage( 27, 27 ), L"HiliteButtonSmall");

   SET_THEME_FLAGS(  resFlagNewLine  );
   DEFINE_IMAGE( bmpMacUpButtonSmall, wxImage( 27, 27 ), L"MacUpButtonSmall");
   DEFINE_IMAGE( bmpMacDownButtonSmall, wxImage( 27, 27 ), L"MacDownButtonSmall");
   DEFINE_IMAGE( bmpMacHiliteUpButtonSmall, wxImage( 27, 27 ), L"MacHiliteUpButtonSmall");
   DEFINE_IMAGE( bmpMacHiliteButtonSmall, wxImage( 27, 27 ), L"MacHiliteButtonSmall");

   SET_THEME_FLAGS(  resFlagInternal  );
   DEFINE_IMAGE( bmpRecoloredUpLarge, wxImage( 48, 48 ), L"RecoloredUpLarge");
   DEFINE_IMAGE( bmpRecoloredDownLarge, wxImage( 48, 48 ), L"RecoloredDownLarge");
   DEFINE_IMAGE( bmpRecoloredUpHiliteLarge, wxImage( 48, 48 ), L"RecoloredUpHiliteLarge");
   DEFINE_IMAGE( bmpRecoloredHiliteLarge, wxImage( 48, 48 ), L"RecoloredHiliteLarge");
   DEFINE_IMAGE( bmpRecoloredUpSmall, wxImage( 27, 27 ), L"RecoloredUpSmall");
   DEFINE_IMAGE( bmpRecoloredDownSmall, wxImage( 27, 27 ), L"RecoloredDownSmall");
   DEFINE_IMAGE( bmpRecoloredUpHiliteSmall, wxImage( 27, 27 ), L"RecoloredUpHiliteSmall");
   DEFINE_IMAGE( bmpRecoloredHiliteSmall, wxImage( 27, 27 ), L"RecoloredHiliteSmall");

   SET_THEME_FLAGS(  resFlagCursor  );
   DEFINE_IMAGE( bmpIBeamCursor, wxImage( 32, 32 ), L"IBeamCursor");
   DEFINE_IMAGE( bmpDrawCursor, wxImage( 32, 32 ), L"DrawCursor");
   DEFINE_IMAGE( bmpEnvCursor, wxImage( 32, 32 ), L"EnvCursor");
   DEFINE_IMAGE( bmpTimeCursor, wxImage( 32, 32 ), L"TimeCursor");
   DEFINE_IMAGE( bmpZoomInCursor, wxImage( 32, 32 ), L"ZoomInCursor");
   DEFINE_IMAGE( bmpZoomOutCursor, wxImage( 32, 32 ), L"ZoomOutCursor");
   DEFINE_IMAGE( bmpLabelCursorLeft, wxImage( 32, 32 ), L"LabelCursorLeft");
   DEFINE_IMAGE( bmpLabelCursorRight, wxImage( 32, 32 ), L"LabelCursorRight");
   DEFINE_IMAGE( bmpDisabledCursor, wxImage( 32, 32 ), L"DisabledCursor");
   DEFINE_IMAGE( bmpBottomFrequencyCursor, wxImage( 32, 32 ), L"BottomFrequencyCursor");
   DEFINE_IMAGE( bmpTopFrequencyCursor, wxImage( 32, 32 ), L"TopFrequencyCursor");
   DEFINE_IMAGE( bmpBandWidthCursor, wxImage(32, 32), L"BandWidthCursor");
   DEFINE_IMAGE( bmpSubViewsCursor, wxImage(32, 32), L"SubViewsCursor");

   //SET_THEME_FLAGS(  resFlagNewLine  );

// DA: The logo with name xpm has a different width.
#ifdef EXPERIMENTAL_DA
#define LOGOWITHNAME_WIDTH 629
#else
#define LOGOWITHNAME_WIDTH 506
#endif

#define LOGOWITHNAME_HEIGHT 200

   SET_THEME_FLAGS( resFlagNewLine );
   DEFINE_IMAGE( bmpAudacityLogo48x48, wxImage( 48, 48 ), L"AudacityLogo48x48");


   DEFINE_COLOUR( clrBlank,      wxColour( 64,  64,  64), L"Blank");
   DEFINE_COLOUR( clrUnselected, wxColour( 30,  30,  30), L"Unselected");
   DEFINE_COLOUR( clrSelected,   wxColour( 93,  65,  93), L"Selected");
   DEFINE_COLOUR( clrSample,     wxColour( 63,  77, 155), L"Sample");
   DEFINE_COLOUR( clrSelSample,  wxColour( 50,  50, 200), L"SelSample");
   DEFINE_COLOUR( clrDragSample, wxColour(  0, 100,   0), L"DragSample");

   DEFINE_COLOUR( clrMuteSample, wxColour(136, 136, 144), L"MuteSample");
   DEFINE_COLOUR( clrRms,        wxColour(107, 154, 247), L"Rms");
   DEFINE_COLOUR( clrMuteRms,    wxColour(136, 136, 144), L"MuteRms");
   DEFINE_COLOUR( clrShadow,     wxColour(148, 148, 148), L"Shadow");

   DEFINE_COLOUR( clrAboutBoxBackground,  wxColour(255, 255, 255),  L"AboutBackground");
   DEFINE_COLOUR( clrTrackPanelText,      wxColour(200, 200, 200),  L"TrackPanelText");
   DEFINE_COLOUR( clrLabelTrackText,      wxColour(  0,   0,   0),  L"LabelTrackText");

   DEFINE_COLOUR( clrMeterPeak,            wxColour(102, 102, 255),  L"MeterPeak");
   DEFINE_COLOUR( clrMeterDisabledPen,     wxColour(192, 192, 192),  L"MeterDisabledPen");
   DEFINE_COLOUR( clrMeterDisabledBrush,   wxColour(160, 160, 160),  L"MeterDisabledBrush");

   DEFINE_COLOUR( clrMeterInputPen,        wxColour(204, 70, 70),     L"MeterInputPen" );
   DEFINE_COLOUR( clrMeterInputBrush,      wxColour(204, 70, 70),     L"MeterInputBrush" );
   DEFINE_COLOUR( clrMeterInputRMSBrush,   wxColour(255, 102, 102),   L"MeterInputRMSBrush" );
   DEFINE_COLOUR( clrMeterInputClipBrush,  wxColour(255, 53, 53),     L"MeterInputClipBrush" );
   DEFINE_COLOUR( clrMeterInputLightPen,   wxColour(255, 153, 153),   L"MeterInputLightPen" );
   DEFINE_COLOUR( clrMeterInputDarkPen,    wxColour(153, 61, 61),     L"MeterInputDarkPen" );

   DEFINE_COLOUR( clrMeterOutputPen,       wxColour(70, 204, 70),     L"MeterOutputPen" );
   DEFINE_COLOUR( clrMeterOutputBrush,     wxColour(70, 204, 70),     L"MeterOutputBrush" );
   DEFINE_COLOUR( clrMeterOutputRMSBrush,  wxColour(102, 255, 102),   L"MeterOutputRMSBrush" );
   DEFINE_COLOUR( clrMeterOutputClipBrush, wxColour(255, 53, 53),     L"MeterOutputClipBrush" );
   DEFINE_COLOUR( clrMeterOutputLightPen,  wxColour(153, 255, 153),   L"MeterOutputLightPen" );
   DEFINE_COLOUR( clrMeterOutputDarkPen,   wxColour(61, 164, 61),     L"MeterOutputDarkPen" );
   DEFINE_COLOUR( clrRulerBackground,      wxColour( 93,  65,  93),   L"RulerBackground" );
   DEFINE_COLOUR( clrAxisLines,            wxColour(0, 0, 255),       L"AxisLines" );
   DEFINE_COLOUR( clrGraphLines,           wxColour(110, 110, 220),   L"GraphLines" );
   DEFINE_COLOUR( clrResponseLines,        wxColour(0, 255, 0),       L"ResponseLines" );
   DEFINE_COLOUR( clrHzPlot,               wxColour(140, 60, 190),    L"HzPlot" );
   DEFINE_COLOUR( clrWavelengthPlot,       wxColour(200, 50, 150),    L"WavelengthPlot" );

   DEFINE_COLOUR( clrEnvelope,             wxColour( 110, 110, 220),  L"Envelope" );

   DEFINE_COLOUR( clrMuteButtonActive,     wxColour( 160, 170, 210),  L"MuteButtonActive" );
   DEFINE_COLOUR( clrMuteButtonVetoed,     wxColour( 180, 180, 185),  L"MuteButtonVetoed" );

   DEFINE_COLOUR( clrCursorPen,            wxColour(     0,  0,  0),  L"CursorPen" );
   DEFINE_COLOUR( clrRecordingPen,         wxColour(   176,  0, 28),  L"RecordingPen" );
   DEFINE_COLOUR( clrPlaybackPen,          wxColour(    36, 96, 46),  L"PlaybackPen" );
   DEFINE_COLOUR( clrRecordingBrush,       wxColour(   190,129,129),  L"RecordingBrush" );
   DEFINE_COLOUR( clrPlaybackBrush,        wxColour(    28,171, 51),  L"PlaybackBrush" );

   DEFINE_COLOUR( clrRulerRecordingBrush,  wxColour(   196,196,196),  L"RulerRecordingBrush" );
   DEFINE_COLOUR( clrRulerRecordingPen,    wxColour(   128,128,128),  L"RulerRecordingPen" );
   DEFINE_COLOUR( clrRulerPlaybackBrush,   wxColour(   190,129,129),  L"RulerPlaybackBrush" );
   DEFINE_COLOUR( clrRulerPlaybackPen,     wxColour(   176,  0, 28),  L"RulerPlaybackPen" );

   DEFINE_COLOUR( clrTimeFont,             wxColour(     0,  0,180),  L"TimeFont" );
   DEFINE_COLOUR( clrTimeBack,             wxColour(   160,160,160),  L"TimeBack" );
   DEFINE_COLOUR( clrTimeFontFocus,        wxColour(     0,  0,  0),  L"TimeFontFocus" );
   DEFINE_COLOUR( clrTimeBackFocus,        wxColour(   242,242,255),  L"TimeBackFocus" );

   DEFINE_COLOUR( clrLabelTextNormalBrush, wxColour(   190,190,240),  L"LabelTextNormalBrush" );
   DEFINE_COLOUR( clrLabelTextEditBrush,   wxColour(   255,255,255),  L"LabelTextEditBrush" );
   DEFINE_COLOUR( clrLabelUnselectedBrush, wxColour(   192,192,192),  L"LabelUnselectedBrush" );
   DEFINE_COLOUR( clrLabelSelectedBrush,   wxColour(   148,148,170),  L"LabelSelectedBrush" );
   DEFINE_COLOUR( clrLabelUnselectedPen,   wxColour(   192,192,192),  L"LabelUnselectedPen" );
   DEFINE_COLOUR( clrLabelSelectedPen,     wxColour(   148,148,170),  L"LabelSelectedPen" );
   DEFINE_COLOUR( clrLabelSurroundPen,     wxColour(     0,  0,  0),  L"LabelSurroundPen" );

   DEFINE_COLOUR( clrTrackFocus0,          wxColour( 200, 200, 200),  L"TrackFocus0" );
   DEFINE_COLOUR( clrTrackFocus1,          wxColour( 180, 180, 180),  L"TrackFocus1" );
   DEFINE_COLOUR( clrTrackFocus2,          wxColour( 160, 160, 160),  L"TrackFocus2" );

   DEFINE_COLOUR( clrSnapGuide,            wxColour( 255, 255,   0),  L"SnapGuide" );
   DEFINE_COLOUR( clrTrackInfo,            wxColour(  64,  64,  64),  L"TrackInfo" );
   DEFINE_COLOUR( clrTrackInfoSelected,    wxColour(  93,  65,  93),  L"TrackInfoSelected" );

   DEFINE_COLOUR( clrLight,                wxColour(  60,  60,  60),  L"Light" );
   DEFINE_COLOUR( clrMedium,               wxColour(  43,  43,  43),  L"Medium" );
   DEFINE_COLOUR( clrDark,                 wxColour(  20,  20,  20),  L"Dark" );

   DEFINE_COLOUR( clrLightSelected,        wxColour(  93,  65,  93),  L"LightSelected" );
   DEFINE_COLOUR( clrMediumSelected,       wxColour(  93,  43,  93),  L"MediumSelected" );
   DEFINE_COLOUR( clrDarkSelected,         wxColour(  93,  20,  93),  L"DarkSelected" );

   DEFINE_COLOUR( clrClipped,    wxColour(255,   0,   0), L"Clipped");
   DEFINE_COLOUR( clrMuteClipped,wxColour(136, 136, 144), L"MuteClipped");

   DEFINE_COLOUR( clrProgressDone,     wxColour(60, 240, 60, 128),   L"ProgressDone");
   DEFINE_COLOUR( clrProgressNotYet,   wxColour(255, 255, 255,220), L"ProgressNotYet");
   DEFINE_COLOUR( clrSyncLockSel,          wxColour(192, 192, 192),      L"SyncLockSel");

   DEFINE_COLOUR( clrSelTranslucent,   wxColour(104, 104, 148, 127), L"SelTranslucent");
   // This is for waveform drawing, selected outside of clips
   DEFINE_COLOUR( clrBlankSelected, wxColour(170, 170, 192), L"BlankSelected");

   DEFINE_COLOUR( clrSliderLight,         wxColour(   1,   1,   1),  L"SliderLight" );
   DEFINE_COLOUR( clrSliderMain,          wxColour(  43,  43,  43),  L"SliderMain" );
   DEFINE_COLOUR( clrSliderDark,          wxColour(   1,   1,   1),  L"SliderDark" );
   DEFINE_COLOUR( clrTrackBackground,     wxColour(  20,  20,  20),  L"TrackBackground" );


   DEFINE_COLOUR( clrPlaceHolder1,        wxColour(  255,  255,  20),  L"Placeholder1" );
   DEFINE_COLOUR( clrGraphLabels,         wxColour(    0,    0,   0),  L"GraphLabels" );
   DEFINE_COLOUR( clrSpectroBackground,   wxColour(  255,  255,  20),  L"SpectroBackground" );
   DEFINE_COLOUR( clrScrubRuler,          wxColour(  255,  255,  20),  L"ScrubRuler" );
   DEFINE_COLOUR( clrTimeHours,           wxColour(  255,  255,  20),  L"TimeHours" );
   DEFINE_COLOUR( clrFoxusBox,            wxColour(  255,  255,  20),  L"FocusBox" );
   DEFINE_COLOUR( clrTrackNameText,       wxColour(  255,  255,  20),  L"TrackNameText" );
   DEFINE_COLOUR( clrMidiZebra,           wxColour(  255,  255,  20),  L"MidiZebra" );
   DEFINE_COLOUR( clrMidiLines,           wxColour(  255,  255,  20),  L"MidiLines" );
   DEFINE_COLOUR( clrTextNegativeNumbers, wxColour(    0,    0,  255), L"TextNegativeNumbers" );

   DEFINE_COLOUR( clrSpectro1,            wxColour(  191,  191,  191),  L"Spectro1" );
   DEFINE_COLOUR( clrSpectro2,            wxColour(   76,  153,  255),  L"Spectro2" );
   DEFINE_COLOUR( clrSpectro3,            wxColour(  229,   25,  229),  L"Spectro3" );
   DEFINE_COLOUR( clrSpectro4,            wxColour(  255,    0,    0),  L"Spectro4" );
   DEFINE_COLOUR( clrSpectro5,            wxColour(  255,  255,  255),  L"Spectro5" );

   DEFINE_COLOUR( clrSpectro1Sel,         wxColour(  143,  143,  143),  L"Spectro1Sel" );
   DEFINE_COLOUR( clrSpectro2Sel,         wxColour(   57,  116,  191),  L"Spectro2Sel" );
   DEFINE_COLOUR( clrSpectro3Sel,         wxColour(  172,   19,  172),  L"Spectro3Sel" );
   DEFINE_COLOUR( clrSpectro4Sel,         wxColour(  191,    0,    0),  L"Spectro4Sel" );
   DEFINE_COLOUR( clrSpectro5Sel,         wxColour(  191,  191,  191),  L"Spectro5Sel" );

