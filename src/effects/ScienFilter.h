/**********************************************************************

Audacity: A Digital Audio Editor

EffectScienFilter.h

Norm C
Mitch Golden
Vaughan Johnson (Preview)

***********************************************************************/

#ifndef __AUDACITY_EFFECT_SCIENFILTER__
#define __AUDACITY_EFFECT_SCIENFILTER__

#include <wx/setup.h> // for wxUSE_* macros

#include "Biquad.h"

#include "Effect.h"
#include "../Shuttle.h"

class wxBitmap;
class RulerPanel;
class ShuttleGui;

class EffectScienFilterPanel;

class EffectScienFilter final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectScienFilter();
   virtual ~EffectScienFilter();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   unsigned GetAudioOutCount() override;
   bool ProcessInitialize(sampleCount totalLen, ChannelNames chanMap = NULL) override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;

   // Effect implementation

   bool Startup() override;
   bool Init() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

private:
   // EffectScienFilter implementation

   void CalcFilter();
   float FilterMagnAtFreq(float Freq);

   void OnSize( wxSizeEvent & evt );
   void OnSlider( wxCommandEvent & evt );

private:
   double mCutoff;
   double mRipple;
   double mStopbandRipple;
   int mFilterType;		// Butterworth etc.
   int mFilterSubtype;	// lowpass, highpass
   int mOrder;
   ArrayOf<Biquad> mpBiquad;

   double mdBMax;
   double mdBMin;
   double mPrevdBMax, mPrevdBMin;
   bool mEditingBatchParams;

   double mLoFreq;
   double mNyquist;

   EffectScienFilterPanel *mPanel;

   RulerPanel *mdBRuler;
   RulerPanel *mfreqRuler;

   CapturedParameters mParameters;
   CapturedParameters &Parameters() override { return mParameters; }
   DECLARE_EVENT_TABLE()

   friend class EffectScienFilterPanel;
};

class EffectScienFilterPanel final : public wxPanelWrapper
{
public:
   EffectScienFilterPanel(
      wxWindow *parent, wxWindowID winid,
      EffectScienFilter *effect, double lo, double hi);
   virtual ~EffectScienFilterPanel();

   // We don't need or want to accept focus.
   bool AcceptsFocus() const;
   // So that wxPanel is not included in Tab traversal - see wxWidgets bug 15581
   bool AcceptsFocusFromKeyboard() const;

   void SetFreqRange(double lo, double hi);
   void SetDbRange(double min, double max);

private:
   void OnPaint(wxPaintEvent & evt);
   void OnSize(wxSizeEvent & evt);

private:
   EffectScienFilter *mEffect;
   wxWindow *mParent;

   double mLoFreq;
   double mHiFreq;

   double mDbMin;
   double mDbMax;

   std::unique_ptr<wxBitmap> mBitmap;
   wxRect mEnvRect;
   int mWidth;
   int mHeight;

   friend class EffectScienFilter;

   DECLARE_EVENT_TABLE()
};

#endif
