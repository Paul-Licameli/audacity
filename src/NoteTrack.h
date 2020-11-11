/**********************************************************************

  Audacity: A Digital Audio Editor

  NoteTrack.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_NOTETRACK__
#define __AUDACITY_NOTETRACK__





#include <utility>
#include "Track.h"

#if defined(USE_MIDI)

// define this switch to play MIDI during redisplay to sonify run times
// Note that if SONIFY is defined, the default MIDI device will be opened
// and may block normal MIDI playback.
//#define SONIFY 1

#ifdef SONIFY

#define SONFNS(name) \
   void Begin ## name(); \
   void End ## name();

SONFNS(NoteBackground)
SONFNS(NoteForeground)
SONFNS(Measures)
SONFNS(Serialize)
SONFNS(Unserialize)
SONFNS(ModifyState)
SONFNS(AutoSave)

#undef SONFNS

#endif

class wxDC;
class wxRect;

class Alg_seq;   // from "allegro.h"

using NoteTrackBase =
#ifdef EXPERIMENTAL_MIDI_OUT
   PlayableTrack
#else
   AudioTrack
#endif
   ;

using QuantizedTimeAndBeat = std::pair< double, double >;

class StretchHandle;
class TimeWarper;

class AUDACITY_DLL_API NoteTrack final
   : public NoteTrackBase
{
public:
   NoteTrack();
   virtual ~NoteTrack();

   using Holder = std::shared_ptr<NoteTrack>;
   
private:
   Track::Holder Clone() const override;

public:
   double GetOffset() const override;
   double GetStartTime() const override;
   double GetEndTime() const override;

   Alg_seq &GetSeq() const;

   void WarpAndTransposeNotes(double t0, double t1,
                              const TimeWarper &warper, double semitones);

   void SetSequence(std::unique_ptr<Alg_seq> &&seq);
   void PrintSequence();

   Alg_seq *MakeExportableSeq(std::unique_ptr<Alg_seq> &cleanup) const;
   bool ExportMIDI(const wxString &f) const;
   bool ExportAllegro(const wxString &f) const;

   // High-level editing
   Track::Holder Cut  (double t0, double t1) override;
   Track::Holder Copy (double t0, double t1, bool forClipboard = true) const override;
   bool Trim (double t0, double t1) /* not override */;
   void Clear(double t0, double t1) override;
   void Paste(double t, const Track *src) override;
   void Silence(double t0, double t1) override;
   void InsertSilence(double t, double len) override;
   bool Shift(double t) /* not override */;

#ifdef EXPERIMENTAL_MIDI_OUT
   float GetVelocity() const { return mVelocity; }
   void SetVelocity(float velocity);
#endif

   QuantizedTimeAndBeat NearestBeatTime( double time ) const;
   bool StretchRegion
      ( QuantizedTimeAndBeat t0, QuantizedTimeAndBeat t1, double newDur );

   /// Gets the current bottom note (a pitch)
   int GetBottomNote() const { return mBottomNote; }
   /// Gets the current top note (a pitch)
   int GetTopNote() const { return mTopNote; }
   /// Sets the bottom note (a pitch), making sure that it is never greater than the top note.
   void SetBottomNote(int note);
   /// Sets the top note (a pitch), making sure that it is never less than the bottom note.
   void SetTopNote(int note);
   /// Sets the top and bottom note (both pitches) automatically, swapping them if needed.
   void SetNoteRange(int note1, int note2) const;

   /// Zooms so that all notes are visible
   void ZoomAllNotes();
   /// Zooms so that the entire track is visible
   void ZoomMaxExtent() { SetNoteRange(MinPitch, MaxPitch); }
   /// Shifts all notes vertically by the given pitch
   void ShiftNoteRange(int offset);

#if 0
   // Vertical scrolling is performed by dragging the keyboard at
   // left of track. Protocol is call StartVScroll, then update by
   // calling VScroll with original and final mouse position.
   // These functions are not used -- instead, zooming/dragging works like
   // audio track zooming/dragging. The vertical scrolling is nice however,
   // so I left these functions here for possible use in the future.
   void StartVScroll();
   void VScroll(int start, int end);
#endif

   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   XMLTagHandler *HandleXMLChild(const wxChar *tag) override;
   void WriteXML(XMLWriter &xmlFile) const override;

   // channels are numbered as integers 0-15, visible channels
   // (mVisibleChannels) is a bit set. Channels are displayed as
   // integers 1-16.

   // Allegro's data structure does not restrict channels to 16.
   // Since there is not way to select more than 16 channels,
   // map all channel numbers mod 16. This will have no effect
   // on MIDI files, but it will allow users to at least select
   // all channels on non-MIDI event sequence data.
#define NUM_CHANNELS 16
   // Bitmask with all NUM_CHANNELS bits set
#define ALL_CHANNELS (1 << NUM_CHANNELS) - 1
#define CHANNEL_BIT(c) (1 << (c % NUM_CHANNELS))
   bool IsVisibleChan(int c) const {
      return (mVisibleChannels & CHANNEL_BIT(c)) != 0;
   }
   void SetVisibleChan(int c) { mVisibleChannels |= CHANNEL_BIT(c); }
   void ClearVisibleChan(int c) { mVisibleChannels &= ~CHANNEL_BIT(c); }
   void ToggleVisibleChan(int c) { mVisibleChannels ^= CHANNEL_BIT(c); }
   // Solos the given channel.  If it's the only channel visible, all channels
   // are enabled; otherwise, it is set to the only visible channel.
   void SoloVisibleChan(int c) {
      if (mVisibleChannels == CHANNEL_BIT(c))
         mVisibleChannels = ALL_CHANNELS;
      else
         mVisibleChannels = CHANNEL_BIT(c);
   }

   const TypeInfo &GetTypeInfo() const override;
   static const TypeInfo &ClassTypeInfo();

   Track::Holder PasteInto( AudacityProject & ) const override;

   ConstIntervals GetIntervals() const override;
   Intervals GetIntervals() override;

 private:

   void AddToDuration( double delta );

   // These are mutable to allow NoteTrack to switch details of representation
   // in logically const methods
   // At most one of the two pointers is not null at any time.
   // Both are null in a newly constructed NoteTrack.
   mutable std::unique_ptr<Alg_seq> mSeq;
   mutable std::unique_ptr<char[]> mSerializationBuffer;
   mutable long mSerializationLength;

#ifdef EXPERIMENTAL_MIDI_OUT
   float mVelocity; // velocity offset
#endif

   mutable int mBottomNote, mTopNote;
#if 0
   // Also unused from vertical scrolling
   int mStartBottomNote;
#endif

   // Remember continuous variation for zooming,
   // but it is rounded off whenever drawing:
   float mPitchHeight;

   enum { MinPitch = 0, MaxPitch = 127 };

   int mVisibleChannels; // bit set of visible channels

   std::weak_ptr<StretchHandle> mStretchHandle;
};

ENUMERATE_TRACK_TYPE(NoteTrack);

#endif // USE_MIDI

#ifndef SONIFY
// no-ops:
#define SonifyBeginSonification()
#define SonifyEndSonification()
#define SonifyBeginNoteBackground()
#define SonifyEndNoteBackground()
#define SonifyBeginNoteForeground()
#define SonifyEndNoteForeground()
#define SonifyBeginMeasures()
#define SonifyEndMeasures()
#define SonifyBeginSerialize()
#define SonifyEndSerialize()
#define SonifyBeginUnserialize()
#define SonifyEndUnserialize()
#define SonifyBeginAutoSave()
#define SonifyEndAutoSave()
#define SonifyBeginModifyState()
#define SonifyEndModifyState()
#endif


#include "Prefs.h"
extern AUDACITY_DLL_API EnumSetting< bool > AllegroStyleSetting;

AUDACITY_DLL_API wxString GetMIDIDeviceInfo();

#endif
