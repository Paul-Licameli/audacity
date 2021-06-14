/**********************************************************************

 Audacity: A Digital Audio Editor

 RealtimeEffectUI.cpp

 *********************************************************************/

#include "RealtimeEffectUI.h"
#include "RealtimeEffectManager.h"
#include "RealtimeEffectState.h"

#include "Effect.h"
#include "EffectManager.h"
#include "EffectUI.h"

#include "../AColor.h"
#include "../AllThemeResources.h"
#include "../ImageManipulation.h"
#include "../PluginManager.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../ProjectHistory.h"
#include "../ProjectWindowBase.h"
#include "../ShuttleGui.h"
#include "../Theme.h"
#include "../TrackPanelAx.h"
#include "../UndoManager.h"
#include "../commands/CommandContext.h"
#include "../widgets/AButton.h"
#include "../widgets/Grid.h"
#include "../widgets/wxPanelWrapper.h"

#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/frame.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/popupwin.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/tglbtn.h>

#include <wx/generic/listctrl.h>

#include "../../images/RealtimeEffectUI/RealtimeEffectUI.h"

#define COL_Power    0
#define COL_Editor   1
#define COL_PrePost  2
#define COL_Up       3
#define COL_Down     4
#define COL_Remove   5
#define COL_Name     6
#define NUMCOLS      7

#define ID_Add       20001
#define ID_Bypass    20002

#define ID_Base      21000
#define ID_Range     100
#define ID_Power     (ID_Base + (COL_Power * ID_Range))
#define ID_Editor    (ID_Base + (COL_Editor * ID_Range))
#define ID_PrePost   (ID_Base + (COL_PrePost * ID_Range))
#define ID_Up        (ID_Base + (COL_Up * ID_Range))
#define ID_Down      (ID_Base + (COL_Down * ID_Range))
#define ID_Remove    (ID_Base + (COL_Remove * ID_Range))
#define ID_Name      (ID_Base + (COL_Name * ID_Range))

BEGIN_EVENT_TABLE(RealtimeEffectUI, ThemedDialog)
   EVT_TIMER(wxID_ANY, RealtimeEffectUI::OnTimer)

   EVT_BUTTON(ID_Add, RealtimeEffectUI::OnAdd)
   EVT_BUTTON(ID_Bypass, RealtimeEffectUI::OnBypass)

   EVT_COMMAND_RANGE(ID_Power,   ID_Power + ID_Range - 1,   wxEVT_COMMAND_BUTTON_CLICKED, RealtimeEffectUI::OnPower)
   EVT_COMMAND_RANGE(ID_Editor,  ID_Editor + ID_Range - 1,  wxEVT_COMMAND_BUTTON_CLICKED, RealtimeEffectUI::OnEditor)
   EVT_COMMAND_RANGE(ID_PrePost, ID_PrePost + ID_Range - 1, wxEVT_COMMAND_BUTTON_CLICKED, RealtimeEffectUI::OnPrePost)
   EVT_COMMAND_RANGE(ID_Up,      ID_Up + ID_Range - 1,      wxEVT_COMMAND_BUTTON_CLICKED, RealtimeEffectUI::OnUp)
   EVT_COMMAND_RANGE(ID_Down,    ID_Down + ID_Range - 1,    wxEVT_COMMAND_BUTTON_CLICKED, RealtimeEffectUI::OnDown)
   EVT_COMMAND_RANGE(ID_Remove,  ID_Remove + ID_Range - 1,  wxEVT_COMMAND_BUTTON_CLICKED, RealtimeEffectUI::OnRemove)
END_EVENT_TABLE()

RealtimeEffectUI::RealtimeEffectUI(RealtimeEffectManager *manager,
                                   const TranslatableString &title,
                                   RealtimeEffectList &list)
:  ThemedDialog(),
   mManager(manager),
   mTitle(title),   
   mList(list)
{
   mIDCounter = 0;
   mLastLatency = -1;
   mTimer.SetOwner(this);

   Create(&GetProjectFrame(manager->mProject), wxID_ANY, title,
          wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT);

   CenterOnScreen();
}

RealtimeEffectUI::~RealtimeEffectUI()
{
}

bool RealtimeEffectUI::Populate(ShuttleGui &S)
{
   SetBackgroundColour(theTheme.Colour(clrTrackInfo));
   ClearBackground();

   PopulateOrExchange(S);

   return true;
}

void RealtimeEffectUI::PopulateOrExchange(ShuttleGui &S)
{
   S.SetBorder(5);
   S.StartVerticalLay(wxEXPAND | wxLEFT | wxRIGHT, 1);
   {
      S.StartHorizontalLay(wxEXPAND, 0);
      {
         AButton *btn;
         wxImage images[5];

         S.SetBorder(1);
         CreateImages(images, add_xpm);
         btn = safenew AButton(S.GetParent(), ID_Add, wxDefaultPosition, wxDefaultSize,
                               images[0], images[1], images[2], images[3], images[4],
                               false);
         btn->SetName(_("Add"));
         btn->SetToolTip(XO("Add effect to list"));
         S.AddWindow(btn);

         CreateImages(images, bypass_xpm);
         mBypass = safenew AButton(S.GetParent(), ID_Bypass, wxDefaultPosition, wxDefaultSize,
                               images[0], images[1], images[3], images[3], images[4],
                               true);
         mBypass->SetName(_("Bypass"));
         mBypass->SetToolTip(XO("Bypass entire list"));
         mList.IsBypassed() ? mBypass->PushDown() : mBypass->PopUp();
         S.AddWindow(mBypass);

         S.GetSizer()->AddStretchSpacer();

         S.SetBorder(5);
         mLatency = S.AddVariableText(XO("Latency: 0.000 ms"), false, wxALIGN_CENTER_VERTICAL | wxRIGHT);
         mLatency->SetForegroundColour(theTheme.Colour(clrTrackPanelText));
      }
      S.EndHorizontalLay();

      S.SetBorder(0);
      S.StartMultiColumn(NUMCOLS, wxALIGN_LEFT);
      {
         mMainSizer = static_cast<wxFlexGridSizer *>(S.GetSizer());
         mMainSizer->SetVGap(0);
         mMainSizer->SetHGap(0);
         mMainSizer->AddGrowableCol(COL_Name, 1);
      }
      S.EndMultiColumn();
   }
   S.EndVerticalLay();

   Layout();
   Fit();

   Rebuild();
}

void RealtimeEffectUI::Rebuild()
{
   mList.IsBypassed() ? mBypass->PushDown() : mBypass->PopUp();

   mMainSizer->Clear(true);

   auto & states = mList.GetStates();
   for (auto & state : states)
   {
      Add(*state);
   }
}

void RealtimeEffectUI::Add(RealtimeEffectState &state)
{
   AButton *btn;
 
   wxASSERT(this); // To justify safenew

   btn = CreateButton(ID_Power + mIDCounter,
                      _("Active State"), XO("Set effect active state"),
                      power_off_xpm, power_on_xpm,
                      true,                     
                      wxLEFT);
   state.IsBypassed() ? btn->PopUp() : btn->PushDown();

   btn = CreateButton(ID_Editor + mIDCounter,
                      _("Show/Hide Editor"), XO("Open/close effect editor"),
                      editor_xpm);

   btn = CreateButton(ID_PrePost + mIDCounter,
                      _("Post Fade"), XO("Apply effects post fader"),
                      pre_fade_xpm, post_fade_xpm,
                      true);
   if (state.IsPreFade())
   {
      btn->PopUp();
      btn->SetName(_("Pre Fade"));
      btn->SetToolTip(XO("Apply effects pre fader"));
   }
   else
   {
      btn->PushDown();
      btn->SetName(_("Post Fade"));
      btn->SetToolTip(XO("Apply effects post fader"));
   }

   btn = CreateButton(ID_Up + mIDCounter,
                      _("Move Up"), XO("Move effect up"),
                      move_up_xpm);

   btn = CreateButton(ID_Down + mIDCounter,
                      _("Move Down"), XO("Move effect down"),
                      move_down_xpm);

   btn = CreateButton(ID_Remove + mIDCounter,
                      _("Remove"), XO("Remove effect"),
                      remove_xpm, nullptr,
                      false,
                      wxRIGHT);

   wxStaticText *text = safenew wxStaticText(GetParent(), ID_Name + mIDCounter, state.GetEffect()->GetName().Translation());
   text->SetToolTip(_("Name of the effect"));
   text->SetForegroundColour(theTheme.Colour(clrTrackPanelText));
   mMainSizer->Add(text, 1, wxEXPAND | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 1);

   Layout();
   Fit();
   Update();
   mIDCounter++;

   if (!mTimer.IsRunning())
   {
      mTimer.Start(1000);
   }
}

void RealtimeEffectUI::OnClose(wxCommandEvent & evt)
{
   Show(false);
}

void RealtimeEffectUI::OnTimer(wxTimerEvent & WXUNUSED(evt))
{
   int latency = mManager->GetLatency();
   if (latency != mLastLatency)
   {
      mLatency->SetLabel(wxString::Format(_("Latency: %.3f ms"), (float) latency / 1000.0));
      mLatency->Refresh();

      mLastLatency = latency;
   }
}

void RealtimeEffectUI::OnAdd(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());
   btn->PopUp();

   PluginID id;
   Selector selector(this, id);

   if (selector.ShowModal() != wxID_CANCEL && !id.empty())
   {
      Add(mManager->AddState(mList, id));
   }
}

void RealtimeEffectUI::OnBypass(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());

   mList.Bypass(btn->IsDown());
   Rebuild();

   GetParent()->Refresh();
}

void RealtimeEffectUI::OnPower(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());
   auto powered = btn->IsDown();

   auto & state = mList.GetState(GetEffectIndex(btn));
   state.Bypass(!powered);
}

void RealtimeEffectUI::OnEditor(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());
   btn->PopUp();

   int index = GetEffectIndex(btn);
   if (index < 0)
   {
      return;
   }

   mList.GetState(GetEffectIndex(btn)).ShowEditor(GetParent());
}

void RealtimeEffectUI::OnPrePost(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());

   auto & state = mList.GetState(GetEffectIndex(btn));

   mList.SetPrefade(state, !btn->IsDown());

   if (state.IsPreFade())
   {
      btn->SetName(_("Pre Fade"));
      btn->SetToolTip(XO("Apply effects pre fader"));
   }
   else
   {
      btn->SetName(_("Post Fade"));
      btn->SetToolTip(XO("Apply effects post fader"));
   }
}

void RealtimeEffectUI::OnUp(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());
   btn->PopUp();

   int index = GetEffectIndex(btn);
   if (index <= 0)
   {
      return;
   }

   MoveRowUp(index);
}

void RealtimeEffectUI::OnDown(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());
   btn->PopUp();

   int index = GetEffectIndex(btn);
   if (index < 0 || index == (mMainSizer->GetChildren().GetCount() / NUMCOLS) - 1)
   {
      return;
   }

   MoveRowUp(index + 1);
}

void RealtimeEffectUI::OnRemove(wxCommandEvent & evt)
{
   AButton *btn =  static_cast<AButton *>(evt.GetEventObject());
   btn->PopUp();

   int index = GetEffectIndex(btn);
   if (index < 0)
   {
      return;
   }

   auto & state = mList.GetState(index);
   state.CloseEditor();
   
   mManager->RemoveState(mList, state);

   if (mList.GetStates().size() == 0)
   {
      if (mTimer.IsRunning())
      {
         mTimer.Stop();
      }
   }

   index *= NUMCOLS;

   for (int i = 0; i < NUMCOLS; i++)
   {
      // Do not allow the AButtons to be deleted here since we'll be returning
      // to the AButton mouse event handler. If we delete now, then we'll return
      // to an invalid object. So, queue the window deletions until after this
      // event has completed.
      auto w = mMainSizer->GetItem(index)->GetWindow();
      CallAfter([w]
      {
         w->Destroy();
      });
      mMainSizer->Remove(index);
   }

   mMainSizer->Layout();
   Fit();
}

void RealtimeEffectUI::UpdatePrefs()
{
   DestroyChildren();
   SetSizer(nullptr);
   SetMinSize(wxDefaultSize);

   ThemedDialog::Populate();
}

AButton *RealtimeEffectUI::CreateButton(wxWindowID id,
                                        wxString name,
                                        TranslatableString tip,
                                        const char *const *up,
                                        const char *const *down /* = nullptr */,
                                        bool toggle /* = false */,
                                        int style /* = 0 */,
                                        bool add /* = true */)
{
   AButton *btn;

   wxImage images[5];
   CreateImages(images, up, down);
   bool toggleHigh = toggle && down == nullptr;

   btn = safenew AButton(GetParent(), id, wxDefaultPosition, wxDefaultSize,
                         images[0], images[1], images[toggleHigh ? 3 : 2], images[3], images[4],
                         toggle);
   btn->SetName(name);
   btn->SetToolTip(tip);
   if (add)
   {
      mMainSizer->Add(btn, 0, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | style, 1);
   }

   return btn;
}

void RealtimeEffectUI::CreateImages(wxImage images[5],
                                    const char *const *up,
                                    const char *const *down /* = nullptr */)
{
   wxMemoryDC dc;

   auto fore = theTheme.Colour(clrTrackPanelText);
   auto back = theTheme.Colour(clrTrackInfo);

   wxImage imgUp(up);
   wxImage imgDown(down ? down : up);

   imgUp.Replace(wxBLACK->Red(), wxBLACK->Green(), wxBLACK->Blue(),
                 fore.Red(), fore.Green(), fore.Blue());
   imgDown.Replace(wxBLACK->Red(), wxBLACK->Green(), wxBLACK->Blue(),
                   fore.Red(), fore.Green(), fore.Blue());

   wxBitmap bmpUp(imgUp);
   wxBitmap bmpDown(imgDown);
   int offset = down ? 0 : 1;

   wxBitmap mod(bmpUp.GetWidth() + 1, bmpUp.GetHeight() + 1);
   dc.SelectObject(mod);

   //
   // Up image
   //
   AColor::Medium(&dc, false);
   dc.SetBackground(dc.GetBrush());
   dc.Clear();
   dc.DrawBitmap(bmpUp, 0, 0, true);
   images[0] = mod.ConvertToImage();

   //
   // Over up image
   //
   AColor::Medium(&dc, true);
   dc.SetBackground(dc.GetBrush());
   dc.Clear();
   dc.DrawBitmap(bmpUp, 0, 0, true);
   images[1] = mod.ConvertToImage();

   //
   // Down image
   //
   AColor::Medium(&dc, false);
   dc.SetBackground(dc.GetBrush());
   dc.Clear();
   dc.DrawBitmap(bmpDown, offset, offset, true);
   images[2] = mod.ConvertToImage();

   //
   // Over down image
   //
   AColor::Medium(&dc, true);
   dc.SetBackground(dc.GetBrush());
   dc.Clear();
   dc.DrawBitmap(bmpDown, offset, offset, true);
   images[3] = mod.ConvertToImage();

   //
   // Disabled image
   //
   images[4] = images[0].ConvertToDisabled();

   dc.SelectObject(wxNullBitmap);

   return;
}

int RealtimeEffectUI::GetEffectIndex(wxWindow *win)
{
   int col = (win->GetId() - ID_Base) / ID_Range;
   int row;
   int cnt = mMainSizer->GetChildren().GetCount() / NUMCOLS;
   for (row = 0; row < cnt; ++row)
   {
      wxSizerItem *si = mMainSizer->GetItem((row * NUMCOLS) + col);
      if (si->GetWindow() == win)
      {
         break;
      }
   }

   if (row == cnt)
   {
      return -1;
   }

   return row;
}

void RealtimeEffectUI::MoveRowUp(int row)
{
   mList.Swap(row, row - 1);

   row *= NUMCOLS;

   for (int i = 0; i < NUMCOLS; i++)
   {
      wxSizerItem *si = mMainSizer->GetItem(row + NUMCOLS - 1);
      wxWindow *w = si->GetWindow();
      int flags = si->GetFlag();
      int border = si->GetBorder();
      int prop = si->GetProportion();
      mMainSizer->Detach(row + NUMCOLS - 1);
      mMainSizer->Insert(row - NUMCOLS, w, prop, flags, border);
      w->Refresh();
   }

   mMainSizer->Layout();
   Refresh();
}

BEGIN_EVENT_TABLE(RealtimeEffectUI::Selector, ThemedDialog)
   EVT_GRID_CELL_LEFT_DCLICK(RealtimeEffectUI::Selector::OnDClick)
   EVT_GRID_COL_SORT(RealtimeEffectUI::Selector::OnSort)
   EVT_KEY_DOWN(RealtimeEffectUI::Selector::OnKey)
END_EVENT_TABLE()

RealtimeEffectUI::Selector::Selector(wxWindow *parent, PluginID & id)
:  ThemedDialog(),
   mID(id)
{
   mID = PluginID();
   SetReturnCode(wxNOT_FOUND);

   mEffects = nullptr;
   mSortColumn = -1;
   mSortDirection = true;

   Create(parent, wxID_ANY, XO("Select Effect"), wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER);

   CenterOnScreen();
}

RealtimeEffectUI::Selector::~Selector()
{
}

bool RealtimeEffectUI::Selector::Populate(ShuttleGui &S)
{
   PopulateOrExchange(S);

   DoSort(COL_Effect);

   if (mEffects->GetNumberRows())
   {
      mEffects->SetGridCursor(0, 0);
   }

   return true;
}

/// Defines the dialog and does data exchange with it.
void RealtimeEffectUI::Selector::PopulateOrExchange(ShuttleGui &S)
{
   SetForegroundColour(theTheme.Colour(clrTrackPanelText));
   SetBackgroundColour(theTheme.Colour(clrTrackInfo));

   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxALIGN_LEFT | wxEXPAND, 0);
      {
         S.AddTitle(XXO("Double click the desired effect"));
      }
      S.EndHorizontalLay();

      S.StartHorizontalLay(wxALIGN_LEFT | wxEXPAND, 1);
      {
         mEffects = safenew Grid(S.GetParent(), wxID_ANY);
         mEffects->Bind(wxEVT_KEY_DOWN, &RealtimeEffectUI::Selector::OnKey, this);

         mEffects->UseNativeColHeader(false);
         mEffects->SetColLabelSize(mEffects->GetDefaultRowSize());
         mEffects->SetLabelBackgroundColour(theTheme.Colour(clrTrackInfoSelected));
         mEffects->SetLabelTextColour(theTheme.Colour(clrTrackPanelText));
         
         // Build the initial (empty) grid
         mEffects->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
         mEffects->DisableDragColMove();
         mEffects->DisableDragRowSize();
         mEffects->SetRowLabelSize(0);
         mEffects->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_CENTER);
         mEffects->SetColLabelValue(0, _("Name"));
         mEffects->SetColLabelValue(1, _("Type"));
         mEffects->SetColLabelValue(2, _("Path"));

         S.Prop(1).Position(wxEXPAND).AddWindow(mEffects);
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();

   auto & pm = PluginManager::Get();
   auto & em = EffectManager::Get();
   for (const auto &plug : pm.PluginsOfType(EffectTypeProcess)) {
      if (plug.IsEnabled() && plug.IsEffectRealtime())
      {
         ItemData item;

         item.id = plug.GetID();
         item.name = plug.GetSymbol().Translation();
         item.type = plug.GetEffectFamily();
         item.path = plug.GetPath();

         mItems.push_back(item);
      }
   }

   RegenerateEffectsList();

   if (mEffects->GetNumberRows())
   {
      // Make sure first item is selected/focused.
      mEffects->SetFocus();
      mEffects->GoToCell(0, COL_Effect);
   }

   mEffects->Fit();

   auto sz = mEffects->GetBestSize();
   auto r = wxGetClientDisplayRect();
   r.height = wxMin(sz.y, (r.height / 3) * 2);
   if (sz.y > r.height)
   {
      sz.x += wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
   }
   mEffects->SetMinSize({sz.x, r.height});

   Layout();
   Fit();
}

void RealtimeEffectUI::Selector::RegenerateEffectsList()
{
   auto row = mEffects->GetGridCursorRow();
   auto col = mEffects->GetGridCursorCol();
   auto path = row >= 0 ? mEffects->GetCellValue(row, COL_Path) : wxString();

   auto rows = mEffects->GetNumberRows();
   if (rows)
   {
      mEffects->DeleteRows(0, rows);
   }
   mEffects->InsertRows(0, mItems.size());

   int i = 0;
   for (auto & item : mItems)
   {
      mEffects->SetCellValue(i, COL_Effect, item.name);
      mEffects->SetCellValue(i, COL_Type, item.type);
      mEffects->SetCellValue(i, COL_Path, item.path);

      mEffects->SetReadOnly(i, COL_Effect);
      mEffects->SetReadOnly(i, COL_Type);
      mEffects->SetReadOnly(i, COL_Path);

      if (path == item.path)
      {
         mEffects->GoToCell(i, col);
      }
      i++;
   }
}

void RealtimeEffectUI::Selector::Exit(bool returnID)
{
   auto selected = mEffects->GetGridCursorRow();

   if (!returnID)
   {
      selected = wxNOT_FOUND;
   }

   if (selected != wxNOT_FOUND)
   {
      mID = mItems[selected].id;
   }
   else
   {
      mID = PluginID();
   }

   EndModal(selected);
}

void RealtimeEffectUI::Selector::DoSort(int col)
{
   if (col != mSortColumn)
   {
      mSortDirection = true;
   }
   else
   {
      mSortDirection = !mSortDirection;
   }

   mSortColumn = col;

   std::sort(mItems.begin(), mItems.end(), 
      [&](const ItemData & a, const ItemData & b) -> bool
      {
         if (col == COL_Effect)
         {
            return mSortDirection ? a.name < b.name : a.name > b.name;
         }
         if (col == COL_Type)
         {
            return mSortDirection ? a.type < b.type : a.type > b.type;
         }
         if (col == COL_Path)
         {
            return mSortDirection ? a.path < b.path : a.path > b.path;
         }
         return false;
      });

   RegenerateEffectsList();

   mEffects->Refresh();
}

void RealtimeEffectUI::Selector::OnSort(wxGridEvent &evt)
{
   DoSort(evt.GetCol());
}

void RealtimeEffectUI::Selector::OnKey(wxKeyEvent &evt)
{
   switch (evt.GetKeyCode())
   {
      case WXK_ESCAPE:
         Exit(false);
      break;

      case WXK_RETURN:
         Exit(true);
      break;

      default:
         evt.Skip();
      break;
   }
}

void RealtimeEffectUI::Selector::OnDClick(wxGridEvent &evt)
{
   Exit(true);
}

bool RealtimeEffectUI::Selector::IsEscapeKey(const wxKeyEvent &evt)
{
   if (evt.GetKeyCode() == WXK_ESCAPE)
   {
      Exit(false);
   }

   return false;
}

