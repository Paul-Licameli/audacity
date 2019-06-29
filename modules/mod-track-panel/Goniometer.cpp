#include "Goniometer.h"

#include "AColor.h"
#include "Track.h"
#include "TrackPanelDrawingContext.h"

#include <wx/dc.h>

// Display parameters, perhaps to be varied with preferences later
static constexpr double persistence = 1.0; // seconds
enum : size_t { nPoints = 100 };

namespace{
AttachedTrackObjects::RegisteredFactory sKey{
   []( Track &track ) {
      return std::make_shared<Goniometer>( track );
   }
};
}

Goniometer &Goniometer::Get( Track &track )
{
   return track.AttachedObjects::Get< Goniometer >( sKey );
}

Goniometer::Goniometer( Track &track )
   : mpTrack{ track.shared_from_this() }
{
}

Goniometer::~Goniometer() = default;

void Goniometer::Clear()
{
   mRecentSamples.clear();
   mLastSample = 0;
   mSampleCount = 0;
   mSampleInterval = 0;
}

void Goniometer::Reset(double sampleRate, bool resetClipping)
{
   Clear();
   mRecentSamples.resize( 2 * nPoints, 0 );
   mSampleInterval = std::max< size_t >( 1,
      ( persistence * sampleRate ) / nPoints );
}

void Goniometer::UpdateDisplay(unsigned numChannels,
                   int numFrames, const float *sampleData)
{
   if ( numChannels != 2 ) {
      wxASSERT( false );
      return;
   }
   // Copy some of the samples into a circular buffer
   size_t index = mSampleInterval - mSampleCount;
   auto size = mRecentSamples.size();
   while( index < numFrames ) {
      while( index < numFrames && mLastSample < size ) {
         auto ii = 2 * index;
         mRecentSamples[ mLastSample++ ] = sampleData[ ii ];
         mRecentSamples[ mLastSample++ ] = sampleData[ ii + 1 ];
         index += mSampleInterval;
      }
      if ( mLastSample >= size )
         mLastSample = 0;
   }
   mSampleCount = index - numFrames;
}

bool Goniometer::IsMeterDisabled() const
{
   return false;
}

bool Goniometer::HasMaxPeak() const
{
   return false;
}

float Goniometer::GetMaxPeak() const
{
   return 0.0;
}

bool Goniometer::IsClipping() const
{
   return false;
}

int Goniometer::GetDBRange() const
{
   return 0;
}

std::shared_ptr<Track> Goniometer::DoFindTrack()
{
   return mpTrack.lock();
}

std::vector<UIHandlePtr> Goniometer::HitTest
   (const TrackPanelMouseState &,
    const AudacityProject *)
{
   return {};
}

void Goniometer::Reparent( const std::shared_ptr<Track> &parent )
{
   mpTrack = parent;
}
