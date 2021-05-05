/**********************************************************************

  Audacity: A Digital Audio Editor

  ThemedDialog.h

**********************************************************************/

#ifndef __AUDACITY_THEMED_DIALOG__
#define __AUDACITY_THEMED_DIALOG__

#include "wxPanelWrapper.h"

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

class AButton;
class ShuttleGui;

class AUDACITY_DLL_API ThemedDialog : public wxDialogWrapper
{
public:
   ThemedDialog();
   ThemedDialog(wxWindow *parent, wxWindowID id,
                const TranslatableString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0);
   virtual ~ThemedDialog();

   // Pseudo ctor
   bool Create(wxWindow *parent, wxWindowID id,
               const TranslatableString& title,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0);

   wxWindow *GetParent();
   wxSizer *GetSizer();

   virtual bool Populate();
   virtual bool Populate(ShuttleGui &S) = 0;

   virtual void OnClose(wxCommandEvent &evt);

private:
   class Border : public wxWindow
   {
   public:
      Border(wxWindow *parent,
             wxWindowID id,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = 0,
             const wxString& name = wxPanelNameStr)
      :  wxWindow(parent, id, pos, size, style, name)
      {
      }

      // We don't need or want to accept focus.
      bool AcceptsFocus() const override
      {
         return false;
      }

      bool AcceptsFocusFromKeyboard() const override
      {
         return false;
      }
   };

   Border *AddBorder(wxBoxSizer *sizer, wxSize sz, wxStockCursor cursor);
   void InstallHook(wxWindow *win);

   AButton *CreateCloseButton(wxWindow *parent);

   void OnSize(wxSizeEvent &evt);
   void OnActivate(wxActivateEvent &evt);
   void OnLeftDown(wxMouseEvent &evt);
   void OnLeftUp(wxMouseEvent &evt);
   void OnMotion(wxMouseEvent &evt);
   void OnCaptureLost(wxMouseCaptureLostEvent &evt);

private:
   TranslatableString mTitle;
   bool mWantResizers;

   wxPanel *mTitleBar;
   wxStaticText *mTitleText;
   AButton *mClose;

   wxSizer *mSizer;

   Border *mLeftBorder;
   Border *mTopBorder;
   Border *mRightBorder;
   Border *mBottomBorder;
   wxPanel *mContent;

   wxWindow *mDragWin;
   wxRect mDragRect;
   wxPoint mDragPos;
   wxPoint mDragOffset;
   bool mDragging;

   DECLARE_EVENT_TABLE();
};

#endif
