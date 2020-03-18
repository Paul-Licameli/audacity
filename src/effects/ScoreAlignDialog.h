/**********************************************************************

  Audacity: A Digital Audio Editor

  ScoreAlignDialog.h

**********************************************************************/

#ifndef __AUDACITY_SCORE_ALIGN_DIALOG__
#define __AUDACITY_SCORE_ALIGN_DIALOG__

#ifdef EXPERIMENTAL_SCOREALIGN

#include "../widgets/wxPanelWrapper.h"

#if 1

#include "ScoreAlignParams.h"

#else

// Stub definitions

#include "../Internat.h"

struct ScoreAlignParams
{
   int mStatus;
   double mMidiStart, mMidiEnd;
   double mAudioStart, mAudioEnd;
   double mFramePeriod;
   double mWindowSize;
   double mSilenceThreshold;
   bool mForceFinalAlignment;
   bool mIgnoreSilence;
   double mPresmoothTime;
   double mLineTime;
   double mSmoothTime;
};
class SAProgress;
class Alg_seq;

extern int scorealign(
   void *data,
   long (*process)(void *data, float **buffer, long n),
   unsigned channels,
   double rate,
   double endTime,
   Alg_seq *seq,
   SAProgress *progress,
   ScoreAlignParams params
);

constexpr float
     SA_DFT_FRAME_PERIOD = 0
   , SA_DFT_WINDOW_SIZE = 0
   , SA_DFT_SILENCE_THRESHOLD = 0
   , SA_DFT_PRESMOOTH_TIME = 0
   , SA_DFT_LINE_TIME = 0
   , SA_DFT_SMOOTH_TIME = 0
;

constexpr bool
     SA_DFT_FORCE_FINAL_ALIGNMENT = false
   , SA_DFT_IGNORE_SILENCE = false
;

static const TranslatableString
     SA_DFT_FRAME_PERIOD_TEXT
   , SA_DFT_WINDOW_SIZE_TEXT
   , SA_DFT_SILENCE_THRESHOLD_TEXT
   , SA_DFT_PRESMOOTH_TIME_TEXT
   , SA_DFT_LINE_TIME_TEXT
   , SA_DFT_SMOOTH_TIME_TEXT
;

#endif

void CloseScoreAlignDialog();

//----------------------------------------------------------------------------
// ScoreAlignDialog
//----------------------------------------------------------------------------

// Declare window functions

class ScoreAlignDialog final : public wxDialogWrapper
{
public:
   ScoreAlignParams p;

   // constructors and destructors
   ScoreAlignDialog(ScoreAlignParams &params);
   ~ScoreAlignDialog();

private:
   enum {
     ID_BASE = 10000,
     ID_PRESMOOTH,
     ID_WINDOWSIZE,
     ID_LINETIME,
     ID_SMOOTHTIME,
     ID_SILENCETHRESHOLD,
     ID_DEFAULT
   };

   // handlers
   void OnOK();
   void OnCancel();
   void OnDefault();
};

#endif

#endif
