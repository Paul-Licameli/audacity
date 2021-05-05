/**********************************************************************

  Audacity: A Digital Audio Editor

  ThemedDialog.cpp

**********************************************************************/

#include "ThemedDialog.h"

#include "../AColor.h"
#include "../AllThemeResources.h"
#include "../ShuttleGui.h"
#include "../Theme.h"
#include "AButton.h"

#include <wx/dcmemory.h>
#include <wx/bitmap.h>
#include <wx/frame.h>
#include <wx/image.h>
#include <wx/sizer.h>

static const char * const close_xpm[] =
{
   "12 14 2 1",
   "  c None",
   ". c #000000",
   "            ",
   "            ",
   "            ",
   "            ",
   "  ..    ..  ",
   "   ..  ..   ",
   "    ....    ",
   "     ..     ",
   "    ....    ",
   "   ..  ..   ",
   "  ..    ..  ",
   "            ",
   "            ",
   "            "
};

static const int BorderWidth = 2;

BEGIN_EVENT_TABLE(ThemedDialog, wxDialogWrapper)
   EVT_SIZE(ThemedDialog::OnSize)
   EVT_ACTIVATE(ThemedDialog::OnActivate)
   EVT_LEFT_DOWN(ThemedDialog::OnLeftDown)
   EVT_LEFT_UP(ThemedDialog::OnLeftUp)
   EVT_MOTION(ThemedDialog::OnMotion)
   EVT_MOUSE_CAPTURE_LOST(ThemedDialog::OnCaptureLost)
   EVT_BUTTON(wxID_CLOSE, ThemedDialog::OnClose)
END_EVENT_TABLE()

ThemedDialog::ThemedDialog()
{
}

ThemedDialog::ThemedDialog(wxWindow *parent,
                           wxWindowID id,
                           const TranslatableString& title,
                           const wxPoint& pos /* = wxDefaultPosition */,
                           const wxSize& size /* = wxDefaultSize */,
                           long style /* = 0 */)
:  wxDialogWrapper()
{
   Create(parent, id, title, pos, size, style);
}

ThemedDialog::~ThemedDialog()
{
}

bool ThemedDialog::Create(wxWindow *parent,
                          wxWindowID id,
                          const TranslatableString& title,
                          const wxPoint& pos /* = wxDefaultPosition */,
                          const wxSize& size /* = wxDefaultSize */,
                          long style /* = 0 */)
{
   mWantResizers = style & wxRESIZE_BORDER;
   style &= ~wxRESIZE_BORDER;

   wxDialogWrapper::Create(parent, id, title, pos, size, style | wxBORDER_SIMPLE | wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT);
   mTitle = title;

   return Populate();
}

bool ThemedDialog::Populate()
{
   SetBackgroundColour(theTheme.Colour(clrTrackInfo));
   ClearBackground();

   auto vs = safenew wxBoxSizer(wxVERTICAL);

   mTopBorder = AddBorder(vs, {wxDefaultCoord, BorderWidth}, wxCURSOR_SIZENS);

   auto hs = safenew wxBoxSizer(wxHORIZONTAL);

   mLeftBorder = AddBorder(hs, {BorderWidth, wxDefaultCoord}, wxCURSOR_SIZEWE);

   auto cs = safenew wxBoxSizer(wxVERTICAL);

   mTitleBar = safenew wxPanelWrapper(this, wxID_ANY);
   mTitleBar->SetForegroundColour(theTheme.Colour(clrTrackPanelText));
   mTitleBar->SetBackgroundColour(theTheme.Colour(clrTrackInfo));
   InstallHook(mTitleBar);

   auto ts = safenew wxBoxSizer(wxHORIZONTAL);

   mTitleText = safenew wxStaticText(mTitleBar, wxID_ANY, mTitle.Translation());
   InstallHook(mTitleText);
   ts->Add(mTitleText, 0, wxEXPAND | wxALIGN_LEFT);

   ts->AddStretchSpacer();

   mClose = CreateCloseButton(mTitleBar);
   mClose->SetName(_("Close"));
   mClose->SetToolTip(XO("Close the effects list"));
   if (IsActive())
   {
      mClose->PopUp();
   }
   else
   {
      mClose->PushDown();
   }
   ts->Add(mClose, 0, wxALIGN_CENTER_VERTICAL);

   mTitleBar->SetSizer(ts);

   cs->Add(mTitleBar, 0, wxEXPAND);

   mContent = safenew wxPanelWrapper(this, wxID_ANY);
   mContent->SetForegroundColour(theTheme.Colour(clrTrackPanelText));
   mContent->SetBackgroundColour(theTheme.Colour(clrTrackInfo));
   mContent->ClearBackground();

   cs->Add(mContent, 1, wxEXPAND);

   hs->Add(cs, 1, wxEXPAND);

   mRightBorder = AddBorder(hs, {BorderWidth, wxDefaultCoord}, wxCURSOR_SIZEWE);

   vs->Add(hs, 1, wxEXPAND);

   mBottomBorder = AddBorder(vs, {wxDefaultCoord, BorderWidth}, wxCURSOR_SIZENS);

   SetSizer(vs);

   ShuttleGui S(mContent, eIsCreating, true, wxDefaultSize);
   S.SetThemed(true);

   mSizer = S.GetSizer();

   if (!Populate(S))
   {
      return false;
   }

   Layout();
   Fit();

   return true;
}

wxWindow *ThemedDialog::GetParent()
{
   return mContent;
}

wxSizer *ThemedDialog::GetSizer()
{
   return mSizer;
}

ThemedDialog::Border *ThemedDialog::AddBorder(wxBoxSizer *sizer, wxSize sz, wxStockCursor cursor)
{
   auto border = safenew Border(this, wxID_ANY,
                                wxDefaultPosition,
                                wxSize(BorderWidth, wxDefaultCoord));
   border->SetBackgroundColour(theTheme.Colour(clrTrackInfo));
   border->ClearBackground();

   if (mWantResizers)
   {
      border->SetCursor(cursor);
      InstallHook(border);
   }
   sizer->Add(border, 0, wxEXPAND);

   return border;
}

void ThemedDialog::InstallHook(wxWindow *win)
{
   win->Bind(wxEVT_LEFT_DOWN, &ThemedDialog::OnLeftDown, this);
   win->Bind(wxEVT_LEFT_UP, &ThemedDialog::OnLeftUp, this);
   win->Bind(wxEVT_MOTION, &ThemedDialog::OnMotion, this);
   win->Bind(wxEVT_MOUSE_CAPTURE_LOST, &ThemedDialog::OnCaptureLost, this);
}

void ThemedDialog::OnSize(wxSizeEvent & evt)
{
   evt.Skip();

   if (!mDragging)
   {
      Layout();
      Fit();
   }
}

void ThemedDialog::OnActivate(wxActivateEvent & evt)
{
   if (evt.GetActive())
   {
      mTitleBar->SetBackgroundColour(theTheme.Colour(clrTrackInfoSelected));
      mClose->PopUp();
   }
   else
   {
      mTitleBar->SetBackgroundColour(theTheme.Colour(clrTrackInfo));
      mClose->PushDown();
   }

   mTitleBar->Refresh();
}

void ThemedDialog::OnLeftDown(wxMouseEvent & evt)
{
   mDragWin = static_cast<wxWindow *>(evt.GetEventObject());
   mDragRect = GetScreenRect();
   mDragPos = mDragWin->ClientToScreen(evt.GetPosition());
   mDragOffset = mDragPos - GetPosition();
   mDragging = true;

   SetFocus();
   if (!HasCapture())
   {
      CaptureMouse();
   }
}

void ThemedDialog::OnLeftUp(wxMouseEvent & evt)
{
   mDragging = false;

   if (HasCapture())
   {
      ReleaseMouse();
   }
}

void ThemedDialog::OnMotion(wxMouseEvent & evt)
{
   if (evt.LeftIsDown() && evt.Dragging() && mDragging)
   {
      auto pos = ClientToScreen(evt.GetPosition()) - mDragOffset;
      auto diffX = pos.x - mDragRect.x;
      auto diffY = pos.y - mDragRect.y;

      if (mDragWin == mLeftBorder)
      {
         SetSize(pos.x, mDragRect.y, mDragRect.width - diffX, mDragRect.height);
      }
      else if (mDragWin == mRightBorder)
      {
         SetSize(mDragRect.width + diffX, mDragRect.height);
      }
      else if (mDragWin == mTopBorder)
      {
         SetSize(mDragRect.x, pos.y, mDragRect.width, mDragRect.height - diffY);
      }
      else if (mDragWin == mBottomBorder)
      {
         SetSize(mDragRect.width, mDragRect.height + diffY);
      }
      else
      {
         Move(pos);
      }

      mDragPos = pos;
   }
}

void ThemedDialog::OnCaptureLost(wxMouseCaptureLostEvent & evt)
{
   mDragging = false;
}

AButton *ThemedDialog::CreateCloseButton(wxWindow *parent)
{
   wxMemoryDC dc;
   wxImage images[5];

   wxImage img(close_xpm);

   auto fore = theTheme.Colour(clrTrackPanelText);
   img.Replace(wxBLACK->Red(), wxBLACK->Green(), wxBLACK->Blue(),
               fore.Red(), fore.Green(), fore.Blue());

   wxBitmap bmp(img);

   wxBitmap mod(bmp.GetWidth() + 1, bmp.GetHeight() + 1);
   dc.SelectObject(mod);

   //
   // Up image
   //
   dc.SetBackground(theTheme.Colour(clrTrackInfoSelected));
   dc.Clear();
   dc.DrawBitmap(bmp, 0, 0, true);
   images[0] = mod.ConvertToImage();

   //
   // Over up image
   //
   images[1] = images[0];

   //
   // Down image
   //
   dc.SetBackground(theTheme.Colour(clrTrackInfo));
   dc.Clear();
   dc.DrawBitmap(bmp, 0, 0, true);
   images[2] = mod.ConvertToImage();

   //
   // Over down image
   //
   images[3] = images[2];

   //
   // Disabled image
   //
   images[4] = images[0].ConvertToDisabled();

   dc.SelectObject(wxNullBitmap);

   return safenew AButton(parent, wxID_CLOSE, wxDefaultPosition, wxDefaultSize,
                          images[0], images[1], images[2], images[3], images[4],
                          false);
}

void ThemedDialog::OnClose(wxCommandEvent &evt)
{
   Close();
}
