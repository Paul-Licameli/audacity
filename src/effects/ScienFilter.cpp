/**********************************************************************

  Audacity: A Digital Audio Editor

  Effect/ScienFilter.cpp

  Norm C
  Mitch Golden
  Vaughan Johnson (Preview)

*******************************************************************//**

\file ScienFilter.cpp
\brief Implements EffectScienFilter, EffectScienFilterPanel.

*//****************************************************************//**

\class EffectScienFilter
\brief An Effect that applies 'classical' IIR filters.

  Performs IIR filtering that emulates analog filters, specifically
  Butterworth, Chebyshev Type I and Type II. Highpass and lowpass filters
  are supported, as are filter orders from 1 to 10.

  The filter is applied using biquads

*//****************************************************************//**

\class EffectScienFilterPanel
\brief EffectScienFilterPanel is used with EffectScienFilter and controls
a graph for EffectScienFilter.

*//*******************************************************************/


#include "ScienFilter.h"
#include "LoadEffects.h"

#include <math.h>
#include <float.h>

#include <wx/setup.h> // for wxUSE_* macros

#include <wx/brush.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>

#include "../AColor.h"
#include "../AllThemeResources.h"
#include "../PlatformCompatibility.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ShuttleGui.h"
#include "../Theme.h"
#include "../WaveTrack.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/Ruler.h"
#include "../widgets/WindowAccessible.h"

#if !defined(M_PI)
#define PI = 3.1415926535897932384626433832795
#else
#define PI M_PI
#endif
#define square(a) ((a)*(a))

enum
{
   ID_FilterPanel = 10000,
};

enum kTypes
{
   kButterworth,
   kChebyshevTypeI,
   kChebyshevTypeII,
   nTypes
};

static const EnumValueSymbol kTypeStrings[nTypes] =
{
   /*i18n-hint: Butterworth is the name of the person after whom the filter type is named.*/
   { XO("Butterworth") },
   /*i18n-hint: Chebyshev is the name of the person after whom the filter type is named.*/
   { XO("Chebyshev Type I") },
   /*i18n-hint: Chebyshev is the name of the person after whom the filter type is named.*/
   { XO("Chebyshev Type II") }
};

enum kSubTypes
{
   kLowPass  = Biquad::kLowPass,
   kHighPass = Biquad::kHighPass,
   nSubTypes = Biquad::nSubTypes
};

static const EnumValueSymbol kSubTypeStrings[nSubTypes] =
{
   // These are acceptable dual purpose internal/visible names
   { XO("Lowpass") },
   { XO("Highpass") }
};

static_assert(nSubTypes == WXSIZEOF(kSubTypeStrings), "size mismatch");

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name       Type     Key                     Def            Min   Max               Scale
static auto Type = EnumParameter(
                           L"FilterType",       kButterworth,  0,    nTypes - 1,    1, kTypeStrings, nTypes  );
static auto Subtype = EnumParameter(
                           L"FilterSubtype",    kLowPass,      0,    nSubTypes - 1, 1, kSubTypeStrings, nSubTypes  );
static auto Order = Parameter<int>(
                           L"Order",            1,             1,    10,               1  );
static auto Cutoff = Parameter<float>(
                           L"Cutoff",           1000.0,        1.0,  FLT_MAX,          1  );
static auto Passband = Parameter<float>(
                           L"PassbandRipple",   1.0,           0.0,  100.0,            1  );
static auto Stopband = Parameter<float>(
                           L"StopbandRipple",   30.0,          0.0,  100.0,            1  );

//----------------------------------------------------------------------------
// EffectScienFilter
//----------------------------------------------------------------------------

const ComponentInterfaceSymbol EffectScienFilter::Symbol
{ XO("Classic Filters") };

#ifdef EXPERIMENTAL_SCIENCE_FILTERS
// true argument means don't automatically enable this effect
namespace{ BuiltinEffectsModule::Registration< EffectScienFilter > reg( true ); }
#endif

BEGIN_EVENT_TABLE(EffectScienFilter, wxEvtHandler)
   EVT_SIZE(EffectScienFilter::OnSize)
END_EVENT_TABLE()

EffectScienFilter::EffectScienFilter()
   : mParameters{
      [this]{
         CalcFilter();
         return true;
      },
      mFilterType, Type,
      mFilterSubtype, Subtype,
      mOrder, Order,
      mCutoff, Cutoff,
      mRipple, Passband,
      mStopbandRipple, Stopband
   }
{
   mOrder = Order.def;
   mFilterType = Type.def;
   mFilterSubtype = Subtype.def;
   mCutoff = Cutoff.def;
   mRipple = Passband.def;
   mStopbandRipple = Stopband.def;

   SetLinearEffectFlag(true);

   mdBMin = -30.0;
   mdBMax = 30.0;

   mLoFreq = 20;              // Lowest frequency to display in response graph
   mNyquist = 44100.0 / 2.0;  // only used during initialization, updated when effect is used
}

EffectScienFilter::~EffectScienFilter()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectScienFilter::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectScienFilter::GetDescription()
{
   /* i18n-hint: "infinite impulse response" */
   return XO("Performs IIR filtering that emulates analog filters");
}

wxString EffectScienFilter::ManualPage()
{
   return L"Classic_Filters";
}


// EffectDefinitionInterface implementation

EffectType EffectScienFilter::GetType()
{
   return EffectTypeProcess;
}

// EffectClientInterface implementation

unsigned EffectScienFilter::GetAudioInCount()
{
   return 1;
}

unsigned EffectScienFilter::GetAudioOutCount()
{
   return 1;
}

bool EffectScienFilter::ProcessInitialize(sampleCount WXUNUSED(totalLen), ChannelNames WXUNUSED(chanMap))
{
   for (int iPair = 0; iPair < (mOrder + 1) / 2; iPair++)
      mpBiquad[iPair].Reset();

   return true;
}

size_t EffectScienFilter::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   float *ibuf = inBlock[0];
   for (int iPair = 0; iPair < (mOrder + 1) / 2; iPair++)
   {
      mpBiquad[iPair].Process(ibuf, outBlock[0], blockLen);
      ibuf = outBlock[0];
   }

   return blockLen;
}

// Effect implementation

bool EffectScienFilter::Startup()
{
   wxString base = L"/SciFilter/";

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + L"Migrated"))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
	   double dTemp;
      gPrefs->Read(base + L"Order", &mOrder, 1);
      mOrder = std::max (1, mOrder);
      mOrder = std::min (Order.max, mOrder);
      gPrefs->Read(base + L"FilterType", &mFilterType, 0);
      mFilterType = std::max (0, mFilterType);
      mFilterType = std::min (2, mFilterType);
      gPrefs->Read(base + L"FilterSubtype", &mFilterSubtype, 0);
      mFilterSubtype = std::max (0, mFilterSubtype);
      mFilterSubtype = std::min (1, mFilterSubtype);
      gPrefs->Read(base + L"Cutoff", &dTemp, 1000.0);
      mCutoff = (float)dTemp;
      mCutoff = std::max (1.0, mCutoff);
      mCutoff = std::min (1000000.0, mCutoff);
      gPrefs->Read(base + L"Ripple", &dTemp, 1.0);
      mRipple = dTemp;
      mRipple = std::max (0.0, mRipple);
      mRipple = std::min (100.0, mRipple);
      gPrefs->Read(base + L"StopbandRipple", &dTemp, 30.0);
      mStopbandRipple = dTemp;
      mStopbandRipple = std::max (0.0, mStopbandRipple);
      mStopbandRipple = std::min (100.0, mStopbandRipple);

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + L"Migrated", true);
      gPrefs->Flush();
   }

   return true;
}

bool EffectScienFilter::Init()
{
   int selcount = 0;
   double rate = 0.0;

   auto trackRange = inputTracks()->Selected< const WaveTrack >();

   {
      auto t = *trackRange.begin();
      mNyquist =
         (t
            ? t->GetRate()
            : mProjectRate)
         / 2.0;
   }

   for (auto t : trackRange)
   {
      if (selcount == 0)
      {
         rate = t->GetRate();
      }
      else
      {
         if (t->GetRate() != rate)
         {
            Effect::MessageBox(
               XO(
"To apply a filter, all selected tracks must have the same sample rate.") );
            return false;
         }
      }
      selcount++;
   }

   mPrevdBMax = mPrevdBMin = 0.0;
   mdBMin = -30.0;
   mdBMax = 30.0;

   return true;
}

void EffectScienFilter::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;

   const auto type1Enabler = [this]{ return mFilterType == kChebyshevTypeI; };
   const auto type2Enabler = [this]{ return mFilterType == kChebyshevTypeII; };

   S.AddSpace(5);
   S.SetSizerProportion(1);
   S.StartMultiColumn(3, wxEXPAND);
   {
      S.SetStretchyCol(1);
      S.SetStretchyRow(0);

      // -------------------------------------------------------------------
      // ROW 1: Freq response panel and sliders for vertical scale
      // -------------------------------------------------------------------

      S.StartVerticalLay();
      {
         S.SetBorder(1);

         S.AddSpace(1, 1);

         S
            .Prop(1)
            .Position(wxALIGN_RIGHT | wxTOP)
            .Window<RulerPanel>(
               wxVERTICAL,
               wxSize{ 100, 100 }, // Ruler can't handle small sizes
               RulerPanel::Range{ 30.0, -120.0 },
               Ruler::LinearDBFormat,
               XO("dB"),
               RulerPanel::Options{}
                  .LabelEdges(true) )
            .Assign(mdBRuler);
   
         S.AddSpace(1, 1);
      }
      S.EndVerticalLay();

      S.SetBorder(5);

      S
         .Prop(1)
         .Position(wxEXPAND | wxRIGHT)
         .MinSize( { -1, -1 } )
         .Window<EffectScienFilterPanel>(
             this, mLoFreq, mNyquist )
         .Assign(mPanel);

      S.StartVerticalLay();
      {
         S
            .AddVariableText(XO("+ dB"), false, wxCENTER);

         S
            .Style(wxSL_VERTICAL | wxSL_INVERSE)
#if wxUSE_ACCESSIBILITY
            .Accessible(
               MakeAccessibleFactory<SliderAx>( XO("%d dB")) )
#endif
            .VariableText( [this]{ return ControlText{ XO("Max dB"), {},
               // tooltip
               XO("%d dB").Format( (int)mdBMax )
            }; } )
            .Target( mdBMax )
            .AddSlider( {}, 10, 20, 0);

         S
            .Style(wxSL_VERTICAL | wxSL_INVERSE)
#if wxUSE_ACCESSIBILITY
            .Accessible(
               MakeAccessibleFactory<SliderAx>( XO("%d dB")) )
#endif
            .VariableText( [this]{ return ControlText{ XO("Min dB"), {},
               // tooltip
               XO("%d dB").Format( (int)mdBMin )
            }; } )
            .Target( mdBMin )
            .AddSlider( {}, -10, -10, -120);

         S
            .AddVariableText(XO("- dB"), false, wxCENTER);
      }
      S.EndVerticalLay();

      // -------------------------------------------------------------------
      // ROW 2: Frequency ruler
      // -------------------------------------------------------------------

      S.AddSpace(1, 1);

      S
         .Prop(1)
         .Position(wxEXPAND | wxALIGN_LEFT | wxRIGHT)
         .Window<RulerPanel>(
            wxHORIZONTAL,
            wxSize{ 100, 100 }, // Ruler can't handle small sizes
            RulerPanel::Range{ mLoFreq, mNyquist },
            Ruler::IntFormat,
            TranslatableString{},
            RulerPanel::Options{}
               .Log(true)
               .Flip(true)
               .LabelEdges(true) )
         .Assign(mfreqRuler);

      S.AddSpace(1, 1);

      // -------------------------------------------------------------------
      // ROW 3 and 4: Type, Order, Ripple, Subtype, Cutoff
      // -------------------------------------------------------------------

      S.AddSpace(1, 1);
      S.SetSizerProportion(0);
      S.StartMultiColumn(8, wxALIGN_CENTER);
      {
         wxASSERT(nTypes == WXSIZEOF(kTypeStrings));

         S
            .Focus()
            .MinSize( { -1, -1 } )
            .Target(mFilterType)
            .AddChoice(XXO("&Filter Type:"),
               Msgids(kTypeStrings, nTypes) );

         S
            .MinSize( { -1, -1 } )
            .Target( Transform( mOrder,
               [](double output){ return output - 1; },
               [](double input){ return input + 1; } ) )
            /*i18n-hint: 'Order' means the complexity of the filter, and is a number between 1 and 10.*/
            .AddChoice(XXO("O&rder:"),
               []{
                  TranslatableStrings orders;
                  for (int i = 1; i <= 10; i++)
                     orders.emplace_back( Verbatim("%d").Format( i ) );
                  return orders;
               }() );

         S.AddSpace(1, 1);

         S
            .Enable( type1Enabler )
            .AddVariableText( XO("&Passband Ripple:"),
               false, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

         S
            .Text(XO("Passband Ripple (dB)"))
            .Enable( type1Enabler )
            .Target( mRipple,
               NumValidatorStyle::DEFAULT, 1,
               Passband.min, Passband.max)
            .AddTextBox( {}, L"", 10);

         S
            .AddVariableText(XO("dB"),
               false, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);

         S
            .MinSize( { -1, -1 } )
            .Target(mFilterSubtype)
            .AddChoice(XXO("&Subtype:"),
               Msgids(kSubTypeStrings, nSubTypes) );

         S
            .Text(XO("Cutoff (Hz)"))
            .Target( mCutoff,
               NumValidatorStyle::DEFAULT, 1,
               Cutoff.min, mNyquist - 1)
            .AddTextBox(XXO("C&utoff:"), L"", 10);

         S
            .AddUnits(XO("Hz"));

         S
            .Enable( type2Enabler )
            .AddVariableText(XO("Minimum S&topband Attenuation:"),
               false, wxALL | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);

         S
            .Text(XO("Minimum S&topband Attenuation (dB)"))
            .Enable( type2Enabler )
            .Target( mStopbandRipple,
               NumValidatorStyle::DEFAULT, 1,
               Stopband.min, Stopband.max)
            .AddTextBox( {}, L"", 10);

         S
            .Enable( type2Enabler )
            .AddVariableText(XO("dB"),
               false, wxALL | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
      }
      S.EndMultiColumn();
      S.AddSpace(1, 1);
   }
   S.EndMultiColumn();

   return;
}

//
// Populate the window with relevant variables
//

bool EffectScienFilter::TransferDataFromWindow()
{
   CalcFilter();

   return true;
}

bool EffectScienFilter::TransferDataToWindow()
{
   if ( mPrevdBMin != mdBMin || mPrevdBMax != mdBMax ) {
      mPrevdBMin = mdBMin;
      mPrevdBMax = mdBMax;
      mPanel->SetDbRange(mdBMin, mdBMax);

      // Refresh ruler if values have changed
      int w1, w2, h;
      mdBRuler->ruler.GetMaxSize(&w1, &h);
      mdBRuler->ruler.SetRange(mdBMax, mdBMin);
      mdBRuler->ruler.GetMaxSize(&w2, &h);
      if( w1 != w2 )   // Reduces flicker
      {
         mdBRuler->SetSize(wxSize(w2,h));
         mUIParent->Layout();
         mfreqRuler->Refresh(false);
      }
      mdBRuler->Refresh(false);
   }

   mPanel->Refresh(false);

   return true;
}

// EffectScienFilter implementation

void EffectScienFilter::CalcFilter()
{
   switch (mFilterType)
   {
   case kButterworth:
      mpBiquad = Biquad::CalcButterworthFilter(mOrder, mNyquist, mCutoff, mFilterSubtype);
      break;
   case kChebyshevTypeI:
      mpBiquad = Biquad::CalcChebyshevType1Filter(mOrder, mNyquist, mCutoff, mRipple, mFilterSubtype);
      break;
   case kChebyshevTypeII:
      mpBiquad = Biquad::CalcChebyshevType2Filter(mOrder, mNyquist, mCutoff, mStopbandRipple, mFilterSubtype);
      break;
   }
}

float EffectScienFilter::FilterMagnAtFreq(float Freq)
{
   float Magn;
   if (Freq >= mNyquist)
      Freq = mNyquist - 1;	// prevent tan(PI/2)
   float FreqWarped = tan (PI * Freq/(2*mNyquist));
   if (mCutoff >= mNyquist)
      mCutoff = mNyquist - 1;
   float CutoffWarped = tan (PI * mCutoff/(2*mNyquist));
   float fOverflowThresh = pow (10.0, 12.0 / (2*mOrder));    // once we exceed 10^12 there's not much to be gained and overflow could happen

   switch (mFilterType)
   {
   case kButterworth:		// Butterworth
   default:
      switch (mFilterSubtype)
      {
      case kLowPass:	// lowpass
      default:
         if (FreqWarped/CutoffWarped > fOverflowThresh)	// prevent pow() overflow
            Magn = 0;
         else
            Magn = sqrt (1 / (1 + pow (FreqWarped/CutoffWarped, 2*mOrder)));
         break;
      case kHighPass:	// highpass
         if (FreqWarped/CutoffWarped > fOverflowThresh)
            Magn = 1;
         else
            Magn = sqrt (pow (FreqWarped/CutoffWarped, 2*mOrder) / (1 + pow (FreqWarped/CutoffWarped, 2*mOrder)));
         break;
      }
      break;

   case kChebyshevTypeI:     // Chebyshev Type 1
      double eps; eps = sqrt(pow (10.0, std::max(0.001, mRipple)/10.0) - 1);
      double chebyPolyVal;
      switch (mFilterSubtype)
      {
      case 0:	// lowpass
      default:
         chebyPolyVal = Biquad::ChebyPoly(mOrder, FreqWarped/CutoffWarped);
         Magn = sqrt (1 / (1 + square(eps) * square(chebyPolyVal)));
         break;
      case 1:
         chebyPolyVal = Biquad::ChebyPoly(mOrder, CutoffWarped/FreqWarped);
         Magn = sqrt (1 / (1 + square(eps) * square(chebyPolyVal)));
         break;
      }
      break;

   case kChebyshevTypeII:     // Chebyshev Type 2
      eps = 1 / sqrt(pow (10.0, std::max(0.001, mStopbandRipple)/10.0) - 1);
      switch (mFilterSubtype)
      {
      case kLowPass:	// lowpass
      default:
         chebyPolyVal = Biquad::ChebyPoly(mOrder, CutoffWarped/FreqWarped);
         Magn = sqrt (1 / (1 + 1 / (square(eps) * square(chebyPolyVal))));
         break;
      case kHighPass:
         chebyPolyVal = Biquad::ChebyPoly(mOrder, FreqWarped/CutoffWarped);
         Magn = sqrt (1 / (1 + 1 / (square(eps) * square(chebyPolyVal))));
         break;
      }
      break;
   }

   return Magn;
}

void EffectScienFilter::OnSize(wxSizeEvent & evt)
{
   // On Windows the Passband and Stopband boxes do not refresh properly
   // on a resize...no idea why.
   mUIParent->Refresh();
   evt.Skip();
}

//----------------------------------------------------------------------------
// EffectScienFilterPanel
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EffectScienFilterPanel, wxPanelWrapper)
    EVT_PAINT(EffectScienFilterPanel::OnPaint)
    EVT_SIZE(EffectScienFilterPanel::OnSize)
END_EVENT_TABLE()

EffectScienFilterPanel::EffectScienFilterPanel(
   wxWindow *parent, wxWindowID winid,
   EffectScienFilter *effect, double lo, double hi)
:  wxPanelWrapper(parent, winid, wxDefaultPosition, wxSize(400, 200))
{
   mEffect = effect;
   mParent = parent;

   mBitmap = NULL;
   mWidth = 0;
   mHeight = 0;
   mLoFreq = 0.0;
   mHiFreq = 0.0;
   mDbMin = 0.0;
   mDbMax = 0.0;

   SetFreqRange(lo, hi);
}

EffectScienFilterPanel::~EffectScienFilterPanel()
{
}

void EffectScienFilterPanel::SetFreqRange(double lo, double hi)
{
   mLoFreq = lo;
   mHiFreq = hi;
   Refresh(false);
}

void EffectScienFilterPanel::SetDbRange(double min, double max)
{
   mDbMin = min;
   mDbMax = max;
   Refresh(false);
}

bool EffectScienFilterPanel::AcceptsFocus() const
{
   return false;
}

bool EffectScienFilterPanel::AcceptsFocusFromKeyboard() const
{
   return false;
}

void EffectScienFilterPanel::OnSize(wxSizeEvent & WXUNUSED(evt))
{
   Refresh(false);
}

void EffectScienFilterPanel::OnPaint(wxPaintEvent & WXUNUSED(evt))
{
   wxPaintDC dc(this);
   int width, height;
   GetSize(&width, &height);

   if (!mBitmap || mWidth != width || mHeight != height)
   {
      mWidth = width;
      mHeight = height;
      mBitmap = std::make_unique<wxBitmap>(mWidth, mHeight,24);
   }

   wxBrush bkgndBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));

   wxMemoryDC memDC;
   memDC.SelectObject(*mBitmap);

   wxRect bkgndRect;
   bkgndRect.x = 0;
   bkgndRect.y = 0;
   bkgndRect.width = mWidth;
   bkgndRect.height = mHeight;
   memDC.SetBrush(bkgndBrush);
   memDC.SetPen(*wxTRANSPARENT_PEN);
   memDC.DrawRectangle(bkgndRect);

   bkgndRect.y = mHeight;
   memDC.DrawRectangle(bkgndRect);

   wxRect border;
   border.x = 0;
   border.y = 0;
   border.width = mWidth;
   border.height = mHeight;

   memDC.SetBrush(*wxWHITE_BRUSH);
   memDC.SetPen(*wxBLACK_PEN);
   memDC.DrawRectangle(border);

   mEnvRect = border;
   mEnvRect.Deflate(2, 2);

   // Pure blue x-axis line
   memDC.SetPen(wxPen(theTheme.Colour(clrGraphLines), 1, wxPENSTYLE_SOLID));
   int center = (int) (mEnvRect.height * mDbMax / (mDbMax - mDbMin) + 0.5);
   AColor::Line(memDC,
                mEnvRect.GetLeft(), mEnvRect.y + center,
                mEnvRect.GetRight(), mEnvRect.y + center);

   //Now draw the actual response that you will get.
   //mFilterFunc has a linear scale, window has a log one so we have to fiddle about
   memDC.SetPen(wxPen(theTheme.Colour(clrResponseLines), 3, wxPENSTYLE_SOLID));
   double scale = (double) mEnvRect.height / (mDbMax - mDbMin);    // pixels per dB
   double yF;                                                     // gain at this freq

   double loLog = log10(mLoFreq);
   double step = log10(mHiFreq) - loLog;
   step /= ((double) mEnvRect.width - 1.0);
   double freq;                                    // actual freq corresponding to x position
   int x, y, xlast = 0, ylast = 0;
   for (int i = 0; i < mEnvRect.width; i++)
   {
      x = mEnvRect.x + i;
      freq = pow(10.0, loLog + i * step);          //Hz
      yF = mEffect->FilterMagnAtFreq (freq);
      yF = LINEAR_TO_DB(yF);

      if (yF < mDbMin)
      {
         yF = mDbMin;
      }

      yF = center-scale * yF;
      if (yF > mEnvRect.height)
      {
         yF = (double) mEnvRect.height - 1.0;
      }
      if (yF < 0.0)
      {
         yF = 0.0;
      }
      y = (int) (yF + 0.5);

      if (i != 0 && (y < mEnvRect.height - 1 || ylast < mEnvRect.y + mEnvRect.height - 1))
      {
         AColor::Line(memDC, xlast, ylast, x, mEnvRect.y + y);
      }
      xlast = x;
      ylast = mEnvRect.y + y;
   }

   memDC.SetPen(*wxBLACK_PEN);
   mEffect->mfreqRuler->ruler.DrawGrid(memDC, mEnvRect.height + 2, true, true, 0, 1);
   mEffect->mdBRuler->ruler.DrawGrid(memDC, mEnvRect.width + 2, true, true, 1, 2);

   dc.Blit(0, 0, mWidth, mHeight, &memDC, 0, 0, wxCOPY, FALSE);

   memDC.SelectObject(wxNullBitmap);
}
