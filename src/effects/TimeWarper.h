/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: GPL v2 - see LICENSE.txt

   Dan Horgan

******************************************************************//**

\file TimeWarper.h
\brief Contains declarations for TimeWarper, IdentityTimeWarper,
ShiftTimeWarper, LinearTimeWarper, LinearInputRateSlideTimeWarper,
LinearOutputRateSlideTimeWarper, LinearInputInverseRateTimeWarper,
GeometricInputRateTimeWarper, GeometricOutputRateTimeWarper classes

\class TimeWarper
\brief Transforms one point in time to another point. For example, a time
stretching effect might use one to keep track of what happens to labels and
split points in the input.

\class IdentityTimeWarper
\brief No change to time at all

\class ShiftTimeWarper
\brief Behaves like another, given TimeWarper, except shifted by a fixed amount

\class LinearTimeWarper
\brief Linear scaling, initialised by giving two points on the line

\class LinearInputRateTimeWarper
\brief TimeScale - rate varies linearly with input

\class LinearOutputRateTimeWarper
\brief TimeScale - rate varies linearly with output

\class LinearInputInverseRateTimeWarper
\brief TimeScale - inverse rate varies linearly with input

\class GeometricInputRateTimeWarper
\brief TimeScale - rate varies geometrically with input

\class GeometricOutputRateTimeWarper
\brief TimeScale - rate varies geometrically with output

\class StepTimeWarper
\brief Like identity but with a jump

\class RegionTimeWarper
\brief No change before the specified region; during the region, warp according
to the given warper; after the region, constant shift so as to match at the end
of the warped region.

*//*******************************************************************/

#ifndef __TIMEWARPER__
#define __TIMEWARPER__

#include "MemoryX.h"

class TimeWarper /* not final */
{
public:
   virtual ~TimeWarper();
   virtual double Warp(double originalTime) const = 0;
};

class IdentityTimeWarper final : public TimeWarper
{
public:
   double Warp(double originalTime) const override;
};

class ShiftTimeWarper final : public TimeWarper
{
private:
   std::unique_ptr<TimeWarper> mWarper;
   double mShift;
public:
   ShiftTimeWarper(std::unique_ptr<TimeWarper> &&warper, double shiftAmount)
      : mWarper(std::move(warper)), mShift(shiftAmount) { }
   virtual ~ShiftTimeWarper() {}
   double Warp(double originalTime) const override;
};

class LinearTimeWarper final : public TimeWarper
{
private:
   double mScale;
   double mShift;
public:
   LinearTimeWarper(double tBefore0, double tAfter0,
                    double tBefore1, double tAfter1)
      : mScale((tAfter1 - tAfter0)/(tBefore1 - tBefore0)),
        mShift(tAfter0 - mScale*tBefore0)
   { }
   double Warp(double originalTime) const override;
};

class LinearInputRateTimeWarper final : public TimeWarper
{
private:
   LinearTimeWarper mRateWarper;
   double mRStart;
   double mTStart;
   double mScale;
public:
   LinearInputRateTimeWarper(double tStart, double tEnd,
                             double rStart, double rEnd);
   double Warp(double originalTime) const override;
};

class LinearOutputRateTimeWarper final : public TimeWarper
{
private:
   LinearTimeWarper mTimeWarper;
   double mRStart;
   double mTStart;
   double mScale;
   double mC1;
   double mC2;
public:
   LinearOutputRateTimeWarper(double tStart, double tEnd,
                              double rStart, double rEnd);
   double Warp(double originalTime) const override;
};

class LinearInputStretchTimeWarper final : public TimeWarper
{
private:
   LinearTimeWarper mTimeWarper;
   double mTStart;
   double mC1;
   double mC2;
public:
   LinearInputStretchTimeWarper(double tStart, double tEnd,
                                double rStart, double rEnd);
   double Warp(double originalTime) const override;
};

class LinearOutputStretchTimeWarper final : public TimeWarper
{
private:
   LinearTimeWarper mTimeWarper;
   double mTStart;
   double mC1;
   double mC2;
public:
   LinearOutputStretchTimeWarper(double tStart, double tEnd,
                                 double rStart, double rEnd);
   double Warp(double originalTime) const override;
};

class GeometricInputTimeWarper final : public TimeWarper
{
private:
   LinearTimeWarper mTimeWarper;
   double mTStart;
   double mScale;
   double mRatio;
public:
   GeometricInputTimeWarper(double tStart, double tEnd,
                            double rStart, double rEnd);
   double Warp(double originalTime) const override;
};

class GeometricOutputTimeWarper final : public TimeWarper
{
private:
   LinearTimeWarper mTimeWarper;
   double mTStart;
   double mScale;
   double mC0;
public:
   GeometricOutputTimeWarper(double tStart, double tEnd,
                             double rStart, double rEnd);
   double Warp(double originalTime) const override;
};

class StepTimeWarper final : public TimeWarper
{
private:
   double mTStep;
   double mOffset;
public:
   StepTimeWarper(double tStep, double offset);
   double Warp(double originalTime) const override;
};


// Note: this assumes that tStart is a fixed point of warper->warp()
class RegionTimeWarper final : public TimeWarper
{
private:
   std::unique_ptr<TimeWarper> mWarper;
   double mTStart;
   double mTEnd;
   double mOffset;
public:
   RegionTimeWarper(double tStart, double tEnd, std::unique_ptr<TimeWarper> &&warper)
      : mWarper(std::move(warper)), mTStart(tStart), mTEnd(tEnd),
         mOffset(mWarper->Warp(mTEnd)-mTEnd)
   { }
   virtual ~RegionTimeWarper() {}
   double Warp(double originalTime) const override
   {
      if (originalTime < mTStart)
      {
         return originalTime;
      } else if (originalTime < mTEnd)
      {
         return mWarper->Warp(originalTime);
      } else
      {
         return mOffset + originalTime;
      }
   }
};

#endif /* End of include guard: __TIMEWARPER__ */
