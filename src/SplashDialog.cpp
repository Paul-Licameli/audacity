/**********************************************************************

  Audacity: A Digital Audio Editor

  SplashDialog.cpp

  James Crook

********************************************************************//**

\class SplashDialog
\brief The SplashDialog shows help information for Audacity when
Audacity starts up.

It was written for the benefit of NEW users who do not want to
read the manual.  The text of the dialog is kept short to increase the
chance of it being read.  The content is designed to reduce the
most commonly asked questions about Audacity.

*//********************************************************************/



#include "SplashDialog.h"



#include <wx/frame.h>
#include <wx/html/htmlwin.h>
#include <wx/statbmp.h>

#include "FileNames.h"
#include "Project.h"
#include "ShuttleGui.h"
#include "widgets/AudacityMessageBox.h"
#include "widgets/HelpSystem.h"

#include "AllThemeResources.h"
#include "Prefs.h"
#include "prefs/GUIPrefs.h"
#include "HelpText.h"

// DA: Logo for Splash Dialog (welcome dialog)
#ifdef EXPERIMENTAL_DA
#include "../images/DarkAudacityLogoWithName.xpm"
#else
#include "../images/AudacityLogoWithName.xpm"
#endif

SplashDialog * SplashDialog::pSelf=NULL;

enum
{
   DontShowID=1000,
};

BEGIN_EVENT_TABLE(SplashDialog, wxDialogWrapper)
   EVT_CHECKBOX( DontShowID, SplashDialog::OnDontShow )
END_EVENT_TABLE()

IMPLEMENT_CLASS(SplashDialog, wxDialogWrapper)

void SplashDialog::DoHelpWelcome( AudacityProject &project )
{
   Show2( &GetProjectFrame( project ) );
}

SplashDialog::SplashDialog(wxWindow * parent)
   :  wxDialogWrapper(parent, -1, XO("Welcome to Audacity!"),
      wxPoint( -1, 60 ), // default x position, y position 60 pixels from top of screen.
      wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   SetName();
   ShuttleGui S( this );
   Populate( S );
   Fit();
   this->Centre();
   int x,y;
   GetPosition( &x, &y );
   Move( x, 60 );
}

void SplashDialog::OnChar(wxMouseEvent &event)
{
   if ( event.ShiftDown() && event.ControlDown() )
      wxLaunchDefaultBrowser("https://www.audacityteam.org");
}

void SplashDialog::Populate( ShuttleGui & S )
{
   auto bShow = GUIShowSplashScreen.Read();

   const float fScale=0.5f;// smaller size.

   S.StartVerticalLay(1);

   wxStaticBitmap *icon{};
   S
      .Prop(0)
#if  (0)
      .ConnectRoot( wxEVT_LEFT_DOWN, &SplashDialog::OnChar)
#endif
      .Window<wxStaticBitmap>(
      //*m_pLogo, //v theTheme.Bitmap(bmpAudacityLogoWithName),
      [=]{
         //v For now, change to AudacityLogoWithName via old-fashioned ways, not Theme.
         auto pLogo = std::make_unique<wxBitmap>(
            (const char **) AudacityLogoWithName_xpm); //v

         // JKC: Resize to 50% of size.  Later we may use a smaller xpm as
         // our source, but this allows us to tweak the size - if we want to.
         // It also makes it easier to revert to full size if we decide to.
         wxImage RescaledImage( pLogo->ConvertToImage() );
         wxColour MainColour(
            RescaledImage.GetRed(1,1),
            RescaledImage.GetGreen(1,1),
            RescaledImage.GetBlue(1,1));
         this->SetBackgroundColour(MainColour);

         // wxIMAGE_QUALITY_HIGH not supported by wxWidgets 2.6.1, or we would use it here.
         RescaledImage.Rescale( (int)(LOGOWITHNAME_WIDTH * fScale), (int)(LOGOWITHNAME_HEIGHT *fScale) );
         // Note: wxBitmap is reference-counted and cheap to copy
         return wxBitmap { RescaledImage };
      }(),
      wxDefaultPosition,
      wxSize((int)(LOGOWITHNAME_WIDTH*fScale),
             (int)(LOGOWITHNAME_HEIGHT*fScale)) );

   S
      .Prop(1)
      .Position( wxEXPAND )
      .Window( [=](wxWindow *parent, wxWindowID winid ){
         auto html = safenew LinkingHtmlWindow(parent, winid,
                                               wxDefaultPosition,
                                               wxSize(506, 280),
                                               wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER );
         html->SetPage(HelpText( L"welcome" ));
         return html;
      } )
      .Assign(mpHtml);

   // S.Prop(0); // PRL: was here, but had no effect
   S.StartMultiColumn(2, GroupOptions{ wxEXPAND }.StretchyColumn(1));
   {
      S.SetBorder( 5 );

      S
         .Id( DontShowID)
         .AddCheckBox( XXO("Don't show this again at start up"), !bShow );

      S.SetBorder( 5 );

      S
         .Id(wxID_OK)
         .Prop(0)
         .Action( [this]{ OnOK(); } )
         .AddButton(XXO("OK"), wxALIGN_RIGHT| wxALL, true);
   }
   S.EndVerticalLay();
}

SplashDialog::~SplashDialog()
{
}

void SplashDialog::OnDontShow( wxCommandEvent & Evt )
{
   bool bShow = !Evt.IsChecked();
   GUIShowSplashScreen.Write( bShow );
   gPrefs->Flush();
}

void SplashDialog::OnOK()
{
   Show( false );

}

void SplashDialog::Show2( wxWindow * pParent )
{
   if( pSelf == NULL )
   {
      // pParent owns it
      wxASSERT(pParent);
      pSelf = safenew SplashDialog( pParent );
   }
   pSelf->mpHtml->SetPage(HelpText( L"welcome" ));
   pSelf->Show( true );
}
