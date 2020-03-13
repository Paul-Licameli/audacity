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
#include <wx/textctrl.h>

#include "../../lib-src/portmidi/pm_common/portmidi.h"

#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../widgets/AudacityMessageBox.h"

#define DEFAULT_SYNTH_LATENCY 5

enum {
   HostID = 10000,
   PlayID,
   RecordID,
   ChannelsID
};

BEGIN_EVENT_TABLE(MidiIOPrefs, PrefsPanel)
   EVT_CHOICE(HostID, MidiIOPrefs::OnHost)
//   EVT_CHOICE(RecordID, MidiIOPrefs::OnDevice)
END_EVENT_TABLE()

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
   // First any pre-processing for constructing the GUI.
   GetNamesAndLabels();

   // Get current setting for devices
   mPlayDevice = gPrefs->Read(L"/MidiIO/PlaybackDevice", L"");
#ifdef EXPERIMENTAL_MIDI_IN
   mRecordDevice = gPrefs->Read(L"/MidiIO/RecordingDevice", L"");
#endif
//   mRecordChannels = gPrefs->Read(L"/MidiIO/RecordChannels", 2L);
}

bool MidiIOPrefs::TransferDataToWindow()
{
   wxCommandEvent e;
   OnHost(e);
   return true;
}

/// Gets the lists of names and lists of labels which are
/// used in the choice controls.
/// The names are what the user sees in the wxChoice.
/// The corresponding labels are what gets stored.
void MidiIOPrefs::GetNamesAndLabels() {
   // Gather list of hosts.  Only added hosts that have devices attached.
   Pm_Terminate(); // close and open to refresh device lists
   Pm_Initialize();
   int nDevices = Pm_CountDevices();
   for (int i = 0; i < nDevices; i++) {
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      if (info->output || info->input) { //should always happen
         wxString name = wxSafeConvertMB2WX(info->interf);
         if (!make_iterator_range(mHostNames)
            .contains( Verbatim( name ) )) {
            mHostNames.push_back( Verbatim( name ) );
            mHostLabels.push_back(name);
         }
      }
   }

   if (nDevices == 0) {
      mHostNames.push_back(XO("No MIDI interfaces"));
      mHostLabels.push_back(L"No MIDI interfaces");
   }
}

static StringSetting MidiIOHost{ "/MidiIO/Host", L"" };

void MidiIOPrefs::PopulateOrExchange( ShuttleGui & S ) {

   S.SetBorder(2);
   S.StartScroller();

   /* i18n-hint Software interface to MIDI */
   S.StartStatic(XC("Interface", "MIDI"));
   {
      S.StartMultiColumn(2);
      {
         mHost =
         S
            .Id(HostID)
            /* i18n-hint: (noun) */
            .TieChoice( XXO("&Host:"),
               {
                  MidiIOHost,
                  { ByColumns, mHostNames, mHostLabels }
               } );

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
         mPlay =
         S
            .Id(PlayID)
            .AddChoice( XXO("&Device:"), {} );

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
         mRecord
         S
            .Id(RecordID);
            .AddChoice(XO("De&vice:"), {} );

         /*
         mChannels =
         S
            .Id(ChannelsID)
            .AddChoice( XO("&Channels:"), wxEmptyString, {} );
         */
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
#endif
   S.EndScroller();

}

void MidiIOPrefs::OnHost(wxCommandEvent & WXUNUSED(e))
{
   Identifier itemAtIndex;
   int index = mHost->GetCurrentSelection();
   if (index >= 0 && index < (int)mHostNames.size())
      itemAtIndex = mHostLabels[index];
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
         wxString device = wxString::Format(L"%s: %s",
                                            interf,
                                            name);
         if (info->output) {
            playnames.push_back(name);
            index = mPlay->Append(name, (void *) info);
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
//   OnDevice(e);
}

bool MidiIOPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   const PmDeviceInfo *info;

   info = (const PmDeviceInfo *) mPlay->GetClientData(mPlay->GetSelection());
   if (info) {
      gPrefs->Write(L"/MidiIO/PlaybackDevice",
                    wxString::Format(L"%s: %s",
                                     wxString(wxSafeConvertMB2WX(info->interf)),
                                     wxString(wxSafeConvertMB2WX(info->name))));
   }
#ifdef EXPERIMENTAL_MIDI_IN
   info = (const PmDeviceInfo *) mRecord->GetClientData(mRecord->GetSelection());
   if (info) {
      gPrefs->Write(L"/MidiIO/RecordingDevice",
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
