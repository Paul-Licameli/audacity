/**********************************************************************

Audacity: A Digital Audio Editor

SampleTrack.h
@brief abstract Track sub-type that maps times to sample values

Paul Licameli split from WaveTrack.h

**********************************************************************/

#ifndef __AUDACITY_SAMPLE_TRACK__
#define __AUDACITY_SAMPLE_TRACK__

#include "SampleFormat.h"
#include "Track.h"

class sampleCount;
class SampleTrack;

using SampleTrackCaches = ClientData::Site<
   SampleTrack,
   ClientData::Cloneable< ClientData::UniquePtr >,
   ClientData::DeepCopying
>;

class AUDACITY_DLL_API SampleTrack /* not final */
   : public PlayableTrack
   , public SampleTrackCaches
{
public:
   using Caches = SampleTrackCaches;

   ~SampleTrack() override;

   const TypeInfo &GetTypeInfo() const override;
   static const TypeInfo &ClassTypeInfo();

   virtual sampleFormat GetSampleFormat() const = 0;

   virtual ChannelType GetChannelIgnoringPan() const = 0;

   // Old gain is used in playback in linearly interpolating
   // the gain.
   virtual float GetOldChannelGain(int channel) const = 0;

   virtual double GetRate() const = 0;

   //! Fetch envelope values corresponding to uniformly separated sample times starting at the given time.
   virtual void GetEnvelopeValues(double *buffer, size_t bufferLen,
                         double t0) const = 0;

   //! Takes gain and pan into account
   virtual float GetChannelGain(int channel) const = 0;

   //! This returns a nonnegative number of samples meant to size a memory buffer
   virtual size_t GetBestBlockSize(sampleCount t) const = 0;

   //! This returns a nonnegative number of samples meant to size a memory buffer
   virtual size_t GetMaxBlockSize() const = 0;

   //! This returns a possibly large or negative value
   virtual sampleCount GetBlockStart(sampleCount t) const = 0;

   virtual bool Get(samplePtr buffer, sampleFormat format,
      sampleCount start, size_t len,
      fillFormat fill = fillZero,
      bool mayThrow = true,
      // Report how many samples were copied from within clips, rather than
      // filled according to fillFormat; but these were not necessarily one
      // contiguous range.
      sampleCount * pNumWithinClips = nullptr) const = 0;

   /** @brief Convert correctly between an (absolute) time in seconds and a number of samples.
    *
    * This method will not give the correct results if used on a relative time (difference of two
    * times). Each absolute time must be converted and the numbers of samples differenced:
    *    sampleCount start = track->TimeToLongSamples(t0);
    *    sampleCount end = track->TimeToLongSamples(t1);
    *    sampleCount len = (sampleCount)(end - start);
    * NOT the likes of:
    *    sampleCount len = track->TimeToLongSamples(t1 - t0);
    * See also SampleTrack::TimeToLongSamples().
    * @param t0 The time (floating point seconds) to convert
    * @return The number of samples from the start of the track which lie before the given time.
    */
   sampleCount TimeToLongSamples(double t0) const;
   /** @brief Convert correctly between a number of samples and an (absolute) time in seconds.
    *
    * @param pos The time number of samples from the start of the track to convert.
    * @return The time in seconds.
    */
   double LongSamplesToTime(sampleCount pos) const;
};

#endif
