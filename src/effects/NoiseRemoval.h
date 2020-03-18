/**********************************************************************

  Audacity: A Digital Audio Editor

  NoiseRemoval.h

  Dominic Mazzoni
  Vaughan Johnson (Preview)

**********************************************************************/

#ifndef __AUDACITY_EFFECT_NOISE_REMOVAL__
#define __AUDACITY_EFFECT_NOISE_REMOVAL__



#if 1//!defined(EXPERIMENTAL_NOISE_REDUCTION)

#include "Effect.h"
#include "EffectUI.h"

class wxButton;
class wxSizer;

class Envelope;
class WaveTrack;

#include "../RealFFTf.h"

class EffectNoiseRemoval final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectNoiseRemoval();
   virtual ~EffectNoiseRemoval();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   bool SupportsAutomation() override;

   // Effect implementation

   bool ShowInterface( wxWindow &parent,
      const EffectDialogFactory &factory, bool forceModal = false) override;
   bool Init() override;
   bool CheckWhetherSkipEffect() override;
   bool Process() override;
   void End() override;

private:

   bool      mDoProfile;
   bool      mHasProfile;
   int       mLevel;

   // Parameters chosen before the first phase
   double    mSampleRate;
   size_t    mWindowSize;
   size_t    mSpectrumSize;
   float     mMinSignalTime;    // in secs

   // The frequency-indexed noise threshold derived during the first
   // phase of analysis
   Floats mNoiseThreshold;  // length is mSpectrumSize

   // Parameters that affect the noise removal, regardless of how the
   // noise profile was extracted
   double     mSensitivity;
   double     mFreqSmoothingHz;
   double     mNoiseGain;              // in dB, should be negative
   double     mAttackDecayTime;        // in secs
   bool       mbLeaveNoise;

   bool ProcessOne(int count, WaveTrack * track,
                   sampleCount start, sampleCount len);

   void Initialize();
   void StartNewTrack();
   void ProcessSamples(size_t len, float *buffer);
   void FillFirstHistoryWindow();
   void ApplyFreqSmoothing(float *spec);
   void GetProfile();
   void RemoveNoise();
   void RotateHistoryWindows();
   void FinishTrack();

   // Variables that only exist during processing
   std::shared_ptr<WaveTrack> mOutputTrack;
   sampleCount       mInSampleCount;
   sampleCount       mOutSampleCount;
   int                   mInputPos;

   HFFT     hFFT;
   Floats mFFTBuffer;         // mWindowSize
   Floats mWindow;            // mWindowSize

   int       mFreqSmoothingBins;
   int       mAttackDecayBlocks;
   float     mOneBlockAttackDecay;
   float     mNoiseAttenFactor;
   float     mSensitivityFactor;
   size_t    mMinSignalBlocks;
   size_t    mHistoryLen;
   Floats mInWaveBuffer;     // mWindowSize
   Floats mOutOverlapBuffer; // mWindowSize
   ArraysOf<float> mSpectrums;        // mHistoryLen x mSpectrumSize
   ArraysOf<float> mGains;            // mHistoryLen x mSpectrumSize
   ArraysOf<float> mRealFFTs;         // mHistoryLen x mWindowSize
   ArraysOf<float> mImagFFTs;         // mHistoryLen x mWindowSize

   friend class NoiseRemovalDialog;
};

// WDR: class declarations

//----------------------------------------------------------------------------
// NoiseRemovalDialog
//----------------------------------------------------------------------------

// Declare window functions

class NoiseRemovalDialog final : public EffectDialog
{
public:
   // constructors and destructors
   NoiseRemovalDialog(EffectNoiseRemoval * effect,
                      wxWindow *parent);

   wxSizer *MakeNoiseRemovalDialog(bool call_fit = true,
                                   bool set_sizer = true);

   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataFromWindow() override;

private:
   // handlers
   void OnGetProfile();
   void OnPreview();
   void OnRemoveNoise();
   void OnCancel();

 public:

   EffectNoiseRemoval * m_pEffect;

   wxButton * m_pButton_GetProfile;

   double      mSensitivity;
   double      mGain;
   double      mFreq;
   double      mTime;

   int         mbLeaveNoise;
   bool        mbHasProfile;
   bool        mbAllowTwiddleSettings;
};

#endif

#endif
