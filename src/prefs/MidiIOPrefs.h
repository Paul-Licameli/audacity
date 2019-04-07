/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIOPrefs.h

  Joshua Haberman
  James Crook

**********************************************************************/

class wxChoice;
class wxTextCtrl;
class ShuttleGui;

#ifdef EXPERIMENTAL_MIDI_OUT

#ifndef __AUDACITY_MIDI_IO_PREFS__
#define __AUDACITY_MIDI_IO_PREFS__

#include <wx/defs.h>

#include "PrefsPanel.h"

#define MIDI_IO_PREFS_PLUGIN_SYMBOL ComponentInterfaceSymbol{ XO("Midi IO") }

class MidiIOPrefs final : public PrefsPanel
{
 public:
   MidiIOPrefs(wxWindow * parent, wxWindowID winid);
   virtual ~MidiIOPrefs();
   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;

   bool Commit() override;
   bool Validate() override;
   wxString HelpPageName() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;

 private:
   void Populate();

   void OnHost();

   wxString mPlayDevice;
#ifdef EXPERIMENTAL_MIDI_IN
   wxString mRecordDevice;
#endif
//   long mRecordChannels;

   wxChoice *mHost;
   wxChoice *mPlay;
#ifdef EXPERIMENTAL_MIDI_IN
   wxChoice *mRecord;
#endif
//   wxChoice *mChannels;
};

extern StringSetting MidiIOPlaybackDevice;
extern StringSetting MidiIORecordingDevice;
extern IntSetting MidiIOSynthLatency; // milliseconds

#endif

#endif
