/**********************************************************************

  Audacity: A Digital Audio Editor

  FreqWindow.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_FREQ_WINDOW__
#define __AUDACITY_FREQ_WINDOW__

#include <vector>
#include <wx/font.h> // member variable
#include <wx/statusbr.h> // to inherit
#include "Prefs.h"
#include "SampleFormat.h"
#include "SpectrumAnalyst.h"
#include "widgets/wxPanelWrapper.h" // to inherit

class wxMemoryDC;
class wxScrollBar;
class wxSlider;
class wxTextCtrl;
class wxButton;
class wxCheckBox;
class wxChoice;

class AudacityProject;
class FrequencyPlotDialog;
class FreqGauge;
class RulerPanel;

DECLARE_EXPORTED_EVENT_TYPE(AUDACITY_DLL_API, EVT_FREQWINDOW_RECALC, -1);

class FreqPlot final : public wxWindow
{
public:
   FreqPlot(wxWindow *parent, wxWindowID winid);

   // We don't need or want to accept focus.
   bool AcceptsFocus() const;

private:
   void OnPaint(wxPaintEvent & event);
   void OnErase(wxEraseEvent & event);
   void OnMouseEvent(wxMouseEvent & event);

private:
    FrequencyPlotDialog *freqWindow;

    DECLARE_EVENT_TABLE()
};

class FrequencyPlotDialog final : public wxDialogWrapper,
                                  public PrefsListener
{
public:
   FrequencyPlotDialog(wxWindow *parent, wxWindowID id,
              AudacityProject &project,
              const TranslatableString & title, const wxPoint & pos);
   virtual ~ FrequencyPlotDialog();

   bool Show( bool show = true ) override;

private:
   void Populate();

   void GetAudio();

   void PlotMouseEvent(wxMouseEvent & event);
   void PlotPaint(wxPaintEvent & event);

   void OnCloseWindow(wxCloseEvent & event);
   void OnCloseButton();
   void OnGetURL();
   void OnSize(wxSizeEvent & event);
   void OnPanScroller(wxScrollEvent & event);
   void OnZoomSlider(wxCommandEvent & event);
   void OnAlgChoice();
   void OnSizeChoice();
   void OnFuncChoice();
   void OnAxisChoice();
   void OnExport();
   void OnReplot();
   void OnGridOnOff(wxCommandEvent & event);
   void OnRecalc(wxCommandEvent & event);

   void SendRecalcEvent();
   void Recalc();
   void DrawPlot();
   void DrawBackground(wxMemoryDC & dc);

   // PrefsListener implementation
   void UpdatePrefs() override;

 private:
   bool mDrawGrid;
   int mSize;
   SpectrumAnalyst::Algorithm mAlg;
   int mFunc;
   int mAxis;
   int dBRange;
   AudacityProject *mProject;

#ifdef __WXMSW__
   static const int fontSize = 8;
#else
   static const int fontSize = 10;
#endif

   RulerPanel *vRuler;
   RulerPanel *hRuler;
   FreqPlot *mFreqPlot;
   FreqGauge *mProgress;

   wxRect mPlotRect;

   wxFont mFreqFont;

   std::unique_ptr<wxCursor> mArrowCursor;
   std::unique_ptr<wxCursor> mCrossCursor;

   wxButton *mExportButton;
   wxButton *mReplotButton;
   wxCheckBox *mGridOnOff;
   int mAlgChoice = 0;
   int mSizeChoice = 0;
   int mFuncChoice = 0;
   int mAxisChoice = 0;
   wxScrollBar *mPanScroller;
   wxSlider *mZoomSlider;
   wxTextCtrl *mCursorText;
   wxTextCtrl *mPeakText;


   double mRate;
   size_t mDataLen;
   Floats mData;
   size_t mWindowSize;

   bool mLogAxis;
   float mYMin;
   float mYMax;
   float mYStep;

   std::unique_ptr<wxBitmap> mBitmap;

   int mMouseX;
   int mMouseY;

   std::unique_ptr<SpectrumAnalyst> mAnalyst;

   DECLARE_EVENT_TABLE()

   friend class FreqPlot;
};

#endif
