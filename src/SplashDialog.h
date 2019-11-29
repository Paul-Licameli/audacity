/**********************************************************************

  Audacity: A Digital Audio Editor

  SplashDialog.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_SPLASH_DLG__
#define __AUDACITY_SPLASH_DLG__

#include "widgets/wxPanelWrapper.h" // to inherit

class wxBitmap;
class ShuttleGui;
class AudacityProject;
class HtmlWindow;

class SplashDialog final : public wxDialogWrapper {
   DECLARE_DYNAMIC_CLASS(SplashDialog)
public:

   static void DoHelpWelcome( AudacityProject &project );

   SplashDialog(wxWindow * parent);
   virtual ~ SplashDialog();
   void OnOK();
   static void Show2( wxWindow * pParent );

   DECLARE_EVENT_TABLE()

private:

   void OnChar(wxMouseEvent &event);
   void Populate( ShuttleGui & S );
   void OnDontShow( wxCommandEvent & Evt );

   HtmlWindow * mpHtml;
   static SplashDialog * pSelf;
};

#endif
