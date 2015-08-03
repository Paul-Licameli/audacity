/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "WaveTrackControls.h"
#include "WaveTrackButtonHandles.h"
#include "WaveTrackSliderHandles.h"

#include "../../../HitTestResult.h"
#include "../../../MixerBoard.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../WaveTrack.h"
#include "../../../ShuttleGui.h"
#include "../../../TrackPanel.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../widgets/PopupMenuTable.h"
#include "../../../ondemand/ODManager.h"
#include "../../../prefs/PrefsDialog.h"
#include "../../../prefs/SpectrumPrefs.h"
#include "../../../prefs/WaveformPrefs.h"

#include <wx/combobox.h>

namespace
{
   /// Puts a check mark at a given position in a menu.
   void SetMenuCheck(wxMenu & menu, int newId)
   {
      wxMenuItemList & list = menu.GetMenuItems();
      wxMenuItem * item;
      int id;

      for (wxMenuItemList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext())
      {
         item = node->GetData();
         id = item->GetId();
         // We only need to set check marks. Clearing checks causes problems on Linux (bug 851)
         if (id == newId)
            menu.Check(id, true);
      }
   }
}

WaveTrackControls::WaveTrackControls()
{
}

WaveTrackControls &WaveTrackControls::Instance()
{
   static WaveTrackControls instance;
   return instance;
}

WaveTrackControls::~WaveTrackControls()
{
}


HitTestResult WaveTrackControls::HitTest
(const TrackPanelMouseEvent & evt,
 const AudacityProject *pProject)
{
   {
      HitTestResult result = TrackControls::HitTest1(evt, pProject);
      if (result.handle)
         return result;
   }

   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   if (event.Button(wxMOUSE_BTN_LEFT)) {
      // VJ: Check sync-lock icon and the blank area to the left of the minimize
      // button.
      // Have to do it here, because if track is shrunk such that these areas
      // occlude controls,
      // e.g., mute/solo, don't want positive hit tests on the buttons.
      // Only result of doing so is to select the track. Don't care whether isleft.
      const bool bTrackSelClick =
         TrackInfo::TrackSelFunc(GetTrack(), rect, event.m_x, event.m_y);

      if (!bTrackSelClick) {
         if (mpTrack->GetKind() == Track::Wave) {
            HitTestResult result;
            if (NULL != (result = MuteButtonHandle::HitTest(event, rect, pProject)).handle)
               return result;

            if (NULL != (result = SoloButtonHandle::HitTest(event, rect, pProject)).handle)
               return result;

            if (NULL != (result =
               GainSliderHandle::HitTest(event, rect, pProject, mpTrack)).handle)
               return result;

            if (NULL != (result =
               PanSliderHandle::HitTest(event, rect, pProject, mpTrack)).handle)
               return result;
         }
      }
   }

   return TrackControls::HitTest2(evt, pProject);
}

enum {
   OnRate8ID,              // <---
   OnRate11ID,             //    |
   OnRate16ID,             //    |
   OnRate22ID,             //    |
   OnRate44ID,             //    |
   OnRate48ID,             //    | Leave these in order
   OnRate88ID,             //    |
   OnRate96ID,             //    |
   OnRate176ID,            //    |
   OnRate192ID,            //    |
   OnRate352ID,            //    |
   OnRate384ID,            //    |
   OnRateOtherID,          //    |
   //    |
   On16BitID,              //    |
   On24BitID,              //    |
   OnFloatID,              // <---

   OnWaveformID,
   OnWaveformDBID,
   OnSpectrumID,
   OnSpectrogramSettingsID,

   OnChannelMonoID,
   OnChannelLeftID,
   OnChannelRightID,
   OnMergeStereoID,

   OnSwapChannelsID,
   OnSplitStereoID,
   OnSplitStereoMonoID,

   ChannelMenuID,
};

//=============================================================================
// Table class for a sub-menu
class FormatMenuTable : public PopupMenuTable
{
   FormatMenuTable() : mpData(NULL) {}
   DECLARE_POPUP_MENU(FormatMenuTable);

public:
   static FormatMenuTable &Instance();

private:
   virtual void InitMenu(Menu *pMenu, void *pUserData);

   virtual void DestroyMenu()
   {
      mpData = NULL;
   }

   TrackControls::InitMenuData *mpData;

   int IdOfFormat(int format);

   void OnFormatChange(wxCommandEvent & event);
};

FormatMenuTable &FormatMenuTable::Instance()
{
   static FormatMenuTable instance;
   return instance;
}

void FormatMenuTable::InitMenu(Menu *pMenu, void *pUserData)
{
   mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   SetMenuCheck(*pMenu, IdOfFormat(pTrack->GetSampleFormat()));

   AudacityProject *const project = ::GetActiveProject();
   bool unsafe = project->IsAudioActive();
   for (int i = On16BitID; i <= OnFloatID; i++) {
      pMenu->Enable(i, !unsafe);
   }
}

BEGIN_POPUP_MENU(FormatMenuTable)
   POPUP_MENU_RADIO_ITEM(On16BitID,
      GetSampleFormatStr(int16Sample), OnFormatChange)
   POPUP_MENU_RADIO_ITEM(On24BitID,
      GetSampleFormatStr(int24Sample), OnFormatChange)
   POPUP_MENU_RADIO_ITEM(OnFloatID,
      GetSampleFormatStr(floatSample), OnFormatChange)
END_POPUP_MENU()

/// Converts a format enumeration to a wxWidgets menu item Id.
int FormatMenuTable::IdOfFormat(int format)
{
   switch (format) {
   case int16Sample:
      return On16BitID;
   case int24Sample:
      return On24BitID;
   case floatSample:
      return OnFloatID;
   default:
      // ERROR -- should not happen
      wxASSERT(false);
      break;
   }
   return OnFloatID;// Compiler food.
}

/// Handles the selection from the Format submenu of the
/// track menu.
void FormatMenuTable::OnFormatChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= On16BitID && id <= OnFloatID);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack && pTrack->GetKind() == Track::Wave);

   sampleFormat newFormat = int16Sample;

   switch (id) {
   case On16BitID:
      newFormat = int16Sample;
      break;
   case On24BitID:
      newFormat = int24Sample;
      break;
   case OnFloatID:
      newFormat = floatSample;
      break;
   default:
      // ERROR -- should not happen
      wxASSERT(false);
      break;
   }
   if (newFormat == pTrack->GetSampleFormat())
      return; // Nothing to do.

   AudacityProject *const project = ::GetActiveProject();
   TrackList *const tracks = project->GetTracks();

   bool bResult = pTrack->ConvertToSampleFormat(newFormat);
   wxASSERT(bResult); // TO DO: Actually handle this.
   WaveTrack *const partner = static_cast<WaveTrack*>(tracks->GetLink(pTrack));
   if (partner)
   {
      bResult = partner->ConvertToSampleFormat(newFormat);
      wxASSERT(bResult); // TO DO: Actually handle this.
   }

   project->PushState(wxString::Format(_("Changed '%s' to %s"),
      pTrack->GetName().
      c_str(),
      GetSampleFormatStr(newFormat)),
      _("Format Change"));

//   SetMenuCheck(*mFormatMenu, id);

   using namespace RefreshCode;
   mpData->result = RefreshAll | FixScrollbars;
}


//=============================================================================
// Table class for a sub-menu
class RateMenuTable : public PopupMenuTable
{
   RateMenuTable() : mpData(NULL) {}
   DECLARE_POPUP_MENU(RateMenuTable);

public:
   static RateMenuTable &Instance();

private:
   virtual void InitMenu(Menu *pMenu, void *pUserData);

   virtual void DestroyMenu()
   {
      mpData = NULL;
   }

   TrackControls::InitMenuData *mpData;

   int IdOfRate(int rate);
   void SetRate(WaveTrack * pTrack, double rate);

   void OnRateChange(wxCommandEvent & event);
   void OnRateOther(wxCommandEvent & event);
};

RateMenuTable &RateMenuTable::Instance()
{
   static RateMenuTable instance;
   return instance;
}

void RateMenuTable::InitMenu(Menu *pMenu, void *pUserData)
{
   mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   SetMenuCheck(*pMenu, IdOfRate((int)pTrack->GetRate()));

   AudacityProject *const project = ::GetActiveProject();
   bool unsafe = project->IsAudioActive();
   for (int i = OnRate8ID; i <= OnRateOtherID; i++) {
      pMenu->Enable(i, !unsafe);
   }
}

BEGIN_POPUP_MENU(RateMenuTable)
   POPUP_MENU_RADIO_ITEM(OnRate8ID, _("8000 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate11ID, _("11025 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate16ID, _("16000 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate22ID, _("22050 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate44ID, _("44100 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate48ID, _("48000 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate88ID, _("88200 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate96ID, _("96000 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate176ID, _("176400 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate192ID, _("192000 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate352ID, _("352800 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRate384ID, _("384000 Hz"), OnRateChange)
   POPUP_MENU_RADIO_ITEM(OnRateOtherID, _("&Other..."), OnRateOther)
END_POPUP_MENU()

const int nRates = 12;

///  gRates MUST CORRESPOND DIRECTLY TO THE RATES AS LISTED IN THE MENU!!
///  IN THE SAME ORDER!!
static int gRates[nRates] = { 8000, 11025, 16000, 22050, 44100, 48000, 88200, 96000,
176400, 192000, 352800, 384000 };

/// Converts a sampling rate to a wxWidgets menu item id
int RateMenuTable::IdOfRate(int rate)
{
   for (int i = 0; i<nRates; i++) {
      if (gRates[i] == rate)
         return i + OnRate8ID;
   }
   return OnRateOtherID;
}

/// Sets the sample rate for a track, and if it is linked to
/// another track, that one as well.
void RateMenuTable::SetRate(WaveTrack * pTrack, double rate)
{
   AudacityProject *const project = ::GetActiveProject();
   TrackList *const tracks = project->GetTracks();
   pTrack->SetRate(rate);
   WaveTrack *const partner = static_cast<WaveTrack*>(tracks->GetLink(pTrack));
   if (partner)
      partner->SetRate(rate);
   // Separate conversion of "rate" enables changing the decimals without affecting i18n
   wxString rateString = wxString::Format(wxT("%.3f"), rate);
   project->PushState(wxString::Format(_("Changed '%s' to %s Hz"),
      pTrack->GetName().c_str(), rateString.c_str()),
      _("Rate Change"));
}

/// This method handles the selection from the Rate
/// submenu of the track menu, except for "Other" (/see OnRateOther).
void RateMenuTable::OnRateChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnRate8ID && id <= OnRate384ID);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack->GetKind() == Track::Wave);

   //   SetMenuCheck(*mRateMenu, id);
   SetRate(pTrack, gRates[id - OnRate8ID]);

   using namespace RefreshCode;
   mpData->result = RefreshAll | FixScrollbars;
}

void RateMenuTable::OnRateOther(wxCommandEvent &)
{
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack && pTrack->GetKind() == Track::Wave);

   int newRate;

   /// \todo Remove artificial constants!!
   /// \todo Make a real dialog box out of this!!
   while (true)
   {
      wxDialog dlg(mpData->pParent, wxID_ANY, wxString(_("Set Rate")));
      dlg.SetName(dlg.GetTitle());
      ShuttleGui S(&dlg, eIsCreating);
      wxString rate;
      wxArrayString rates;
      wxComboBox *cb;

      rate.Printf(wxT("%ld"), lrint(pTrack->GetRate()));

      rates.Add(wxT("8000"));
      rates.Add(wxT("11025"));
      rates.Add(wxT("16000"));
      rates.Add(wxT("22050"));
      rates.Add(wxT("44100"));
      rates.Add(wxT("48000"));
      rates.Add(wxT("88200"));
      rates.Add(wxT("96000"));
      rates.Add(wxT("176400"));
      rates.Add(wxT("192000"));
      rates.Add(wxT("352800"));
      rates.Add(wxT("384000"));

      S.StartVerticalLay(true);
      {
         S.SetBorder(10);
         S.StartHorizontalLay(wxEXPAND, false);
         {
            cb = S.AddCombo(_("New sample rate (Hz):"),
               rate,
               &rates);
#if defined(__WXMAC__)
            // As of wxMac-2.8.12, setting manually is required
            // to handle rates not in the list.  See: Bug #427
            cb->SetValue(rate);
#endif
         }
         S.EndHorizontalLay();
         S.AddStandardButtons();
      }
      S.EndVerticalLay();

      dlg.SetClientSize(dlg.GetSizer()->CalcMin());
      dlg.Center();

      if (dlg.ShowModal() != wxID_OK)
      {
         return;  // user cancelled dialog
      }

      long lrate;
      if (cb->GetValue().ToLong(&lrate) && lrate >= 1 && lrate <= 1000000)
      {
         newRate = (int)lrate;
         break;
      }

      wxMessageBox(_("The entered value is invalid"), _("Error"),
         wxICON_ERROR, mpData->pParent);
   }

   //   SetMenuCheck(*mRateMenu, event.GetId());
   SetRate(pTrack, newRate);

   using namespace RefreshCode;
   mpData->result = RefreshAll | FixScrollbars;
}

//=============================================================================
// Class defining common command handlers for mono and stereo tracks,
// it defines no table of its own
class WaveTrackMenuTable : public PopupMenuTable
{
protected:
   WaveTrackMenuTable() : mpData(NULL) {}

   virtual void InitMenu(Menu *pMenu, void *pUserData);

   virtual void DestroyMenu()
   {
      mpData = NULL;
   }

   TrackControls::InitMenuData *mpData;

   void OnSetDisplay(wxCommandEvent & event);
   void OnSpectrogramSettings(wxCommandEvent & event);
};

void WaveTrackMenuTable::InitMenu(Menu *pMenu, void *pUserData)
{
   mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);

   const int display = pTrack->GetDisplay();
   pMenu->Check(
      (display == WaveTrack::Waveform)
      ? (pTrack->GetWaveformSettings().isLinear() ? OnWaveformID : OnWaveformDBID)
      : OnSpectrumID,
      true
   );

   pMenu->Enable(OnSpectrogramSettingsID, display == WaveTrack::Spectrum);
}

///  Set the Display mode based on the menu choice in the Track Menu.
void WaveTrackMenuTable::OnSetDisplay(wxCommandEvent & event)
{
   int idInt = event.GetId();
   wxASSERT(idInt >= OnWaveformID && idInt <= OnSpectrumID);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack && pTrack->GetKind() == Track::Wave);

   bool linear = false;
   WaveTrack::WaveTrackDisplay id;
   switch (idInt) {
   default:
   case OnWaveformID:
      linear = true, id = WaveTrack::Waveform; break;
   case OnWaveformDBID:
      id = WaveTrack::Waveform; break;
   case OnSpectrumID:
      id = WaveTrack::Spectrum; break;
   }

   const bool wrongType = pTrack->GetDisplay() != id;
   const bool wrongScale =
      (id == WaveTrack::Waveform &&
      pTrack->GetWaveformSettings().isLinear() != linear);
   if (wrongType || wrongScale) {
      pTrack->SetLastScaleType();
      pTrack->SetDisplay(WaveTrack::WaveTrackDisplay(id));
      if (wrongScale)
         pTrack->GetIndependentWaveformSettings().scaleType = linear
         ? WaveformSettings::stLinear
         : WaveformSettings::stLogarithmic;

      WaveTrack *partner = static_cast<WaveTrack *>(pTrack->GetLink());
      if (partner) {
         partner->SetLastScaleType();
         partner->SetDisplay(WaveTrack::WaveTrackDisplay(id));
         if (wrongScale)
            partner->GetIndependentWaveformSettings().scaleType = linear
            ? WaveformSettings::stLinear
            : WaveformSettings::stLogarithmic;
   }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if (wt->GetDisplay() == WaveTrack::WaveformDisplay) {
         wt->SetVirtualState(false);
      }
      else if (id == WaveTrack::WaveformDisplay) {
         wt->SetVirtualState(true);
      }
#endif

      AudacityProject *const project = ::GetActiveProject();
      project->ModifyState(true);

      using namespace RefreshCode;
      mpData->result = RefreshAll | UpdateVRuler;
   }
}

void WaveTrackMenuTable::OnSpectrogramSettings(wxCommandEvent &)
{
   class ViewSettingsDialog : public PrefsDialog
   {
   public:
      ViewSettingsDialog
         (wxWindow *parent, const wxString &title, PrefsDialog::Factories &factories,
         int page)
         : PrefsDialog(parent, title, factories)
         , mPage(page)
      {
      }

      virtual long GetPreferredPage()
      {
         return mPage;
      }

      virtual void SavePreferredPage()
      {
      }

   private:
      const int mPage;
   };

   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   // WaveformPrefsFactory waveformFactory(pTrack);
   SpectrumPrefsFactory spectrumFactory(pTrack);

   // Put Waveform page first
   PrefsDialog::Factories factories;
   // factories.push_back(&waveformFactory);
   factories.push_back(&spectrumFactory);
   const int page = (pTrack->GetDisplay() == WaveTrack::Spectrum)
      ? 1 : 0;

   wxString title(pTrack->GetName() + wxT(": "));
   ViewSettingsDialog dialog(mpData->pParent, title, factories, page);

   if (0 != dialog.ShowModal())
      // Redraw
      mpData->result = RefreshCode::RefreshAll;
}

//=============================================================================
// Class defining handlers peculiar to mono tracks, and a menu table
class MonoTrackMenuTable : public WaveTrackMenuTable
{
   DECLARE_POPUP_MENU(WaveTrackMenuTable);
public:
   static MonoTrackMenuTable &Instance();

private:
   virtual void InitMenu(Menu *pMenu, void *pUserData);

   void OnChannelChange(wxCommandEvent & event);
   void OnMergeStereo(wxCommandEvent & event);
};

MonoTrackMenuTable &MonoTrackMenuTable::Instance()
{
   static MonoTrackMenuTable instance;
   return instance;
}

void MonoTrackMenuTable::InitMenu(Menu *pMenu, void *pUserData)
{
   WaveTrackMenuTable::InitMenu(pMenu, pUserData);

   mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);

   AudacityProject *const project = ::GetActiveProject();
   TrackList *const tracks = project->GetTracks();
   Track *const next = tracks->GetNext(pTrack);

   const bool isMono = !pTrack->GetLinked();
   wxASSERT(isMono);

   if (isMono) {
      const bool canMakeStereo =
         (next && !next->GetLinked()
         && pTrack->GetKind() == Track::Wave
         && next->GetKind() == Track::Wave);
      pMenu->Enable(OnMergeStereoID, canMakeStereo);

      // We only need to set check marks. Clearing checks causes problems on Linux (bug 851)
      switch (pTrack->GetChannel()) {
      case Track::LeftChannel:
         pMenu->Check(OnChannelLeftID, true);
         break;
      case Track::RightChannel:
         pMenu->Check(OnChannelRightID, true);
         break;
      default:
         pMenu->Check(OnChannelMonoID, true);
         break;
      }
   }

}

BEGIN_POPUP_MENU(MonoTrackMenuTable)
   POPUP_MENU_SEPARATOR()

   POPUP_MENU_RADIO_ITEM(OnWaveformID, _("Wa&veform"), OnSetDisplay)
   POPUP_MENU_RADIO_ITEM(OnWaveformDBID, _("&Waveform (dB)"), OnSetDisplay)
   POPUP_MENU_RADIO_ITEM(OnSpectrumID, _("&Spectrogram"), OnSetDisplay)
   POPUP_MENU_ITEM(OnSpectrogramSettingsID, _("S&pectrogram Settings..."), OnSpectrogramSettings)
   POPUP_MENU_SEPARATOR()

   POPUP_MENU_RADIO_ITEM(OnChannelMonoID, _("&Mono"), OnChannelChange)
   POPUP_MENU_RADIO_ITEM(OnChannelLeftID, _("&Left"), OnChannelChange)
   POPUP_MENU_RADIO_ITEM(OnChannelRightID, _("R&ight"), OnChannelChange)
   POPUP_MENU_ITEM(OnMergeStereoID, _("Make &Stereo"), OnMergeStereo)
   POPUP_MENU_SEPARATOR()

   POPUP_MENU_SUB_MENU(0, _("&Format"), FormatMenuTable)
   POPUP_MENU_SEPARATOR()
   POPUP_MENU_SUB_MENU(0, _("&Rate"), RateMenuTable)
END_POPUP_MENU()

void MonoTrackMenuTable::OnChannelChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnChannelMonoID && id <= OnChannelRightID);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack);
   int channel;
   wxString channelmsg;
   switch (id) {
   default:
   case OnChannelMonoID:
      channel = Track::MonoChannel;
      channelmsg = _("Mono");
      break;
   case OnChannelLeftID:
      channel = Track::LeftChannel;
      channelmsg = _("Left Channel");
      break;
   case OnChannelRightID:
      channel = Track::RightChannel;
      channelmsg = _("Right Channel");
      break;
   }
   pTrack->SetChannel(channel);
   AudacityProject *const project = ::GetActiveProject();
   project->PushState(wxString::Format(_("Changed '%s' to %s"),
      pTrack->GetName().c_str(),
      channelmsg),
      _("Channel"));
   mpData->result = RefreshCode::RefreshAll;
}

/// Merge two tracks into one stereo track ??
void MonoTrackMenuTable::OnMergeStereo(wxCommandEvent &)
{
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack);
   pTrack->SetLinked(true);
   Track *const partner = pTrack->GetLink();

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if (MONO_WAVE_PAN(pTrack))
      pTrack->SetVirtualState(false);
   if (MONO_WAVE_PAN(partner))
      static_cast<WaveTrack*>(partner)->SetVirtualState(false);
#endif

   if (partner) {
      // Set partner's parameters to match target.
      partner->Merge(*pTrack);

      pTrack->SetChannel(Track::LeftChannel);
      partner->SetChannel(Track::RightChannel);

      // Set new track heights and minimized state
      bool bBothMinimizedp = ((pTrack->GetMinimized()) && (partner->GetMinimized()));
      pTrack->SetMinimized(false);
      partner->SetMinimized(false);
      int AverageHeight = (pTrack->GetHeight() + partner->GetHeight()) / 2;
      pTrack->SetHeight(AverageHeight);
      partner->SetHeight(AverageHeight);
      pTrack->SetMinimized(bBothMinimizedp);
      partner->SetMinimized(bBothMinimizedp);

      //On Demand - join the queues together.
      if (ODManager::IsInstanceCreated() &&
          partner->GetKind() == Track::Wave &&
          pTrack->GetKind() == Track::Wave)
         if (!ODManager::Instance()->
             MakeWaveTrackDependent(static_cast<WaveTrack*>(partner), pTrack))
         {
            ;
            //TODO: in the future, we will have to check the return value of MakeWaveTrackDependent -
            //if the tracks cannot merge, it returns false, and in that case we should not allow a merging.
            //for example it returns false when there are two different types of ODTasks on each track's queue.
            //we will need to display this to the user.
         }

      AudacityProject *const project = ::GetActiveProject();
      project->PushState(wxString::Format(_("Made '%s' a stereo track"),
         pTrack->GetName().
         c_str()),
         _("Make Stereo"));
   }
   else
      pTrack->SetLinked(false);

   mpData->result = RefreshCode::RefreshAll;
}

//=============================================================================
// Class defining handlers peculiar to stereo tracks, and a menu table
class StereoTrackMenuTable : public WaveTrackMenuTable
{
   DECLARE_POPUP_MENU(WaveTrackMenuTable);

public:
   static StereoTrackMenuTable &Instance();

private:
   virtual void InitMenu(Menu *pMenu, void *pUserData);

   void SplitStereo(bool stereo);

   void OnSwapChannels(wxCommandEvent & event);
   void OnSplitStereo(wxCommandEvent & event);
   void OnSplitStereoMono(wxCommandEvent & event);
};

StereoTrackMenuTable &StereoTrackMenuTable::Instance()
{
   static StereoTrackMenuTable instance;
   return instance;
}

void StereoTrackMenuTable::InitMenu(Menu *pMenu, void *pUserData)
{
   WaveTrackMenuTable::InitMenu(pMenu, pUserData);

   mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);

   const bool isMono = !pTrack->GetLinked();
   wxASSERT(!isMono);
}

BEGIN_POPUP_MENU(StereoTrackMenuTable)
   POPUP_MENU_SEPARATOR()

   POPUP_MENU_RADIO_ITEM(OnWaveformID, _("Wa&veform"), OnSetDisplay)
   POPUP_MENU_RADIO_ITEM(OnWaveformDBID, _("&Waveform (dB)"), OnSetDisplay)
   POPUP_MENU_RADIO_ITEM(OnSpectrumID, _("&Spectrum"), OnSetDisplay)
   POPUP_MENU_ITEM(OnSpectrogramSettingsID, _("S&pectrogram Settings..."), OnSpectrogramSettings)
   POPUP_MENU_SEPARATOR()

   POPUP_MENU_ITEM(OnSwapChannelsID, _("S&wap"), OnSwapChannels)
   POPUP_MENU_ITEM(OnSplitStereoID, _("S&plit"), OnSplitStereo)
   POPUP_MENU_ITEM(OnSplitStereoMonoID, _("Split to &Mono"), OnSplitStereoMono)
   POPUP_MENU_SEPARATOR()

   POPUP_MENU_SUB_MENU(0, _("&Format"), FormatMenuTable)
   POPUP_MENU_SEPARATOR()
   POPUP_MENU_SUB_MENU(0, _("&Rate"), RateMenuTable)
END_POPUP_MENU()

/// Split a stereo track into two tracks...
void StereoTrackMenuTable::SplitStereo(bool stereo)
{
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   wxASSERT(pTrack);
   if (!stereo)
      pTrack->SetChannel(Track::MonoChannel);

   Track *const partner = pTrack->GetLink();

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   if (!stereo && MONO_WAVE_PAN(pTrack))
      pTrack->SetVirtualState(true, true);
   if (!stereo && MONO_WAVE_PAN(partner))
      partner->SetVirtualState(true, true);
#endif

   if (partner)
   {
      partner->SetName(pTrack->GetName());
      if (!stereo)
         partner->SetChannel(Track::MonoChannel);  // Keep original stereo track name.

      //On Demand - have each channel add its own.
      if (ODManager::IsInstanceCreated() && partner->GetKind() == Track::Wave)
         ODManager::Instance()->MakeWaveTrackIndependent(static_cast<WaveTrack*>(partner));
   }

   pTrack->SetLinked(false);
   //make sure neither track is smaller than its minimum height
   if (pTrack->GetHeight() < pTrack->GetMinimizedHeight())
      pTrack->SetHeight(pTrack->GetMinimizedHeight());
   if (partner)
   {
      if (partner->GetHeight() < partner->GetMinimizedHeight())
         partner->SetHeight(partner->GetMinimizedHeight());

      // Make tracks the same height
      if (pTrack->GetHeight() != partner->GetHeight())
      {
         pTrack->SetHeight((pTrack->GetHeight() + partner->GetHeight()) / 2.0);
         partner->SetHeight(pTrack->GetHeight());
      }
   }
}

/// Swap the left and right channels of a stero track...
void StereoTrackMenuTable::OnSwapChannels(wxCommandEvent &)
{
   AudacityProject *const project = ::GetActiveProject();

   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   Track *partner = pTrack->GetLink();
   Track *const focused = project->GetTrackPanel()->GetFocusedTrack();
   const bool hasFocus =
      (focused == pTrack || focused == partner);

   SplitStereo(true);
   pTrack->SetChannel(Track::RightChannel);
   partner->SetChannel(Track::LeftChannel);

   TrackList *const tracks = project->GetTracks();
   (tracks->MoveUp(partner));
   partner->SetLinked(true);

   MixerBoard* pMixerBoard = project->GetMixerBoard();
   if (pMixerBoard) {
      pMixerBoard->UpdateTrackClusters();
   }

   if (hasFocus)
      project->GetTrackPanel()->SetFocusedTrack(partner);

   project->PushState(wxString::Format(_("Swapped Channels in '%s'"),
      pTrack->GetName().c_str()),
      _("Swap Channels"));

   mpData->result = RefreshCode::RefreshAll;
}

/// Split a stereo track into two tracks...
void StereoTrackMenuTable::OnSplitStereo(wxCommandEvent &)
{
   SplitStereo(true);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   AudacityProject *const project = ::GetActiveProject();
   project->PushState(wxString::Format(_("Split stereo track '%s'"),
      pTrack->GetName().c_str()),
      _("Split"));

   mpData->result = RefreshCode::RefreshAll;
}

/// Split a stereo track into two mono tracks...
void StereoTrackMenuTable::OnSplitStereoMono(wxCommandEvent &)
{
   SplitStereo(false);
   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);
   AudacityProject *const project = ::GetActiveProject();
   project->PushState(wxString::Format(_("Split Stereo to Mono '%s'"),
      pTrack->GetName().c_str()),
      _("Split to Mono"));

   mpData->result = RefreshCode::RefreshAll;
}

//=============================================================================
PopupMenuTable *WaveTrackControls::GetMenuExtension(Track *pTrack)
{
   if (pTrack->GetLink())
      return &StereoTrackMenuTable::Instance();
   else
      return &MonoTrackMenuTable::Instance();
}
