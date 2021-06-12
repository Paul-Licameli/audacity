/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 PlaybackSchedule.cpp
 
 Paul Licameli split from AudioIOBase.cpp
 
 **********************************************************************/

#include "PlaybackSchedule.h"

#include "AudioIOBase.h"
#include "Envelope.h"
#include "Mix.h"

#include <cmath>

PlaybackPolicy::~PlaybackPolicy() = default;
void PlaybackPolicy::Initialize( PlaybackSchedule &, double rate )
{
   mRate = rate;
}
void PlaybackPolicy::Finalize( PlaybackSchedule & ){}

Mixer::WarpOptions PlaybackPolicy::MixerWarpOptions(PlaybackSchedule &schedule)
{
   return Mixer::WarpOptions{ schedule.mEnvelope };
}

PlaybackPolicy::BufferTimes
PlaybackPolicy::SuggestedBufferTimes(PlaybackSchedule &)
{
   return { 4.0, 4.0, 10.0 };
}

double PlaybackPolicy::NormalizeTrackTime( PlaybackSchedule &schedule )
{
   // Track time readout for the main thread

   // dmazzoni: This function is needed for two reasons:
   // One is for looped-play mode - this function makes sure that the
   // position indicator keeps wrapping around.  The other reason is
   // more subtle - it's because PortAudio can query the hardware for
   // the current stream time, and this query is not always accurate.
   // Sometimes it's a little behind or ahead, and so this function
   // makes sure that at least we clip it to the selection.

   // Limit the time between t0 and t1 if not scrubbing.
   // Should the limiting be necessary in any play mode if there are no bugs?
   double absoluteTime = schedule.LimitTrackTime();

   return absoluteTime;
}

bool PlaybackPolicy::AllowSeek(PlaybackSchedule &)
{
   return true;
}

bool PlaybackPolicy::Done( PlaybackSchedule &schedule,
   unsigned long outputFrames)
{
   auto diff = schedule.GetTrackTime() - schedule.mT1;
   if (schedule.ReversedTime())
      diff *= -1;
   return sampleCount(floor(diff * mRate + 0.5)) >= 0 &&
      // Require also that output frames are all consumed from ring buffer
      outputFrames == 0;
}

double PlaybackPolicy::OffsetTrackTime(
   PlaybackSchedule &schedule, double offset )
{
   const auto time = schedule.ClampTrackTime(
      schedule.GetTrackTime() + offset );
   schedule.RealTimeInit( time );
   return time;
}

std::chrono::milliseconds PlaybackPolicy::SleepInterval(PlaybackSchedule &)
{
   using namespace std::chrono;
   return 10ms;
}

PlaybackSlice
PlaybackPolicy::GetPlaybackSlice(PlaybackSchedule &schedule, size_t available)
{
   // How many samples to produce for each channel.
   const auto realTimeRemaining = schedule.RealTimeRemaining();
   auto frames = available;
   auto toProduce = frames;
   double deltat = frames / mRate;

   if (deltat > realTimeRemaining)
   {
      // Produce some extra silence so that the time queue consumer can
      // satisfy its end condition
      const double extraRealTime = (TimeQueueGrainSize + 1) / mRate;
      auto extra =
         std::min( extraRealTime, deltat - realTimeRemaining );
      auto realTime = std::max( 0.0, realTimeRemaining + extra );
      frames = realTime * mRate;
      toProduce = std::max( 0.0, realTimeRemaining * mRate );
      schedule.RealTimeAdvance( realTime );
   }
   else
      schedule.RealTimeAdvance( deltat );

   return { available, frames, toProduce };
}

std::pair<double, double>
PlaybackPolicy::AdvancedTrackTime( PlaybackSchedule &schedule,
   double trackTime, size_t nSamples )
{
   auto realDuration = nSamples / mRate;
   if (schedule.ReversedTime())
      realDuration *= -1.0;

   if (schedule.mEnvelope)
      trackTime =
         schedule.SolveWarpedLength(trackTime, realDuration);
   else
      trackTime += realDuration;
   
   if ( trackTime >= schedule.mT1 )
      return { schedule.mT1, std::numeric_limits<double>::infinity() };
   else
      return { trackTime, trackTime };
}

bool PlaybackPolicy::RepositionPlayback(
   PlaybackSchedule &, const Mixers &, size_t, size_t)
{
   return true;
}

namespace {
struct DefaultPlaybackPolicy final : PlaybackPolicy {
   ~DefaultPlaybackPolicy() override = default;
};
}

PlaybackPolicy &PlaybackSchedule::GetPolicy()
{
   if (mPolicyValid.load(std::memory_order_acquire) && mpPlaybackPolicy)
      return *mpPlaybackPolicy;

   static DefaultPlaybackPolicy defaultPolicy;
   return defaultPolicy;
}

const PlaybackPolicy &PlaybackSchedule::GetPolicy() const
{
   return const_cast<PlaybackSchedule&>(*this).GetPolicy();
}

LoopingPlaybackPolicy::~LoopingPlaybackPolicy() = default;

bool LoopingPlaybackPolicy::Done( PlaybackSchedule &, unsigned long )
{
   return false;
}

PlaybackSlice
LoopingPlaybackPolicy::GetPlaybackSlice(
   PlaybackSchedule &schedule, size_t available)
{
   // How many samples to produce for each channel.
   const auto realTimeRemaining = schedule.RealTimeRemaining();
   auto frames = available;
   auto toProduce = frames;
   double deltat = frames / mRate;

   mRemaining = realTimeRemaining * mRate;
   if (deltat > realTimeRemaining)
   {
      toProduce = frames = mRemaining;
      schedule.RealTimeAdvance( realTimeRemaining );
   }
   else
      schedule.RealTimeAdvance( deltat );

   // Don't fall into an infinite loop, if loop-playing a selection
   // that is so short, it has no samples: detect that case
   if (frames == 0) {
      bool progress = (schedule.mWarpedTime != 0.0);
      if (!progress)
         frames = available, toProduce = 0;
   }
   return { available, frames, toProduce };
}

std::pair<double, double> LoopingPlaybackPolicy::AdvancedTrackTime(
   PlaybackSchedule &schedule, double trackTime, size_t nSamples )
{
   mRemaining -= std::min(mRemaining, nSamples);
   if ( mRemaining == 0 )
      // Wrap to start
      return { schedule.mT1, schedule.mT0 };

   // Defense against cases that might cause loops not to terminate
   if ( fabs(schedule.mT0 - schedule.mT1) < 1e-9 )
      return {schedule.mT0, schedule.mT0};

   auto realDuration = nSamples / mRate;
   if (schedule.ReversedTime())
      realDuration *= -1.0;

   if (schedule.mEnvelope)
      trackTime =
         schedule.SolveWarpedLength(trackTime, realDuration);
   else
      trackTime += realDuration;

   return { trackTime, trackTime };
}

bool LoopingPlaybackPolicy::RepositionPlayback(
   PlaybackSchedule &schedule, const Mixers &playbackMixers,
   size_t frames,
   size_t available )
{
   // msmeyer: If playing looped, check if we are at the end of the buffer
   // and if yes, restart from the beginning.
   if (schedule.RealTimeRemaining() <= 0)
   {
      for (auto &pMixer : playbackMixers)
         pMixer->Reposition(schedule.mT0, true);
      schedule.RealTimeRestart();
   }
   return false;
}

void PlaybackSchedule::Init(
   const double t0, const double t1,
   const AudioIOStartStreamOptions &options,
   const RecordingSchedule *pRecordingSchedule )
{
   mpPlaybackPolicy.reset();

   if ( pRecordingSchedule )
      // It does not make sense to apply the time warp during overdub recording,
      // which defeats the purpose of making the recording synchronized with
      // the existing audio.  (Unless we figured out the inverse warp of the
      // captured samples in real time.)
      // So just quietly ignore the time track.
      mEnvelope = nullptr;
   else
      mEnvelope = options.envelope;

   mT0      = t0;
   if (pRecordingSchedule)
      mT0 -= pRecordingSchedule->mPreRoll;

   mT1      = t1;
   if (pRecordingSchedule)
      // adjust mT1 so that we don't give paComplete too soon to fill up the
      // desired length of recording
      mT1 -= pRecordingSchedule->mLatencyCorrection;

   // Main thread's initialization of mTime
   SetTrackTime( mT0 );

   if (options.policyFactory)
      mpPlaybackPolicy = options.policyFactory();

   mWarpedTime = 0.0;
   mWarpedLength = RealDuration(mT1);

   mPolicyValid.store(true, std::memory_order_release);

   mMessageChannel.Initialize();
   mMessageChannel.Write( { mT0, mT1 } );
}

double PlaybackSchedule::LimitTrackTime() const
{
   // Track time readout for the main thread
   // Allows for forward or backward play
   return ClampTrackTime( GetTrackTime() );
}

double PlaybackSchedule::ClampTrackTime( double trackTime ) const
{
   if (ReversedTime())
      return std::max(mT1, std::min(mT0, trackTime));
   else
      return std::max(mT0, std::min(mT1, trackTime));
}

double PlaybackSchedule::ComputeWarpedLength(double t0, double t1) const
{
   if (mEnvelope)
      return mEnvelope->IntegralOfInverse(t0, t1);
   else
      return t1 - t0;
}

double PlaybackSchedule::SolveWarpedLength(double t0, double length) const
{
   if (mEnvelope)
      return mEnvelope->SolveIntegralOfInverse(t0, length);
   else
      return t0 + length;
}

double PlaybackSchedule::RealDuration(double trackTime1) const
{
   return fabs(ComputeWarpedLength(mT0, trackTime1));
}

double PlaybackSchedule::RealTimeRemaining() const
{
   return mWarpedLength - mWarpedTime;
}

void PlaybackSchedule::RealTimeAdvance( double increment )
{
   mWarpedTime += increment;
}

void PlaybackSchedule::RealTimeInit( double trackTime )
{
   mWarpedTime = RealDuration( trackTime );
}

void PlaybackSchedule::RealTimeRestart()
{
   mWarpedTime = 0;
}

double RecordingSchedule::ToConsume() const
{
   return mDuration - Consumed();
}

double RecordingSchedule::Consumed() const
{
   return std::max( 0.0, mPosition + TotalCorrection() );
}

double RecordingSchedule::ToDiscard() const
{
   return std::max(0.0, -( mPosition + TotalCorrection() ) );
}

void PlaybackSchedule::TimeQueue::Clear()
{
   mData = Records{};
   mHead = {};
   mTail = {};
}

void PlaybackSchedule::TimeQueue::Resize(size_t size)
{
   mData.resize(size);
}

#include "AudioIOExt.h"

void PlaybackSchedule::TimeQueue::Producer(
   PlaybackSchedule &schedule, PlaybackSlice slice, const AudioIOExts &exts)
{
   auto &policy = schedule.GetPolicy();

   if ( mData.empty() )
      // Recording only.  Don't fill the queue.
      return;

   // Don't check available space:  assume it is enough because of coordination
   // with RingBuffer.
   auto index = mTail.mIndex;
   auto time = mLastTime;
   auto remainder = mTail.mRemainder;
   auto space = TimeQueueGrainSize - remainder;
   const auto size = mData.size();

   // Produce advancing times
   auto frames = slice.toProduce;
   while ( frames >= space ) {
      auto times = policy.AdvancedTrackTime( schedule, time, space );
      time = times.second;
      if (!std::isfinite(time))
         time = times.first;
      ProduceExt(exts, times, space);
      index = (index + 1) % size;
      mData[ index ].timeValue = time;
      frames -= space;
      remainder = 0;
      space = TimeQueueGrainSize;
   }
   // Last odd lot
   if ( frames > 0 ) {
      auto times = policy.AdvancedTrackTime( schedule, time, frames );
      time = times.second;
      if (!std::isfinite(time))
         time = times.first;
      ProduceExt(exts, times, frames);
      remainder += frames;
      space -= frames;
   }

   // Produce constant times if there is also some silence in the slice
   frames = slice.frames - slice.toProduce;
   while ( frames >= space ) {
      ProduceExt(exts, {time, time}, space);
      index = (index + 1) % size;
      mData[ index ].timeValue = time;
      frames -= space;
      remainder = 0;
      space = TimeQueueGrainSize;
   }
   ProduceExt(exts, {time, time}, frames);

   mLastTime = time;
   mTail.mRemainder = remainder + frames;
   mTail.mIndex = index;
}

double PlaybackSchedule::TimeQueue::Consumer(
   size_t nSamples, double rate, unsigned long pauseFrames, bool hasSolo,
   const AudioIOExts &exts )
{
   if ( mData.empty() ) {
      // Recording only.  No scrub or playback time warp.  Don't use the queue.
      return ( mLastTime += nSamples / rate );
   }

   // Don't check available space:  assume it is enough because of coordination
   // with RingBuffer.
   auto remainder = mHead.mRemainder;
   auto space = TimeQueueGrainSize - remainder;
   const auto size = mData.size();
   while ( nSamples >= space ) {
      if (space)
         std::for_each(exts.begin(), exts.end(), [&](auto &pExt){
            pExt->Consumer(space, rate, pauseFrames, hasSolo); });

      mHead.mIndex = (mHead.mIndex + 1) % size;
      nSamples -= space;
      remainder = 0;
      space = TimeQueueGrainSize;
   }

   if (nSamples)
      std::for_each(exts.begin(), exts.end(), [&](auto &pExt){
         pExt->Consumer(nSamples, rate, pauseFrames, hasSolo); });
   mHead.mRemainder = remainder + nSamples;
   return mData[ mHead.mIndex ].timeValue;
}

void PlaybackSchedule::TimeQueue::Prime(
   double timeValue, const AudioIOExts &exts)
{
   mHead = {};
   mTail = {};
   mLastTime = timeValue;
   if (!mData.empty())
      mData[0].timeValue = timeValue;
   std::for_each(exts.begin(), exts.end(), [&](auto &pExt){
      pExt->Prime(timeValue); });
}

void PlaybackSchedule::TimeQueue::ProduceExt(const AudioIOExts &exts,
   std::pair<double, double> newTrackTimes, size_t nFrames)
{
   std::for_each(exts.begin(), exts.end(), [&](auto &pExt){
      pExt->Producer(newTrackTimes, nFrames); });
}

#include "ViewInfo.h"
void PlaybackSchedule::MessageProducer( SelectedRegionEvent &evt)
{
   // This executes in the main thread and is a producer of messages for
   // the AudioThread
   auto *pRegion = evt.pRegion.get();
   if (!pRegion)
      return;
   const SelectedRegion &region = *pRegion;

   mMessageChannel.Write( { region.t0(), region.t1() } );
}

void PlaybackSchedule::MessageConsumer()
{
   // This executes in the AudioThread and is the consumer

   auto data = mMessageChannel.Read();
   // TODO use data
}
