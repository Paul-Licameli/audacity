/**********************************************************************

  Audacity: A Digital Audio Editor

  WaveClip.h

  ?? Dominic Mazzoni
  ?? Markus Meyer

*******************************************************************/

#ifndef __AUDACITY_WAVECLIP__
#define __AUDACITY_WAVECLIP__



#include "ClientData.h"
#include "SampleFormat.h"
#include "XMLTagHandler.h"

#include <wx/longlong.h>

#include <vector>
#include <functional>

class BlockArray;
class Envelope;
class ProgressDialog;
class sampleCount;
class SampleBlock;
class SampleBlockFactory;
using SampleBlockFactoryPtr = std::shared_ptr<SampleBlockFactory>;
class Sequence;
class wxFileNameWrapper;

class WaveClip;

// Array of pointers that assume ownership
using WaveClipHolder = std::shared_ptr< WaveClip >;
using WaveClipHolders = std::vector < WaveClipHolder >;
using WaveClipConstHolders = std::vector < std::shared_ptr< const WaveClip > >;

// A bundle of arrays needed for drawing waveforms.  The object may or may not
// own the storage for those arrays.  If it does, it destroys them.
class WaveDisplay
{
public:
   int width;
   sampleCount *where;
   float *min, *max, *rms;
   int* bl;

   std::vector<sampleCount> ownWhere;
   std::vector<float> ownMin, ownMax, ownRms;
   std::vector<int> ownBl;

public:
   WaveDisplay(int w)
      : width(w), where(0), min(0), max(0), rms(0), bl(0)
   {
   }

   // Create "own" arrays.
   void Allocate()
   {
      ownWhere.resize(width + 1);
      ownMin.resize(width);
      ownMax.resize(width);
      ownRms.resize(width);
      ownBl.resize(width);

      where = &ownWhere[0];
      if (width > 0) {
         min = &ownMin[0];
         max = &ownMax[0];
         rms = &ownRms[0];
         bl = &ownBl[0];
      }
      else {
         min = max = rms = 0;
         bl = 0;
      }
   }

   ~WaveDisplay()
   {
   }
};

struct AUDACITY_DLL_API WaveClipListener
{
   virtual ~WaveClipListener() = 0;
   virtual void MarkChanged() = 0;
   virtual void Invalidate() = 0;
};

class AUDACITY_DLL_API WaveClip final : public XMLTagHandler
   , public ClientData::Site< WaveClip, WaveClipListener >
{
private:
   // It is an error to copy a WaveClip without specifying the
   // sample block factory.

   WaveClip(const WaveClip&) PROHIBITED;
   WaveClip& operator= (const WaveClip&) PROHIBITED;

public:
   using Caches = Site< WaveClip, WaveClipListener >;

   // typical constructor
   WaveClip(const SampleBlockFactoryPtr &factory, sampleFormat format,
      int rate, int colourIndex);

   // essentially a copy constructor - but you must pass in the
   // current sample block factory, because we might be copying
   // from one project to another
   WaveClip(const WaveClip& orig,
            const SampleBlockFactoryPtr &factory,
            bool copyCutlines);

   // Copy only a range from the given WaveClip
   WaveClip(const WaveClip& orig,
            const SampleBlockFactoryPtr &factory,
            bool copyCutlines,
            double t0, double t1);

   virtual ~WaveClip();

   void ConvertToSampleFormat(sampleFormat format,
      const std::function<void(size_t)> & progressReport = {});

   // Always gives non-negative answer, not more than sample sequence length
   // even if t0 really falls outside that range
   void TimeToSamplesClip(double t0, sampleCount *s0) const;

   int GetRate() const { return mRate; }

   // Set rate without resampling. This will change the length of the clip
   void SetRate(int rate);

   // Resample clip. This also will set the rate, but without changing
   // the length of the clip
   void Resample(int rate, ProgressDialog *progress = NULL);

   void SetColourIndex( int index ){ mColourIndex = index;};
   int GetColourIndex( ) const { return mColourIndex;};
   void SetOffset(double offset);
   double GetOffset() const { return mOffset; }
   /*! @excsafety{No-fail} */
   void Offset(double delta)
      { SetOffset(GetOffset() + delta); }
   double GetStartTime() const;
   double GetEndTime() const;
   sampleCount GetStartSample() const;
   sampleCount GetEndSample() const;
   sampleCount GetNumSamples() const;

   // One and only one of the following is true for a given t (unless the clip
   // has zero length -- then BeforeClip() and AfterClip() can both be true).
   // Within() is true if the time is substantially within the clip
   bool WithinClip(double t) const;
   bool BeforeClip(double t) const;
   bool AfterClip(double t) const;
   bool IsClipStartAfterClip(double t) const;

   bool GetSamples(samplePtr buffer, sampleFormat format,
                   sampleCount start, size_t len, bool mayThrow = true) const;
   void SetSamples(constSamplePtr buffer, sampleFormat format,
                   sampleCount start, size_t len);

   Envelope* GetEnvelope() { return mEnvelope.get(); }
   const Envelope* GetEnvelope() const { return mEnvelope.get(); }
   BlockArray* GetSequenceBlockArray();
   const BlockArray* GetSequenceBlockArray() const;

   // Get low-level access to the sequence. Whenever possible, don't use this,
   // but use more high-level functions inside WaveClip (or add them if you
   // think they are useful for general use)
   Sequence* GetSequence() { return mSequence.get(); }
   const Sequence* GetSequence() const { return mSequence.get(); }

   /** WaveTrack calls this whenever data in the wave clip changes. It is
    * called automatically when WaveClip has a chance to know that something
    * has changed, like when member functions SetSamples() etc. are called. */
   /*! @excsafety{No-fail} */
   void MarkChanged();

   /** Getting high-level data for screen display and clipping
    * calculations and Contrast */
   std::pair<float, float> GetMinMax(
      double t0, double t1, bool mayThrow = true) const;
   float GetRMS(double t0, double t1, bool mayThrow = true) const;

   /** Whenever you do an operation to the sequence that will change the number
    * of samples (that is, the length of the clip), you will want to call this
    * function to tell the envelope about it. */
   void UpdateEnvelopeTrackLen();

   //! For use in importing pre-version-3 projects to preserve sharing of blocks
   std::shared_ptr<SampleBlock> AppendNewBlock(
      samplePtr buffer, sampleFormat format, size_t len);

   //! For use in importing pre-version-3 projects to preserve sharing of blocks
   void AppendSharedBlock(const std::shared_ptr<SampleBlock> &pBlock);

   /// You must call Flush after the last Append
   /// @return true if at least one complete block was created
   bool Append(constSamplePtr buffer, sampleFormat format,
               size_t len, unsigned int stride);
   /// Flush must be called after last Append
   void Flush();

   /// This name is consistent with WaveTrack::Clear. It performs a "Cut"
   /// operation (but without putting the cut audio to the clipboard)
   void Clear(double t0, double t1);

   /// Clear, and add cut line that starts at t0 and contains everything until t1.
   void ClearAndAddCutLine(double t0, double t1);

   /// Paste data from other clip, resampling it if not equal rate
   void Paste(double t0, const WaveClip* other);

   /** Insert silence - note that this is an efficient operation for large
    * amounts of silence */
   void InsertSilence( double t, double len, double *pEnvelopeValue = nullptr );

   /** Insert silence at the end, and causes the envelope to ramp
       linearly to the given value */
   void AppendSilence( double len, double envelopeValue );

   /// Get access to cut lines list
   WaveClipHolders &GetCutLines() { return mCutLines; }
   const WaveClipConstHolders &GetCutLines() const
      { return reinterpret_cast< const WaveClipConstHolders& >( mCutLines ); }
   size_t NumCutLines() const { return mCutLines.size(); }

   /** Find cut line at (approximately) this position. Returns true and fills
    * in cutLineStart and cutLineEnd (if specified) if a cut line at this
    * position could be found. Return false otherwise. */
   bool FindCutLine(double cutLinePosition,
                    double* cutLineStart = NULL,
                    double *cutLineEnd = NULL) const;

   /** Expand cut line (that is, re-insert audio, then DELETE audio saved in
    * cut line). Returns true if a cut line could be found and successfully
    * expanded, false otherwise */
   void ExpandCutLine(double cutLinePosition);

   /// Remove cut line, without expanding the audio in it
   bool RemoveCutLine(double cutLinePosition);

   /// Offset cutlines right to time 't0' by time amount 'len'
   void OffsetCutLines(double t0, double len);

   void CloseLock(); //should be called when the project closes.
   // not balanced by unlocking calls.

   //
   // XMLTagHandler callback methods for loading and saving
   //

   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   void HandleXMLEndTag(const wxChar *tag) override;
   XMLTagHandler *HandleXMLChild(const wxChar *tag) override;
   void WriteXML(XMLWriter &xmlFile) const /* not override */;

   // AWD, Oct 2009: for pasting whitespace at the end of selection
   bool GetIsPlaceholder() const { return mIsPlaceholder; }
   void SetIsPlaceholder(bool val) { mIsPlaceholder = val; }

   // used by commands which interact with clips using the keyboard
   bool SharesBoundaryWithNextClip(const WaveClip* next) const;

   const SampleBuffer &GetAppendBuffer() const { return mAppendBuffer; }
   size_t GetAppendBufferLen() const { return mAppendBufferLen; }

protected:
   double mOffset { 0 };
   int mRate;
   int mColourIndex;

   std::unique_ptr<Sequence> mSequence;
   std::unique_ptr<Envelope> mEnvelope;

   SampleBuffer  mAppendBuffer {};
   size_t        mAppendBufferLen { 0 };

   // Cut Lines are nothing more than ordinary wave clips, with the
   // offset relative to the start of the clip.
   WaveClipHolders mCutLines {};

   // AWD, Oct. 2009: for whitespace-at-end-of-selection pasting
   bool mIsPlaceholder { false };
};

//! TODO find a better place for this function
AUDACITY_DLL_API
void findCorrection(const std::vector<sampleCount> &oldWhere, size_t oldLen,
         size_t newLen,
         double t0, double rate, double samplesPerPixel,
         int &oldX0, double &correction);

//! TODO find a better place for this function
AUDACITY_DLL_API
void fillWhere(std::vector<sampleCount> &where,
   size_t len, double bias, double correction,
   double t0, double rate, double samplesPerPixel);

#endif
