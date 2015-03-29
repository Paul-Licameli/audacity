 /**********************************************************************

Audacity: A Digital Audio Editor

ViewInfo.cpp

Paul Licameli

**********************************************************************/

#include "ViewInfo.h"
#include <algorithm>
#include "Internat.h"
#include "xml/XMLWriter.h"

namespace {
static const double gMaxZoom = 6000000;
static const double gMinZoom = 0.001;
}

#ifdef EXPERIMENTAL_FISHEYE
#include "Prefs.h"
#include <wx/intl.h>

namespace {
enum FisheyeStyle {
   STYLE_SIMPLE,
   STYLE_LINEAR,

   STYLE_NONLINEAR,
   STYLE_NUM_STYLES,

   // Not ready...
   STYLE_NONLINEAR_2,
};

struct Function {
   bool polynomial;
   double a, b, c, d;
   double alpha, constant;
   double interval, coeffLeft, coeffRight;
   mutable double lastSolution;
   Function()
      : polynomial(false)
      , a(0), b(0), c(0), d(0)
      , alpha(0), constant(0)
      , interval(0), coeffLeft(0), coeffRight(0)
      , lastSolution(0)
   {}
   void initialize(bool poly, double left, double right, double iv, double integral)
   {
      polynomial = poly;
      interval = iv;
      lastSolution = 0.0;
      if (polynomial) {
         const double K = (6 / (iv * iv)) * ((left + right) / 2 - integral / iv);
         a = K / 3;
         b = (right - left) / (2 * iv) - (K * iv)/ 2;
         c = left;
         d = 0;
      }
      else {
         coeffLeft = left;
         coeffRight = right;
         alpha = ((coeffLeft + coeffRight) * interval) / integral - 1.0;
         constant = (interval * coeffLeft) / (alpha + 1.0);
      }
   }
   double evaluate(double x) const
   {
      if (polynomial)
         return d + x * (c + x * (b + x * a));
      else {
         const double xa = x / interval,
            alpha1 = alpha + 1.0,
            term0 = pow(xa, alpha1) * coeffRight,
            term1 = pow(1.0 - xa, alpha1) * coeffLeft;
         return
            constant + (interval / alpha1) * (term0 - term1);
      }
   }
   double evaluateDerivative(double x) const
   {
      if (polynomial)
         return c + x * (2 * b + x * 3 * a);
      const double xa = x / interval,
         term0 = pow(xa, alpha) * coeffRight,
         term1 = pow(1.0 - xa, alpha) * coeffLeft;
      return term0 + term1;
   }
   double solve(double y) const
   {
      // Newton's method
      static const double tolerance = 1e-9;
      double guess = lastSolution;
      double prevGuess;
      bool inTolerance;
      double bracketLeft = 0.0, bracketRight = interval;
      do {
         prevGuess = guess;
         const double value = evaluate(guess) - y;
         // Assume the function is increasing:
         if (value > 0.0)
            bracketRight = guess;
         else
            bracketLeft = guess;
         const double derivative = evaluateDerivative(guess);
         const double delta = value / derivative;
         double newGuess;
         if (delta != delta // nan
            || !isfinite(derivative)
            || (newGuess = guess - delta) <= bracketLeft ||
            newGuess >= bracketRight)
            // Fall back to bisection
            newGuess = (bracketLeft + bracketRight) / 2.0;

         guess = newGuess;
         inTolerance = (fabs(guess - prevGuess) <= tolerance);
      } while  (!inTolerance);
      lastSolution = guess;
      return guess;
   }
};

const double BEVEL_FACTOR = 1.25;
inline int pixelHalfWidth(int focusPixelHalfWidth, FisheyeStyle style)
{
   switch (style) {
   case STYLE_SIMPLE:
      return focusPixelHalfWidth;
   case STYLE_LINEAR:
   case STYLE_NONLINEAR:
      return BEVEL_FACTOR * focusPixelHalfWidth;
   default:
      wxASSERT(false);
      return 0;
   }
}

}

struct FisheyeInfo {
   FisheyeInfo();

   void Update(double zoom, double totalZoom);

   double centerTime;
   int focusPixelHalfWidth;
   ZoomInfo::FisheyeState state;

   FisheyeStyle style;

   double magnification; // relative to ViewInfo::zoom

   // Remaining fields are computed from the above fields
   // and from ZoomInfo::zoom.
   Function function;
   double transition;
   int transitionWidth;

   // In case we use a polynomial, this still holds the correct
   // average zoom value:
   double transitionZoom;
};

FisheyeInfo::FisheyeInfo()
   : centerTime(0.0)
   , magnification(2.0)
   , focusPixelHalfWidth(150)
   , state(ZoomInfo::HIDDEN)
   , style(STYLE_SIMPLE)
   , function()
   , transition(0.0)
   , transitionWidth(0)
   , transitionZoom(0.0)
{
}

void FisheyeInfo::Update(double zoom, double totalZoom)
{
   const int halfWidth = pixelHalfWidth(focusPixelHalfWidth, style);
   transitionWidth = halfWidth - focusPixelHalfWidth;

   transition =
      (halfWidth - 1) / zoom - (focusPixelHalfWidth - 1) / totalZoom;

   if (style != STYLE_SIMPLE) {
      wxASSERT(transition > 0.0);
      transitionZoom = transitionWidth / transition;
   }
   // else, transitionZoom is not needed.

   // Update some precalculated results.
   if (style == STYLE_NONLINEAR) {
      // The derivative of the function has value 1 / zoom at 0,
      // value 1 / totalZoom at transitionWidth,
      // is positive everywhere on that interval,
      // and integrates over it to transition.
      // The constant is chosen so the function evaluates to 0 at 0.
      function.initialize(true, 1.0 / zoom, 1.0 / totalZoom, transitionWidth, transition);
   }
   else if (style == STYLE_NONLINEAR_2) {
      // The derivative of the function has value zoom at 0,
      // value totalZoom at transition,
      // is positive everywhere on that interval,
      // and integrates over it to transitionWidth.
      // The constant is chosen so the function evaluates to 0 at 0.
      function.initialize(false, zoom, totalZoom, transition, transitionWidth);
   }
}
#endif

ZoomInfo::ZoomInfo(double start, double duration, double pixelsPerSecond)
   : vpos(0)
   , h(start)
   , screen(duration)
   , zoom(pixelsPerSecond)

#ifdef EXPERIMENTAL_FISHEYE
   , fisheye(new FisheyeInfo)
#endif

   // PRL: get rid of this?  It can only become true by preference
   // /GUI/AutoScroll which is read and never written.
   , bUpdateTrackIndicator(true)

   // PRL: This is never made true.  Remove?
   , bIsPlaying(false)
{
   UpdatePrefs();

#ifdef EXPERIMENTAL_FISHEYE
   double value;
   gPrefs->Read(wxT("/GUI/Fisheye/DefaultMagnification"), &value, 2.0);
   const double newMagnification = std::max(1.0, value);
   fisheye->magnification = newMagnification;

   UpdateFisheye();
#endif
}

ZoomInfo::~ZoomInfo()
{
#ifdef EXPERIMENTAL_FISHEYE
   delete fisheye;
#endif
}

bool ZoomInfo::UpdatePrefs()
{
   bool result = false;
#ifdef EXPERIMENTAL_FISHEYE
   int value;
   gPrefs->Read(wxT("/GUI/Fisheye/Style"), &value, 0);
   const FisheyeStyle style = FisheyeStyle(value);
   if (fisheye->style != style) {
      fisheye->style = style;
      result = true;
   }

   gPrefs->Read(wxT("/GUI/Fisheye/Width"), &value, GetFisheyeDefaultWidth());
   value /= 2;
   const int newHalfWidth = std::max(2, std::min(800, value));
   if (fisheye->focusPixelHalfWidth != newHalfWidth) {
      fisheye->focusPixelHalfWidth = newHalfWidth;
      result = true;
   }

   if (result)
      UpdateFisheye();
#endif
   return result;
}

/// Converts a position (mouse X coordinate) to
/// project time, in seconds.  Needs the left edge of
/// the track as an additional parameter.
double ZoomInfo::PositionToTime(wxInt64 position,
   wxInt64 origin
#ifdef EXPERIMENTAL_FISHEYE
   , bool ignoreFisheye
#endif
   ) const
{
#ifdef EXPERIMENTAL_FISHEYE
   if (!ignoreFisheye) {
      if (InFisheyeFocus(position, origin)) {
         const wxInt64 fisheyeCenterPosition = GetFisheyeCenterPosition(origin);
         const double totalZoom = GetFisheyeTotalZoom();
         return fisheye->centerTime +
            (position - fisheyeCenterPosition) / totalZoom;
      }
      else if (fisheye->style != STYLE_SIMPLE &&
         InFisheye(position, origin)) {
         const wxInt64 left = GetFisheyeLeftBoundary(origin),
            focusLeft = GetFisheyeFocusLeftBoundary(origin),
            right = GetFisheyeRightBoundary(origin) - 1;
         if (position < focusLeft) {
            double base = h + (left - origin) / zoom;
            switch (fisheye->style) {
            case STYLE_NONLINEAR:
               return base + fisheye->function.evaluate(position - left);
            case STYLE_NONLINEAR_2:
               return base + fisheye->function.solve(position - left);
            case STYLE_LINEAR:
            default:
               return base + (position - left) / fisheye->transitionZoom;
            }
         }
         else {
            double base = h + (right - origin) / zoom;
            switch (fisheye->style) {
            case STYLE_NONLINEAR:
               return base - fisheye->function.evaluate(right - position);
            case STYLE_NONLINEAR_2:
               return base - fisheye->function.solve(right - position);
            case STYLE_LINEAR:
            default:
               return base - (right - position) / fisheye->transitionZoom;
            }
         }
      }
   }
#endif
   return h + (position - origin) / zoom;
}


/// STM: Converts a project time to screen x position.
wxInt64 ZoomInfo::TimeToPosition(double projectTime,
   wxInt64 origin
#ifdef EXPERIMENTAL_FISHEYE
   , bool ignoreFisheye
#endif
   ) const
{
#ifdef EXPERIMENTAL_FISHEYE
   if (!ignoreFisheye && fisheye->state != ZoomInfo::HIDDEN) {
      const double
         fisheyeCenter = fisheye->centerTime,
         totalZoom = GetFisheyeTotalZoom();
      const double
         halfWidth =
         (pixelHalfWidth(fisheye->focusPixelHalfWidth, fisheye->style) - 1) / zoom,
         start = fisheyeCenter - halfWidth,
         end = fisheyeCenter + halfWidth + (1 / zoom);
      if (projectTime >= start && projectTime < end) {
         const double
            focusHalfWidth = (fisheye->focusPixelHalfWidth - 1) / totalZoom,
            focusStart = fisheyeCenter - focusHalfWidth,
            focusEnd = fisheyeCenter + focusHalfWidth + (1 / totalZoom);
         const double base = zoom * (start - h) + origin;
         double offset;
         if (projectTime >= focusStart && projectTime < focusEnd) {
            // Compute answer for increased magnification in the focus
            offset = totalZoom * (projectTime - focusStart);
            if (fisheye->style != STYLE_SIMPLE)
               offset += fisheye->transitionZoom * (focusStart - start);
         }
         else switch (fisheye->style) {
         case STYLE_SIMPLE:
         {
            if (projectTime < focusStart)
               // Collapse early hidden times leftward
               offset = 0.0;
            else
               // Collapse later hidden times rightward
               offset = zoom * (end - start);
         }
            break;
         case STYLE_LINEAR:
         {
            if (projectTime < focusStart)
               offset = fisheye->transitionZoom * (projectTime - start);
            else
               offset = zoom * (end - start) -
                  fisheye->transitionZoom * (end - projectTime);
         }
            break;
         case STYLE_NONLINEAR:
         {
            if (projectTime < focusStart)
               offset = fisheye->function.solve(projectTime - start);
            else
               offset = zoom * (end - start) -
                  fisheye->function.solve(end - projectTime);
         }
            break;
         case STYLE_NONLINEAR_2:
         {
            if (projectTime < focusStart)
               offset = fisheye->function.evaluate(projectTime - start);
            else
               offset = zoom * (end - start) -
                  fisheye->function.evaluate(end - projectTime);
         }
            break;
         default:
            wxASSERT(false);
            offset = 0;
            break;
         }
         return floor(0.5 + base + offset);
      }
   }
#endif
   return floor(0.5 +
      zoom * (projectTime - h) + origin
   );
}

bool ZoomInfo::ZoomInAvailable() const
{
   return zoom < gMaxZoom;
}

bool ZoomInfo::ZoomOutAvailable() const
{
   return zoom > gMinZoom;
}

void ZoomInfo::SetZoom(double pixelsPerSecond)
{
   zoom = std::max(gMinZoom, std::min(gMaxZoom, pixelsPerSecond));
#ifdef EXPERIMENTAL_FISHEYE
   UpdateFisheye();
#endif
}

void ZoomInfo::ZoomBy(double multiplier)
{
   SetZoom(zoom * multiplier);
}

void ZoomInfo::FindIntervals
   (double /*rate*/, Intervals &results, wxInt64 origin) const
{
   results.clear();
   results.reserve(6);

   const wxInt64 rightmost(origin + (0.5 + screen * zoom));
   wxASSERT(origin <= rightmost);
#ifdef EXPERIMENTAL_FISHEYE
   if (fisheye->state != ZoomInfo::HIDDEN) {
      switch (fisheye->style) {
      case STYLE_SIMPLE:
      {
         const wxInt64
            left1 = std::max(origin, GetFisheyeFocusLeftBoundary(origin)),
            left2 = std::max(left1, GetFisheyeFocusRightBoundary(origin));
         if (origin < left1)
            results.push_back(Interval(origin, zoom, false));
         if (left1 < left2 && left1 < rightmost)
            results.push_back(Interval(left1, GetFisheyeTotalZoom(), true));
         if (left2 < rightmost)
            results.push_back(Interval(left2, zoom, false));
      }
         break;
      case STYLE_LINEAR:
      case STYLE_NONLINEAR:
      {
         const double totalZoom = GetFisheyeTotalZoom();
         const wxInt64 left = GetFisheyeLeftBoundary(origin),
            focusLeft = GetFisheyeFocusLeftBoundary(origin),
            focusRight = GetFisheyeFocusRightBoundary(origin),
            right = GetFisheyeRightBoundary(origin);

         const wxInt64
            left1 = std::max(origin, left),
            left2 = std::max(left1, focusLeft),
            left3 = std::max(left2, focusRight),
            left4 = std::max(left3, right);

         if (origin < left1)
            results.push_back(Interval(origin, zoom, false));
         if (left1 < left2 && left1 < rightmost)
            results.push_back(Interval(left1, fisheye->transitionZoom, true));
         if (left2 < left3 && left2 < rightmost)
            results.push_back(Interval(left2, totalZoom, true));
         if (left3 < left4 && left3 < rightmost)
            results.push_back(Interval(left3, fisheye->transitionZoom, true));
         if (left4 < rightmost)
            results.push_back(Interval(left4, zoom, false));
      }
         break;
      default:
         wxASSERT(false);
      }
   }
   else
#endif
   {
      results.push_back(Interval(origin, zoom, false));
   }

   if (origin < rightmost)
      results.push_back(Interval(rightmost, 0, false));
   wxASSERT(!results.empty() && results[0].position == origin);
}

#ifdef EXPERIMENTAL_FISHEYE
//static
wxArrayString ZoomInfo::GetFisheyeStyleChoices()
{
   wxArrayString result;
   result.Add(_("Simple"));
   result.Add(_("Uniform Transition"));
   result.Add(_("Variable Transition"));
   return result;
}

SelectedRegion ZoomInfo::GetFisheyeFocusRegion() const
{
   const double
      fisheyeCenter = fisheye->centerTime,
      totalZoom = GetFisheyeTotalZoom(),
      focusHalfWidth = (fisheye->focusPixelHalfWidth - 1) / totalZoom,
      focusStart = fisheyeCenter - focusHalfWidth,
      focusEnd = fisheyeCenter + focusHalfWidth + (1 / totalZoom);

   return SelectedRegion(focusStart, focusEnd);
}

bool ZoomInfo::InFisheye(wxInt64 position, wxInt64 origin) const
{
   if (fisheye->state != ZoomInfo::HIDDEN) {
      const wxInt64
         start = GetFisheyeLeftBoundary(origin),
         end = GetFisheyeRightBoundary(origin);
      return
         position >= start &&
         position < end;
   }
   else
      return false;
}

bool ZoomInfo::InFisheyeFocus(wxInt64 position, wxInt64 origin) const
{
   if (fisheye->state != ZoomInfo::HIDDEN) {
      const wxInt64
         start = GetFisheyeFocusLeftBoundary(origin),
         end = GetFisheyeFocusRightBoundary(origin);
      return
         position >= start &&
         position < end;
   }
   else
      return false;
}

wxInt64 ZoomInfo::GetFisheyeLeftBoundary(wxInt64 origin) const
{
   const wxInt64 center = GetFisheyeCenterPosition(origin);
   return center - pixelHalfWidth(fisheye->focusPixelHalfWidth, fisheye->style) + 1;
}

wxInt64 ZoomInfo::GetFisheyeFocusLeftBoundary(wxInt64 origin) const
{
   const wxInt64 center = GetFisheyeCenterPosition(origin);
   return center - fisheye->focusPixelHalfWidth + 1;
}

wxInt64 ZoomInfo::GetFisheyeCenterPosition(wxInt64 origin) const
{
   return floor(0.5 + zoom * (fisheye->centerTime - h) + origin);
}

wxInt64 ZoomInfo::GetFisheyeFocusRightBoundary(wxInt64 origin) const
{
   const wxInt64 center = GetFisheyeCenterPosition(origin);
   return center + fisheye->focusPixelHalfWidth;
}

wxInt64 ZoomInfo::GetFisheyeRightBoundary(wxInt64 origin) const
{
   const wxInt64 center = GetFisheyeCenterPosition(origin);
   return center + pixelHalfWidth(fisheye->focusPixelHalfWidth, fisheye->style);
}

double ZoomInfo::GetFisheyeTotalZoom() const
{
   return std::min(gMaxZoom, std::max(gMinZoom, zoom * fisheye->magnification));
}

bool ZoomInfo::ZoomFisheyeBy(wxCoord position, wxCoord origin, double multiplier)
{
   const double oldMagnification = fisheye->magnification;

   // Use GetFisheyeTotalZoom() because it gives the limited magnification
   // really used in drawing.
   const double oldTotalZoom = GetFisheyeTotalZoom();
   const double newMagnification =
      std::min(gMaxZoom / zoom,
         std::max(1.0,
            multiplier * oldTotalZoom / zoom));
   if (newMagnification * zoom == oldTotalZoom)
      // No change
      return false;

   const double oldCenter = fisheye->centerTime;
   const double oldTime = PositionToTime(position, origin);
   fisheye->magnification = newMagnification;
   UpdateFisheye();

   // Supposing the focus has infinite width, move the center so that the
   // time at the mouse position is unchanged.
   // That means, solve for the center:
   // fisheye->centerTime
   //   + (position - (origin + zoom * (fisheye->centerTime - h))) / fisheyeZoom
   //   == oldTime
   const double fisheyeZoom = GetFisheyeTotalZoom();
   const double denom = (fisheyeZoom - zoom);
   if (fabs(denom) < 1e-6) {
      // can't solve
      fisheye->magnification = oldMagnification;
      UpdateFisheye();
      return false;
   }
   else {
      // Check whether the time really remains in the finite focus.
      const double newCenter =
         (oldTime * fisheyeZoom - position + origin - zoom * h) / denom;
      fisheye->centerTime = newCenter;
      if (!InFisheyeFocus(position, origin)) {
#if 0
         // Widen the fisheye to keep the mouse inside it
         int centerPosition =
            int(TimeToPosition(newCenter, origin));
         fisheye->pixelHalfWidth = 1 + abs(centerPosition - position);
#else
         fisheye->centerTime = oldCenter;
         fisheye->magnification = oldMagnification;
         UpdateFisheye();
         return false;
#endif
      }
   }

   // Do not write magnification as a preference, there is a preference
   // only for the default magnification setting for the zoom normal command.
   return true;
}

bool ZoomInfo::DefaultFisheyeZoom(wxCoord position, wxCoord origin)
{
   double value;
   gPrefs->Read(wxT("/GUI/Fisheye/DefaultMagnification"), &value, 2.0);
   const double multiplier = value / fisheye->magnification;
   return ZoomFisheyeBy(position, origin, multiplier);
}

ZoomInfo::FisheyeState ZoomInfo::GetFisheyeState() const
{
   return fisheye->state;
   // No UpdateFisheye() needed
}

void ZoomInfo::SetFisheyeState(ZoomInfo::FisheyeState state)
{
   fisheye->state = state;
}

void ZoomInfo::ChangeFisheyeStyle()
{
   fisheye->style = FisheyeStyle(
      (1 + fisheye->style) % STYLE_NUM_STYLES
   );
   UpdateFisheye();
}

void ZoomInfo::AdjustFisheyePixelWidth(int delta, int maximum)
{
   int newHalfWidth = std::min(maximum, std::max(2,
      fisheye->focusPixelHalfWidth + delta
   ));
   if (fisheye->focusPixelHalfWidth != newHalfWidth) {
      gPrefs->Write(wxT("/GUI/Fisheye/Width"), 2 * newHalfWidth);
      gPrefs->Flush();
      fisheye->focusPixelHalfWidth = newHalfWidth;
      UpdateFisheye();
   }
}

double ZoomInfo::GetFisheyeCenterTime() const
{
   return fisheye->centerTime;
}

void ZoomInfo::SetFisheyeCenterTime(double time)
{
   fisheye->centerTime = time;
   // No UpdateFisheye() needed
}

void ZoomInfo::UpdateFisheye()
{
   fisheye->Update(zoom, GetFisheyeTotalZoom());
}
#endif

ViewInfo::ViewInfo(double start, double screenDuration, double pixelsPerSecond)
   : ZoomInfo(start, screenDuration, pixelsPerSecond)
   , selectedRegion()
   , total(screen)
   , track(NULL)
   , sbarH(0)
   , sbarScreen(1)
   , sbarTotal(1)
   , sbarScale(1.0)
   , scrollStep(16)
   , bRedrawWaveform(false)
{
}

void ViewInfo::SetBeforeScreenWidth(wxInt64 width)
{
   h =
      std::max(0.0,
         std::min(total - screen,
            width / zoom));
}

void ViewInfo::WriteXMLAttributes(XMLWriter &xmlFile)
{
   xmlFile.WriteAttr(wxT("h"), h, 10);
   xmlFile.WriteAttr(wxT("zoom"), zoom, 10);

   // To do: write fisheye magnification?
}

bool ViewInfo::ReadXMLAttribute(const wxChar *attr, const wxChar *value)
{
   if (!wxStrcmp(attr, wxT("h"))) {
      Internat::CompatibleToDouble(value, &h);
      return true;
   }
   else if (!wxStrcmp(attr, wxT("zoom"))) {
      Internat::CompatibleToDouble(value, &zoom);
#ifdef EXPERIMENTAL_FISHEYE
      UpdateFisheye();
#endif
      return true;
   }
   else
      return false;
}
