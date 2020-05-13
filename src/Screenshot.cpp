/**********************************************************************

  Audacity: A Digital Audio Editor

  Screenshot.cpp

  Dominic Mazzoni

*******************************************************************//**

\class ScreenshotBigDialog
\brief ScreenshotBigDialog provides an alternative Gui for ScreenshotCommand.
It adds a timer that allows a delay before taking a screenshot,
provides lots of one-click buttons, options to resize the screen.
It forwards the actual work of doing the commands to the ScreenshotCommand.

***********************************************************************/

#include "Screenshot.h"
#include "commands/ScreenshotCommand.h"
#include "commands/CommandTargets.h"
#include "commands/CommandContext.h"
#include <wx/defs.h>
#include <wx/frame.h>

#include "ShuttleGui.h"
#include <wx/checkbox.h>
#include <wx/dirdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/tglbtn.h>

#include "prefs/GUISettings.h" // for RTL_WORKAROUND
#include "Project.h"
#include "ProjectStatus.h"
#include "ProjectWindow.h"
#include "Prefs.h"
#include "prefs/GUIPrefs.h"
#include "tracks/ui/TrackView.h"
#include "widgets/HelpSystem.h"

#include "ViewInfo.h"
#include "WaveTrack.h"

class OldStyleCommandType;
class ScreenFrameTimer;

////////////////////////////////////////////////////////////////////////////////
#define ScreenCaptureFrameTitle XO("Screen Capture Frame")

// ANSWER-ME: Should this derive from wxDialogWrapper instead?
class ScreenshotBigDialog final : public wxFrame,
                                  public PrefsListener
{
 public:

   // constructors and destructors
   ScreenshotBigDialog(
      wxWindow *parent, wxWindowID id, AudacityProject &project);
   virtual ~ScreenshotBigDialog();

   bool ProcessEvent(wxEvent & event) override;

 private:
   void Populate();
   void PopulateOrExchange(ShuttleGui &S);

   void OnCloseWindow(wxCloseEvent & event);
   void OnDirChoose();
   void OnGetURL();
   void OnClose();


   void SizeMainWindow(int w, int h);
   void OnMainWindowSmall();
   void OnMainWindowLarge();
   void OnToggleBackgroundBlue(wxCommandEvent & event);
   void OnToggleBackgroundWhite(wxCommandEvent & event);

   void DoCapture(int captureMode);
   void OnCaptureSomething(wxCommandEvent & event);

   void TimeZoom(double seconds);
   void OnOneSec();
   void OnTenSec();
   void OnOneMin();
   void OnFiveMin();
   void OnOneHour();

   void SizeTracks(int h);
   void OnShortTracks();
   void OnMedTracks();
   void OnTallTracks();

   // PrefsListener implementation
   void UpdatePrefs() override;

   AudacityProject &mProject;

   std::unique_ptr<ScreenshotCommand> CreateCommand();

   wxCheckBox *mDelayCheckBox;
   wxTextCtrl *mDirectoryTextBox;
   wxToggleButton *mBlue;
   wxToggleButton *mWhite;
   wxStatusBar *mStatus;

   std::unique_ptr<ScreenFrameTimer> mTimer;

   std::unique_ptr<ScreenshotCommand> mCommand;
   const CommandContext mContext;

   DECLARE_EVENT_TABLE()
};

// Static pointer to the unique ScreenshotBigDialog window.
// Formerly it was parentless, therefore this was a Destroy_ptr<ScreenshotBigDialog>
// But now the window is owned, so just use a bare pointer, and null it when
// the unique window is destroyed.
using ScreenshotBigDialogPtr = ScreenshotBigDialog*;
ScreenshotBigDialogPtr mFrame;

////////////////////////////////////////////////////////////////////////////////

void OpenScreenshotTools( AudacityProject &project )
{
   if (!mFrame) {
      auto parent = wxTheApp->GetTopWindow();
      if (!parent) {
         wxASSERT(false);
         return;
      }
      mFrame = ScreenshotBigDialogPtr{
         safenew ScreenshotBigDialog(parent, -1, project) };
   }
   mFrame->Show();
   mFrame->Raise();
}

void CloseScreenshotTools()
{
   mFrame = nullptr;
}

////////////////////////////////////////////////////////////////////////////////

class ScreenFrameTimer final : public wxTimer
{
 public:
   ScreenFrameTimer(ScreenshotBigDialog *frame,
                    wxEvent & event)
   {
      screenFrame = frame;
      evt.reset(event.Clone());
   }

   virtual ~ScreenFrameTimer()
   {
      if (IsRunning())
      {
         Stop();
      }
   }

   void Notify() override
   {
      // Process timer notification just once, then destroy self
      evt->SetEventObject(NULL);
      screenFrame->ProcessEvent(*evt);
   }

 private:
   ScreenshotBigDialog *screenFrame;
   std::unique_ptr<wxEvent> evt;
};

////////////////////////////////////////////////////////////////////////////////

enum
{
   IdMainWindowSmall = 19200,
   IdDirectory,

   IdDelayCheckBox,

   IdCaptureFirst,
   // No point delaying the capture of sets of things.
   IdCaptureEffects= IdCaptureFirst,
   IdCaptureScriptables,
   IdCapturePreferences,
   IdCaptureToolbars,

   // Put all events that need delay between AllDelayed and LastDelayed.
   IdAllDelayedEvents,
   IdCaptureWindowContents=IdAllDelayedEvents,
   IdCaptureFullWindow,
   IdCaptureWindowPlus,
   IdCaptureFullScreen,

   IdCaptureSelectionBar,
   IdCaptureSpectralSelection,
   IdCaptureTimer,
   IdCaptureTools,
   IdCaptureTransport,
   IdCaptureMixer,
   IdCaptureMeter,
   IdCapturePlayMeter,
   IdCaptureRecordMeter,
   IdCaptureEdit,
   IdCaptureDevice,
   IdCaptureTranscription,
   IdCaptureScrub,

   IdCaptureTrackPanel,
   IdCaptureRuler,
   IdCaptureTracks,
   IdCaptureFirstTrack,
   IdCaptureSecondTrack,
   IdCaptureLast = IdCaptureSecondTrack,

   IdLastDelayedEvent,

   IdToggleBackgroundBlue,
   IdToggleBackgroundWhite,

};

BEGIN_EVENT_TABLE(ScreenshotBigDialog, wxFrame)
   EVT_CLOSE(ScreenshotBigDialog::OnCloseWindow)

   EVT_TOGGLEBUTTON(IdToggleBackgroundBlue,   ScreenshotBigDialog::OnToggleBackgroundBlue)
   EVT_TOGGLEBUTTON(IdToggleBackgroundWhite,  ScreenshotBigDialog::OnToggleBackgroundWhite)
   EVT_COMMAND_RANGE(IdCaptureFirst, IdCaptureLast, wxEVT_COMMAND_BUTTON_CLICKED, ScreenshotBigDialog::OnCaptureSomething)
END_EVENT_TABLE();

// Must not be called before CreateStatusBar!
std::unique_ptr<ScreenshotCommand> ScreenshotBigDialog::CreateCommand()
{
   wxASSERT(mStatus != NULL);
   auto output =
      std::make_unique<CommandOutputTargets>(std::make_unique<NullProgressTarget>(),
                              std::make_shared<StatusBarTarget>(*mStatus),
                              std::make_shared<MessageBoxTarget>());
   return std::make_unique<ScreenshotCommand>();//*type, std::move(output), this);
}

ScreenshotBigDialog::ScreenshotBigDialog(
   wxWindow * parent, wxWindowID id, AudacityProject &project)
:  wxFrame(parent, id, ScreenCaptureFrameTitle.Translation(),
           wxDefaultPosition, wxDefaultSize,

#if !defined(__WXMSW__)

   #if !defined(__WXMAC__) // bug1358
           wxFRAME_TOOL_WINDOW |
   #endif

#else

           wxSTAY_ON_TOP|

#endif

           wxSYSTEM_MENU|wxCAPTION|wxCLOSE_BOX)
   , mProject{ project }
   , mContext( project )
{
   mDelayCheckBox = NULL;
   mDirectoryTextBox = NULL;

   mStatus = CreateStatusBar(3);
   mCommand = CreateCommand();

   Populate();

   // Reset the toolbars to a known state.
   // Note that the audio could be playing.
   // The monitoring will switch off temporarily
   // because we've switched monitor mid play.
   // Bug 383 - Resetting the toolbars is not wanted.
   // Any that are invisible will be amde visible as/when needed.
   //ToolManager::Get( mContext.project ).Reset();
   Center();
}

ScreenshotBigDialog::~ScreenshotBigDialog()
{
   if (this == mFrame)
      mFrame = nullptr;
   else
      // There should only be one!
      wxASSERT(false);
}

void ScreenshotBigDialog::Populate()
{
   ShuttleGui S(this);
   PopulateOrExchange(S);
}

void ScreenshotBigDialog::PopulateOrExchange(ShuttleGui & S)
{
   auto enabler = [this] {
#ifdef __WXMAC__
      if ( mCommand ) {
         wxTopLevelWindow *top = mCommand->GetFrontWindow(&mProject);
         return (top && !top->IsIconized());
      }
#endif
      return true;
   };

   wxPanel *p = S.StartPanel();
   RTL_WORKAROUND(p);
   {
      S
         .StartStatic(XO("Choose location to save files"));
      {
         S.StartMultiColumn(3, GroupOptions{ wxEXPAND }.StretchyColumn(1));
         {
            S
               .Id(IdDirectory)
               .Enable( enabler )
               .AddTextBox(
                  XXO("Save images to:"),
                  gPrefs->Read(L"/ScreenshotPath", wxFileName::GetHomeDir()),
                  30 )
               .Assign(mDirectoryTextBox);

            S
               .Enable( enabler )
               .Action( [this]{ OnDirChoose(); } )
               .AddButton(XXO("Choose..."));
         }
         S.EndMultiColumn();
      }
      S.EndStatic();

      S
         .StartStatic(XO("Capture entire window or screen"));
      {
         S.StartHorizontalLay();
         {
            S
               .Enable( enabler )
               .Action( [this]{ OnMainWindowSmall(); } )
               .AddButton(XXO("Resize Small"));

            S
               .Enable( enabler )
               .Action( [this]{ OnMainWindowLarge(); } )
               .AddButton(XXO("Resize Large"));

            S
               .Id(IdToggleBackgroundBlue)
               /* i18n-hint: Bkgnd is short for background and appears on a small button
                * It is OK to just translate this item as if it said 'Blue' */
               .Window<wxToggleButton>(_("Blue Bkgnd"))
               .Assign(mBlue);

            S
               .Id(IdToggleBackgroundWhite)
            /* i18n-hint: Bkgnd is short for background and appears on a small button
             * It is OK to just translate this item as if it said 'White' */
               .Window<wxToggleButton>(_("White Bkgnd"))
               .Assign(mWhite);
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureWindowContents)
               .Enable( enabler )
               .AddButton(XXO("Capture Window Only"));

            S
               .Id(IdCaptureFullWindow)
               .Enable( enabler )
               .AddButton(XXO("Capture Full Window"));

            S
               .Id(IdCaptureWindowPlus)
               .Enable( enabler )
               .AddButton(XXO("Capture Window Plus"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureFullScreen)
               .Enable( enabler )
               .AddButton(XXO("Capture Full Screen"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdDelayCheckBox)
               .Enable( enabler )
               .AddCheckBox(
                  XXO("Wait 5 seconds and capture frontmost window/dialog"),
                  false)
               .Assign(mDelayCheckBox);
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S
         .StartStatic(XO("Capture part of a project window"));
      {
         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureToolbars)
               .Enable( enabler )
               .AddButton(XXO("All Toolbars"));

            S
               .Id(IdCaptureEffects)
               .Enable( enabler )
               .AddButton(XXO("All Effects"));

            S
               .Id(IdCaptureScriptables)
               .AddButton(XXO("All Scriptables"));

            S
               .Enable( enabler )
               .AddButton(XXO("All Preferences"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureSelectionBar)
               .Enable( enabler )
               .AddButton(XXO("SelectionBar"));

            S
               .Id(IdCaptureSpectralSelection)
               .Enable( enabler )
               .AddButton(XXO("Spectral Selection"));

            S
               .Id(IdCaptureTimer)
               .Enable( enabler )
               .AddButton(XXO("Timer"));

            S
               .Id(IdCaptureTools)
               .Enable( enabler )
               .AddButton(XXO("Tools"));

            S
               .Id(IdCaptureTransport)
               .Enable( enabler )
               .AddButton(XXO("Transport"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureMixer)
               .Enable( enabler )
               .AddButton(XXO("Mixer"));

            S
               .Id(IdCaptureMeter)
               .Enable( enabler )
               .AddButton(XXO("Meter"));

            S
               .Id(IdCapturePlayMeter)
               .Enable( enabler )
               .AddButton(XXO("Play Meter"));

            S
               .Id(IdCaptureRecordMeter)
               .Enable( enabler )
               .AddButton(XXO("Record Meter"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureEdit)
               .Enable( enabler )
               .AddButton(XXO("Edit"));

            S
               .Id(IdCaptureDevice)
               .Enable( enabler )
               .AddButton(XXO("Device"));

            S
               .Id(IdCaptureTranscription)
               .Enable( enabler )
               .AddButton(XXO("Play-at-Speed"));

            S
               .Id(IdCaptureScrub)
               .Enable( enabler )
               .AddButton(XXO("Scrub"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Id(IdCaptureTrackPanel)
               .Enable( enabler )
               .AddButton(XXO("Track Panel"));

            S
               .Id(IdCaptureRuler)
               .Enable( enabler )
               .AddButton(XXO("Ruler"));

            S
               .Id(IdCaptureTracks)
               .Enable( enabler )
               .AddButton(XXO("Tracks"));

            S
               .Id(IdCaptureFirstTrack)
               .Enable( enabler )
               .AddButton(XXO("First Track"));

            S
               .Id(IdCaptureSecondTrack)
               .Enable( enabler )
               .AddButton(XXO("Second Track"));
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S
         .StartStatic(XO("Scale"));
      {
         S.StartHorizontalLay();
         {
            S
               .Enable( enabler )
              .Action( [this]{ OnOneSec(); } )
                .AddButton(XXO("One Sec"));

            S
               .Enable( enabler )
               .Action( [this]{ OnTenSec(); } )
               .AddButton(XXO("Ten Sec"));

            S
               .Enable( enabler )
               .Action( [this]{ OnOneMin(); } )
               .AddButton(XXO("One Min"));

            S
               .Enable( enabler )
               .Action( [this]{ OnFiveMin(); } )
               .AddButton(XXO("Five Min"));

            S
               .Enable( enabler )
               .Action( [this]{ OnOneHour(); } )
               .AddButton(XXO("One Hour"));
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay();
         {
            S
               .Enable( enabler )
               .Action( [this]{ OnShortTracks(); } )
               .AddButton(XXO("Short Tracks"));

            S
               .Enable( enabler )
               .Action( [this]{ OnMedTracks(); } )
               .AddButton(XXO("Medium Tracks"));

            S
               .Enable( enabler )
               .Action( [this]{ OnTallTracks(); } )
               .AddButton(XXO("Tall Tracks"));
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S
         .AddStandardButtons(0, {
            S.Item( eCancelButton ).Action( [this]{ OnClose(); } ),
            S.Item( eHelpButton ).Action( [this]{ OnGetURL(); } )
         });
   }
   S.EndPanel();

   Layout();
   GetSizer()->Fit(this);
   SetMinSize(GetSize());

   int top = 0;
#ifdef __WXMAC__
   // Allow for Mac menu bar
   top += 20;
#endif

   int width, height;
   GetSize(&width, &height);
   int displayWidth, displayHeight;
   wxDisplaySize(&displayWidth, &displayHeight);

   if (width > 100) {
      Move(displayWidth - width - 16, top + 16);
   }
   else {
      CentreOnParent();
   }

   SetIcon( GetProjectFrame( mContext.project ).GetIcon() );
}

bool ScreenshotBigDialog::ProcessEvent(wxEvent & e)
{
   if (!IsFrozen())
   {
      int id = e.GetId();

      // If split into two parts to make for easier breakpoint
      // when testing timer.
      if (mDelayCheckBox &&
          mDelayCheckBox->GetValue() &&
          e.IsCommandEvent() &&
          e.GetEventType() == wxEVT_COMMAND_BUTTON_CLICKED)
      {
         if( id >= IdAllDelayedEvents && id <= IdLastDelayedEvent &&
          e.GetEventObject() != NULL) {
            mTimer = std::make_unique<ScreenFrameTimer>(this, e);
            mTimer->Start(5000, true);
            return true;
         }
      }

      if (e.IsCommandEvent() && e.GetEventObject() == NULL) {
         e.SetEventObject(this);
      }
   }

   return wxFrame::ProcessEvent(e);
}

void ScreenshotBigDialog::OnCloseWindow(wxCloseEvent &  WXUNUSED(event))
{
   if (mDirectoryTextBox->IsModified()) {
      gPrefs->Write(L"/ScreenshotPath", mDirectoryTextBox->GetValue());
      gPrefs->Flush();
   }

   Destroy();
}

void ScreenshotBigDialog::OnClose()
{
   if (mDirectoryTextBox->IsModified()) {
      gPrefs->Write(L"/ScreenshotPath", mDirectoryTextBox->GetValue());
      gPrefs->Flush();
   }

   Destroy();
}

void ScreenshotBigDialog::OnGetURL()
{
   HelpSystem::ShowHelp(this, L"Screenshot");
}

void ScreenshotBigDialog::OnDirChoose()
{
   auto current = mDirectoryTextBox->GetValue();

   wxDirDialogWrapper dlog(this,
      XO("Choose a location to save screenshot images"),
      current);

   dlog.ShowModal();
   if (!dlog.GetPath().empty()) {
      wxFileName tmpDirPath;
      tmpDirPath.AssignDir(dlog.GetPath());
      auto path = tmpDirPath.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR);
      mDirectoryTextBox->SetValue(path);
      gPrefs->Write(L"/ScreenshotPath", path);
      gPrefs->Flush();
      mCommand->mPath = path;
   }
}

void ScreenshotBigDialog::OnToggleBackgroundBlue(wxCommandEvent & WXUNUSED(event))
{
   mWhite->SetValue(false);
}

void ScreenshotBigDialog::OnToggleBackgroundWhite(wxCommandEvent & WXUNUSED(event))
{
   mBlue->SetValue(false);
}

void ScreenshotBigDialog::SizeMainWindow(int w, int h)
{
   int top = 20;

   auto &window = GetProjectFrame( mContext.project );
   window.Maximize(false);
   window.SetSize(16, 16 + top, w, h);
   //Bug383 - Toolbar Resets not wanted.
   //ToolManager::Get( mContext.project ).Reset();
}

void ScreenshotBigDialog::OnMainWindowSmall()
{
   SizeMainWindow(680, 450);
}

void ScreenshotBigDialog::OnMainWindowLarge()
{
   SizeMainWindow(900, 600);
}

void ScreenshotBigDialog::DoCapture(int captureMode)
{
   Hide();
   wxYieldIfNeeded();
   //mCommand->SetParameter(L"FilePath", mDirectoryTextBox->GetValue());
   //mCommand->SetParameter(L"CaptureMode", captureMode);
   mCommand->mBack = mWhite->GetValue()
      ? ScreenshotCommand::kWhite
      : mBlue->GetValue()
         ? ScreenshotCommand::kBlue : ScreenshotCommand::kNone;
   mCommand->mPath = mDirectoryTextBox->GetValue();
   mCommand->mWhat = captureMode;
   if (!mCommand->Apply(mContext))
      mStatus->SetStatusText(_("Capture failed!"), mainStatusBarField);

   // Bug 2323: (100% hackage alert) Since the command target dialog is not
   // accessible from outside the command, this seems to be the only way we
   // can get the window on top of this dialog. 
   auto w = static_cast<wxDialogWrapper *>(wxFindWindowByLabel(XO("Long Message").Translation()));
   if (w) {
      auto endmodal = [w](wxCommandEvent &evt)
      {
         w->EndModal(0);
      };
      w->Bind(wxEVT_BUTTON, endmodal);
      w->ShowModal();
   }

   Show();
}

void ScreenshotBigDialog::OnCaptureSomething(wxCommandEvent &  event)
{
   int i = event.GetId() - IdCaptureFirst;

   /*
   IdCaptureEffects= IdCaptureFirst,
   IdCaptureScriptables,
   IdCapturePreferences,
   IdCaptureToolbars,

   // Put all events that need delay between AllDelayed and LastDelayed.
   IdAllDelayedEvents,
   IdCaptureWindowContents=IdAllDelayedEvents,
   IdCaptureFullWindow,
   IdCaptureWindowPlus,
   IdCaptureFullScreen,

   IdCaptureSelectionBar,
   IdCaptureSpectralSelection,
   IdCaptureTools,
   IdCaptureTransport,
   IdCaptureMixer,
   IdCaptureMeter,
   IdCapturePlayMeter,
   IdCaptureRecordMeter,
   IdCaptureEdit,
   IdCaptureDevice,
   IdCaptureTranscription,
   IdCaptureScrub,

   IdCaptureTrackPanel,
   IdCaptureRuler,
   IdCaptureTracks,
   IdCaptureFirstTrack,
   IdCaptureSecondTrack,
   IdCaptureLast = IdCaptureSecondTrack,
    */

   static const int codes[] = {
      ScreenshotCommand::keffects,
      ScreenshotCommand::kscriptables,
      ScreenshotCommand::kpreferences,
      ScreenshotCommand::ktoolbars,

      ScreenshotCommand::kwindow,
      ScreenshotCommand::kfullwindow,
      ScreenshotCommand::kwindowplus,
      ScreenshotCommand::kfullscreen,
      ScreenshotCommand::kselectionbar,
      ScreenshotCommand::kspectralselection,
      ScreenshotCommand::ktimer,
      ScreenshotCommand::ktools,
      ScreenshotCommand::ktransport,
      ScreenshotCommand::kmixer,
      ScreenshotCommand::kmeter,
      ScreenshotCommand::kplaymeter,
      ScreenshotCommand::krecordmeter,
      ScreenshotCommand::kedit,
      ScreenshotCommand::kdevice,
      ScreenshotCommand::ktranscription,
      ScreenshotCommand::kscrub,
      ScreenshotCommand::ktrackpanel,
      ScreenshotCommand::kruler,
      ScreenshotCommand::ktracks,
      ScreenshotCommand::kfirsttrack,
      ScreenshotCommand::ksecondtrack,
   };

   DoCapture(codes[i]);
}

void ScreenshotBigDialog::TimeZoom(double seconds)
{
   auto &viewInfo = ViewInfo::Get( mContext.project );
   auto &window = ProjectWindow::Get( mContext.project );
   int width, height;
   window.GetClientSize(&width, &height);
   viewInfo.SetZoom((0.75 * width) / seconds);
   window.RedrawProject();
}

void ScreenshotBigDialog::OnOneSec()
{
   TimeZoom(1.0);
}

void ScreenshotBigDialog::OnTenSec()
{
   TimeZoom(10.0);
}

void ScreenshotBigDialog::OnOneMin()
{
   TimeZoom(60.0);
}

void ScreenshotBigDialog::OnFiveMin()
{
   TimeZoom(300.0);
}

void ScreenshotBigDialog::OnOneHour()
{
   TimeZoom(3600.0);
}

void ScreenshotBigDialog::SizeTracks(int h)
{
   // h is the height for a channel
   // Set the height of a mono track twice as high

   // TODO: more-than-two-channels
   // If there should be more-than-stereo tracks, this makes
   // each channel as high as for a stereo channel

   auto &tracks = TrackList::Get( mContext.project );
   for (auto t : tracks.Leaders<WaveTrack>()) {
      auto channels = TrackList::Channels(t);
      auto nChannels = channels.size();
      auto height = nChannels == 1 ? 2 * h : h;
      for (auto channel : channels)
         TrackView::Get( *channel ).SetHeight(height);
   }
   ProjectWindow::Get( mContext.project ).RedrawProject();
}

void ScreenshotBigDialog::OnShortTracks()
{
   for (auto t : TrackList::Get( mContext.project ).Any<WaveTrack>()) {
      auto &view = TrackView::Get( *t );
      view.SetHeight( view.GetMinimizedHeight() );
   }

   ProjectWindow::Get( mContext.project ).RedrawProject();
}

void ScreenshotBigDialog::OnMedTracks()
{
   SizeTracks(60);
}

void ScreenshotBigDialog::OnTallTracks()
{
   SizeTracks(85);
}

void ScreenshotBigDialog::UpdatePrefs()
{
   Freeze();

   SetSizer(nullptr);
   DestroyChildren();

   SetTitle(ScreenCaptureFrameTitle.Translation());
   Populate();

   Thaw();
}
