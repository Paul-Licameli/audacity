/**********************************************************************

Audacity: A Digital Audio Editor

ProjectAudioIO.h

Paul Licameli split from AudacityProject.h

**********************************************************************/

#ifndef __PROJECT_AUDIO_IO__
#define __PROJECT_AUDIO_IO__

#include "Project.h"

#include <memory>
#include <vector>
class AudacityProject;
class Meter;

///\ brief Holds per-project state needed for interaction with AudioIO,
/// including the audio stream token and pointers to meters
class AUDIO_IO_API ProjectAudioIO final
   : public AttachedProjectObject
{
public:
   static ProjectAudioIO &Get( AudacityProject &project );
   static const ProjectAudioIO &Get( const AudacityProject &project );

   explicit ProjectAudioIO( AudacityProject &project );
   ProjectAudioIO( const ProjectAudioIO & ) = delete;
   ProjectAudioIO &operator=( const ProjectAudioIO & ) = delete;
   ~ProjectAudioIO();

   int GetAudioIOToken() const;
   bool IsAudioActive() const;
   void SetAudioIOToken(int token);

   using MeterPtr = std::shared_ptr< Meter >;
   using Meters = std::vector< MeterPtr >;

   const Meters &GetPlaybackMeters() const;
   bool HasPlaybackMeter( Meter *pMeter ) const;
   void AddPlaybackMeter(const MeterPtr &playback);
   void RemovePlaybackMeter(Meter *playback);

   const Meters &GetCaptureMeters() const;
   bool HasCaptureMeter( Meter *pMeter ) const;
   void AddCaptureMeter(const MeterPtr &capture);
   void RemoveCaptureMeter(Meter *capture);

private:
   AudacityProject &mProject;

   // Project owned meters
   Meters mPlaybackMeters;
   Meters mCaptureMeters;

   int  mAudioIOToken{ -1 };
};

#endif
