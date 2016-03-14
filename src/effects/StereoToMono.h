/**********************************************************************

  Audacity: A Digital Audio Editor

  StereoToMono.h

  Lynn Allan

**********************************************************************/

#ifndef __AUDACITY_EFFECT_STEREO_TO_MONO__
#define __AUDACITY_EFFECT_STEREO_TO_MONO__

#include <wx/string.h>

#include "Effect.h"

#define STEREOTOMONO_PLUGIN_SYMBOL XO("Stereo To Mono")

class EffectStereoToMono final : public Effect
{
public:
   EffectStereoToMono();
   virtual ~EffectStereoToMono() NOEXCEPT;

   // IdentInterface implementation

   wxString GetSymbol() override;
   wxString GetDescription() override;

   // EffectIdentInterface implementation

   EffectType GetType() override;
   bool IsInteractive() override;

   // EffectClientInterface implementation

   int GetAudioInCount() override;
   int GetAudioOutCount() override;

   // Effect implementation

   bool Process() override;
   bool IsHidden() override;

private:
   // EffectStereoToMono implementation

   bool ProcessOne(int count);

private:
   sampleCount mStart;
   sampleCount mEnd;
   WaveTrack *mLeftTrack;
   WaveTrack *mRightTrack;
   WaveTrack *mOutTrack;
};

#endif

