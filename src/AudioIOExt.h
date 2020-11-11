/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file AudioIOExt.h
 @brief Abstract base class for hooks into audio playback procedures
 
 Paul Licameli
 
 **********************************************************************/

#ifndef __AUDACITY_AUDIO_IO_EXT__
#define __AUDACITY_AUDIO_IO_EXT__

#include <functional>
#include <memory>
#include <vector>

#include <wx/string.h>

struct PaStreamCallbackTimeInfo;
struct PaStreamInfo;
struct PlaybackSchedule;
struct TransportTracks;

struct AudioIODiagnostics{
   wxString filename;    // For crash report bundle
   wxString text;        // One big string, may be localized
   wxString description; // Non-localized short description
};

class AUDACITY_DLL_API AudioIOExt
{
public:
   using Factory = std::function<std::unique_ptr<AudioIOExt>(
      const PlaybackSchedule&)>;
   using Factories = std::vector<AudioIOExt::Factory>;
   static Factories &GetFactories();

   //! Typically statically constructed
   struct AUDACITY_DLL_API RegisteredFactory{
      RegisteredFactory(Factory factory);
      ~RegisteredFactory();
   };

   virtual ~AudioIOExt();

   // Formerly in AudioIOBase
   virtual bool IsOtherStreamActive() const = 0;

   // Formerly in AudioIoCallback
   virtual void ComputeOtherTimings(double rate,
      const PaStreamCallbackTimeInfo *timeInfo,
      unsigned long framesPerBuffer) = 0;
   virtual void SignalOtherCompletion() = 0;
   virtual unsigned CountOtherSoloTracks() const = 0;

   // Formerly in AudioIO
   virtual bool StartOtherStream(const TransportTracks &tracks,
      const PaStreamInfo* info, double startTime, double rate) = 0;
   virtual void AbortOtherStream() = 0;
   virtual void FillOtherBuffers(
      double rate, unsigned long pauseFrames, bool paused, bool hasSolo) = 0;
   virtual void StopOtherStream() = 0;

   virtual AudioIODiagnostics Dump() const = 0;
};

#endif
