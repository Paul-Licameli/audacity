/**********************************************************************

Audacity: A Digital Audio Editor

@file WaveTrackLocation.cpp
@brief implements WaveTrackLocationsCache

Paul Licameli -- split from WaveTrack.h

**********************************************************************/

#include "WaveTrackLocation.h"
#include "WaveTrack.h"
#include "WaveClip.h"

WaveTrackLocationsCache::~WaveTrackLocationsCache() = default;

auto WaveTrackLocationsCache::Clone() const -> PointerType
{
   return std::make_unique<WaveTrackLocationsCache>(*this);
}

void WaveTrackLocationsCache::Update( const WaveTrack &track )
{
   auto clips = track.SortedClipArray();

   mDisplayLocationsCache.clear();

   // Count number of display locations
   int num = 0;
   {
      const WaveClip *prev = nullptr;
      for (const auto clip : clips)
      {
         num += clip->NumCutLines();

         if (prev && fabs(prev->GetEndTime() -
                          clip->GetStartTime()) < WAVETRACK_MERGE_POINT_TOLERANCE)
            ++num;

         prev = clip;
      }
   }

   if (num == 0)
      return;

   // Alloc necessary number of display locations
   mDisplayLocationsCache.reserve(num);

   // Add all display locations to cache
   int curpos = 0;

   const WaveClip *previousClip = nullptr;
   for (const auto clip: clips)
   {
      for (const auto &cc : clip->GetCutLines())
      {
         // Add cut line expander point
         mDisplayLocationsCache.push_back(WaveTrackLocation{
            clip->GetOffset() + cc->GetOffset(),
            WaveTrackLocation::locationCutLine
         });
         curpos++;
      }

      if (previousClip)
      {
         if (fabs(previousClip->GetEndTime() - clip->GetStartTime())
                                          < WAVETRACK_MERGE_POINT_TOLERANCE)
         {
            // Add merge point
            mDisplayLocationsCache.push_back(WaveTrackLocation{
               previousClip->GetEndTime(),
               WaveTrackLocation::locationMergePoint,
               track.GetClipIndex(previousClip),
               track.GetClipIndex(clip)
            });
            curpos++;
         }
      }

      previousClip = clip;
   }

   wxASSERT(curpos == num);
}

static WaveTrack::Caches::RegisteredFactory sKey{ []( WaveTrack& ){
   return std::make_unique< WaveTrackLocationsCache >();
} };

WaveTrackLocationsCache &WaveTrackLocationsCache::Get( const WaveTrack &track )
{
   return const_cast< WaveTrack& >( track )
      .Caches::Get< WaveTrackLocationsCache >( sKey );
}

