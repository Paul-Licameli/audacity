/**********************************************************************

Audacity: A Digital Audio Editor

Meter.h

Paul Licameli split from MeterPanelBase.h

**********************************************************************/

#ifndef __AUDACITY_METER__
#define __AUDACITY_METER__

/// \brief an interface for AudioIO to communicate buffers of samples for
/// real-time display updates
class AUDIO_DEVICES_API Meter /* not final */
{
public:
   virtual ~Meter();

   virtual void Clear() = 0;
   virtual void Reset(double sampleRate, bool resetClipping) = 0;
   virtual void UpdateDisplay(unsigned numChannels,
                      int numFrames, const float *sampleData) = 0;
   virtual bool IsMeterDisabled() const = 0;
   virtual bool HasMaxPeak() const = 0;
   virtual float GetMaxPeak() const = 0;
   virtual bool IsClipping() const = 0;
   virtual int GetDBRange() const = 0;
};

#endif
