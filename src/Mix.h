/**********************************************************************

  Audacity: A Digital Audio Editor

  Mix.h

  Dominic Mazzoni
  Markus Meyer

********************************************************************//**

\class ArrayOf
\brief Memory.h template class for making an array of float, bool, etc.

\class ArraysOf
\brief memory.h template class for making an array of arrays.

*//********************************************************************/

#ifndef __AUDACITY_MIX__
#define __AUDACITY_MIX__

#include "audacity/Types.h"
#include "SampleFormat.h"
#include <functional>
#include <vector>

class sampleCount;
class Resample;
class BoundedEnvelope;
class WaveTrackFactory;
class TrackList;
class WaveTrack;
using WaveTrackConstArray = std::vector < std::shared_ptr < const WaveTrack > >;
class SampleTrackCache;

/** @brief Mixes together all input tracks, applying any envelopes, amplitude
 * gain, panning, and real-time effects in the process.
 *
 * Takes one or more tracks as input; of all the WaveTrack s that are selected,
 * it mixes them together, applying any envelopes, amplitude gain, panning, and
 * real-time effects in the process.  The resulting pair of tracks (stereo) are
 * "rendered" and have no effects, gain, panning, or envelopes. Other sorts of
 * tracks are ignored.
 * If the start and end times passed are the same this is taken as meaning
 * no explicit time range to process, and the whole occupied length of the
 * input tracks is processed.
 */
void AUDACITY_DLL_API MixAndRender(TrackList * tracks, WaveTrackFactory *factory,
                  double rate, sampleFormat format,
                  double startTime, double endTime,
                  std::shared_ptr<WaveTrack> &uLeft,
                  std::shared_ptr<WaveTrack> &uRight);

void MixBuffers(unsigned numChannels, int *channelFlags, float *gains,
                const float *src,
                samplePtr *dests, int len, bool interleaved);

class AUDACITY_DLL_API MixerSpec
{
   unsigned mNumTracks, mNumChannels, mMaxNumChannels;

   void Alloc();

public:
   ArraysOf<bool> mMap;

   MixerSpec( unsigned numTracks, unsigned maxNumChannels );
   MixerSpec( const MixerSpec &mixerSpec );
   virtual ~MixerSpec();

   bool SetNumChannels( unsigned numChannels );
   unsigned GetNumChannels() { return mNumChannels; }

   unsigned GetMaxNumChannels() { return mMaxNumChannels; }
   unsigned GetNumTracks() { return mNumTracks; }

   MixerSpec& operator=( const MixerSpec &mixerSpec );
};

class AUDACITY_DLL_API Mixer {
 public:

    // An argument to Mixer's constructor
    class AUDACITY_DLL_API WarpOptions
    {
    public:
       //! Type of hook function for default time warp
       using DefaultWarpFunction =
          std::function< const BoundedEnvelope*(const TrackList&) >;

       //! Install a default warp function, returning the previously installed
       static DefaultWarpFunction SetDefaultWarpFunction(DefaultWarpFunction);

       //! Apply the default warp function
       static const BoundedEnvelope *DefaultWarp(const TrackList &list);

       //! Construct using the default warp function
       explicit WarpOptions(const TrackList &list);

       //! Construct with an explicit warp
       explicit WarpOptions(const BoundedEnvelope *e);

       //! Construct with no time warp
       WarpOptions(double min, double max);

    private:
       friend class Mixer;
       const BoundedEnvelope *envelope = nullptr;
       double minSpeed, maxSpeed;
    };

    //
   // Constructor / Destructor
   //

   Mixer(const WaveTrackConstArray &inputTracks, bool mayThrow,
         const WarpOptions &warpOptions,
         double startTime, double stopTime,
         unsigned numOutChannels, size_t outBufferSize, bool outInterleaved,
         double outRate, sampleFormat outFormat,
         bool highQuality = true, MixerSpec *mixerSpec = nullptr,
         bool applytTrackGains = true);

   virtual ~ Mixer();

   //
   // Processing
   //

   /// Process a maximum of 'maxSamples' samples and put them into
   /// a buffer which can be retrieved by calling GetBuffer().
   /// Returns number of output samples, or 0, if there are no
   /// more samples that must be processed.
   size_t Process(size_t maxSamples);

   /// Restart processing at beginning of buffer next time
   /// Process() is called.
   void Restart();

   /// Reposition processing to absolute time next time
   /// Process() is called.
   void Reposition(double t, bool bSkipping = false);

   // Used in scrubbing.
   void SetTimesAndSpeed(double t0, double t1, double speed);
   void SetSpeedForPlayAtSpeed(double speed);
   void SetSpeedForKeyboardScrubbing(double speed, double startTime);

   /// Current time in seconds (unwarped, i.e. always between startTime and stopTime)
   /// This value is not accurate, it's useful for progress bars and indicators, but nothing else.
   double MixGetCurrentTime();

   /// Retrieve the main buffer or the interleaved buffer
   constSamplePtr GetBuffer();

   /// Retrieve one of the non-interleaved buffers
   constSamplePtr GetBuffer(int channel);

 private:

   void Clear();
   size_t MixSameRate(int *channelFlags, SampleTrackCache &cache,
                           sampleCount *pos);

   size_t MixVariableRates(int *channelFlags, SampleTrackCache &cache,
                                sampleCount *pos, float *queue,
                                int *queueStart, int *queueLen,
                                Resample * pResample);

   void MakeResamplers();

 private:

    // Input
   const size_t     mNumInputTracks;
   ArrayOf<SampleTrackCache> mInputTrack;
   bool             mbVariableRates;
   const BoundedEnvelope *mEnvelope;
   ArrayOf<sampleCount> mSamplePos;
   const bool       mApplyTrackGains;
   Doubles          mEnvValues;
   double           mT0; // Start time
   double           mT1; // Stop time (none if mT0==mT1)
   double           mTime;  // Current time (renamed from mT to mTime for consistency with AudioIO - mT represented warped time there)
   ArrayOf<std::unique_ptr<Resample>> mResample;
   const size_t     mQueueMaxLen;
   FloatBuffers     mSampleQueue;
   ArrayOf<int>     mQueueStart;
   ArrayOf<int>     mQueueLen;
   size_t           mProcessLen;
   MixerSpec        *mMixerSpec;

   // Output
   size_t              mMaxOut;
   const unsigned   mNumChannels;
   Floats           mGains;
   unsigned         mNumBuffers;
   size_t              mBufferSize;
   size_t              mInterleavedBufferSize;
   const sampleFormat mFormat;
   bool             mInterleaved;
   ArrayOf<SampleBuffer> mBuffer;
   ArrayOf<Floats>  mTemp;
   Floats           mFloatBuffer;
   const double     mRate;
   double           mSpeed;
   bool             mHighQuality;
   std::vector<double> mMinFactor, mMaxFactor;

   const bool       mMayThrow;
};

#endif

