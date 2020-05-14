/**********************************************************************

  Audacity: A Digital Audio Editor

  MidiIOPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni
  James Crook

*******************************************************************//**

\class MidiIOPrefs
\brief A PrefsPanel used to select recording and playback devices and
other settings.

  Presents interface for user to select the recording device and
  playback device, from the list of choices that PortMidi
  makes available.

  Also lets user decide whether or not to record in stereo, and
  whether or not to play other tracks while recording one (duplex).

*//********************************************************************/


#include "MidiIOPrefs.h"

#include "AudioIOBase.h"

#ifdef EXPERIMENTAL_MIDI_OUT

#include <wx/defs.h>

#include <wx/choice.h>

#include "../../lib-src/portmidi/pm_common/portmidi.h"

#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../widgets/AudacityMessageBox.h"

#define DEFAULT_SYNTH_LATENCY 5

enum {
   PlayID = 10000,
   ChannelsID
};

MidiIOPrefs::MidiIOPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: untranslatable acronym for "Musical Instrument Device Interface" */
:  PrefsPanel(parent, winid, XO("MIDI Devices"))
{
   Populate();
}

MidiIOPrefs::~MidiIOPrefs()
{
}

ComponentInterfaceSymbol MidiIOPrefs::GetSymbol()
{
   return MIDI_IO_PREFS_PLUGIN_SYMBOL;
}

TranslatableString MidiIOPrefs::GetDescription()
{
   return XO("Preferences for MidiIO");
}

wxString MidiIOPrefs::HelpPageName()
{
   return "MIDI_Devices_Preferences";
}

void MidiIOPrefs::Populate()
{
   // Get current setting for devices
   mPlayDevice = MidiIOPlaybackDevice.Read();
#ifdef EXPERIMENTAL_MIDI_IN
   mRecordDevice = MidiIORecordingDevice.Read();
#endif
//   mRecordChannels = MidiIORecordChannels.Read();
}

bool MidiIOPrefs::TransferDataToWindow()
{
   OnHost();
   return true;
}

static StringSetting MidiIOHost{ "/MidiIO/Host", L"" };

void MidiIOPrefs::PopulateOrExchange( ShuttleGui & S ) {
   using namespace DialogDefinition;

   Identifiers hostLabels;

   // Gather list of hosts.  Only added hosts that have devices attached.
   Pm_Terminate(); // close and open to refresh device lists
   Pm_Initialize();
   int nDevices = Pm_CountDevices();
   for (int i = 0; i < nDevices; i++) {
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      if (info->output || info->input) { //should always happen
         wxString name = wxSafeConvertMB2WX(info->interf);
         if (!make_iterator_range(hostLabels)
            .contains( name ))
            hostLabels.push_back(name);
      }
   }

   if (nDevices == 0)
      hostLabels.push_back(XO("No MIDI interfaces").Translation()); //?

   S.StartScroller(0, 2);
   
   /* i18n-hint Software interface to MIDI */
   S.StartStatic(XC("Interface", "MIDI"));
   {
      S.StartMultiColumn(2);
      {
         S
            .Target( Choice( MidiIOHost, Verbatim( hostLabels ) ) )
            .Action( [this]{ OnHost(); } )
            /* i18n-hint: (noun) */
            .AddChoice( XXO("&Host:") )
            .Assign(mHost);

         S
            .AddPrompt(XXO("Using: PortMidi"));
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S
      .StartStatic(XO("Playback"));
   {
      S.StartMultiColumn(2);
      {
         S
            .Id(PlayID)
            .AddChoice( XXO("&Device:"), {} )
            .Assign(mPlay);

         S
            .Target( MidiIOSynthLatency )
            .AddTextBox(XXO("MIDI Synth L&atency (ms):"), {}, 3);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

#ifdef EXPERIMENTAL_MIDI_IN
   S
      .StartStatic(XO("Recording"));
   {
      S.StartMultiColumn(2);
      {
         S
            // .Action( &My::OnDevice )
            .AddChoice(XO("De&vice:") );
            .Assign(mRecord);

         /*
         S
            .Id(ChannelsID)
            .AddChoice( XO("&Channels:"), wxEmptyString, {} )
            .Assign(mChannels);
         */
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
#endif
   S.EndScroller();

}

void MidiIOPrefs::OnHost()
{
   Identifier itemAtIndex = MidiIOHost.Read();
   int nDevices = Pm_CountDevices();

   mPlay->Clear();
#ifdef EXPERIMENTAL_MIDI_IN
   mRecord->Clear();
#endif

   wxArrayStringEx playnames;
   wxArrayStringEx recordnames;

   for (int i = 0; i < nDevices; i++) {
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      wxString interf = wxSafeConvertMB2WX(info->interf);
      if (itemAtIndex == interf) {
         wxString name = wxSafeConvertMB2WX(info->name);
         auto device = wxString::Format(L"%s: %s",
                                            interf,
                                            name);
         if (info->output) {
            playnames.push_back(name);
            auto index = mPlay->Append(name, (void *) info);
            if (device == mPlayDevice) {
               mPlay->SetSelection(index);
            }
         }
#ifdef EXPERIMENTAL_MIDI_IN
         if (info->input) {
            recordnames.push_back(name);
            index = mRecord->Append(name, (void *) info);
            if (device == mRecordDevice) {
               mRecord->SetSelection(index);
            }
         }
#endif
      }
   }

   if (mPlay->GetCount() == 0) {
      playnames.push_back(_("No devices found"));
      mPlay->Append(playnames[0], (void *) NULL);
   }
#ifdef EXPERIMENTAL_MIDI_IN
   if (mRecord->GetCount() == 0) {
      recordnames.push_back(_("No devices found"));
      mRecord->Append(recordnames[0], (void *) NULL);
   }
#endif
   if (mPlay->GetCount() && mPlay->GetSelection() == wxNOT_FOUND) {
      mPlay->SetSelection(0);
   }
#ifdef EXPERIMENTAL_MIDI_IN
   if (mRecord->GetCount() && mRecord->GetSelection() == wxNOT_FOUND) {
      mRecord->SetSelection(0);
   }
#endif
   ShuttleGui::SetMinSize(mPlay, playnames);
#ifdef EXPERIMENTAL_MIDI_IN
   ShuttleGui::SetMinSize(mRecord, recordnames);
#endif
}

bool MidiIOPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();

   const PmDeviceInfo *info;

   info = (const PmDeviceInfo *) mPlay->GetClientData(mPlay->GetSelection());
   if (info) {
      MidiIOPlaybackDevice.Write(
                    wxString::Format(L"%s: %s",
                                     wxString(wxSafeConvertMB2WX(info->interf)),
                                     wxString(wxSafeConvertMB2WX(info->name))));
   }
#ifdef EXPERIMENTAL_MIDI_IN
   info = (const PmDeviceInfo *) mRecord->GetClientData(mRecord->GetSelection());
   if (info) {
      MidiIORecordingDevice.Write(
                    wxString::Format(L"%s: %s",
                                     wxString(wxSafeConvertMB2WX(info->interf)),
                                     wxString(wxSafeConvertMB2WX(info->name))));
   }
#endif
   return gPrefs->Flush();
}

bool MidiIOPrefs::Validate()
{
   return true;
}

#ifdef EXPERIMENTAL_MIDI_OUT
namespace{
PrefsPanel::Registration sAttachment{ "MidiIO",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew MidiIOPrefs(parent, winid);
   },
   false,
   // Register with an explicit ordering hint because this one is
   // only conditionally compiled
   { "", { Registry::OrderingHint::After, "Recording" } }
};
}
#endif

#endif
