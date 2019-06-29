#ifndef __AUDACITY_GONIOMETER__
#define __AUDACITY_GONIOMETER__

#include <memory>
#include "TrackAttachment.h" // to inherit
#include "CommonTrackPanelCell.h" // to inherit
#include "Meter.h" // to inherit

class Track;

class Goniometer
   : public std::enable_shared_from_this< Goniometer >
   , public TrackAttachment
   , public CommonTrackPanelCell
   , public Meter
{
public:
   static Goniometer &Get( Track &track );

   explicit Goniometer( Track &track );
   ~Goniometer() override;

   // Meter implementation
   void Clear() override;
   void Reset(double sampleRate, bool resetClipping) override;
   void UpdateDisplay(unsigned numChannels,
                      int numFrames, const float *sampleData) override;
   bool IsMeterDisabled() const override;
   bool HasMaxPeak() const override;
   float GetMaxPeak() const override;
   bool IsClipping() const override;
   int GetDBRange() const override;

   // CommonTrackPanelCell implementation
   std::shared_ptr<Track> DoFindTrack() override;

   // TrackPanelCell implementation
   std::vector<UIHandlePtr> HitTest
      (const TrackPanelMouseState &state,
       const AudacityProject *pProject) override;

   // TrackAttachment implementation
   void Reparent( const std::shared_ptr<Track> &parent ) override;

   // TrackPanelDrawable implementation
   void Draw(
      TrackPanelDrawingContext &context, const wxRect &rect, unsigned iPass )
      override;

private:
   std::vector<float> mRecentSamples;
   size_t mLastSample{ 0 };
   size_t mSampleCount{ 0 };
   size_t mSampleInterval{ 0 };

   std::weak_ptr<Track> mpTrack;
};

#endif
