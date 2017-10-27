/**********************************************************************

  Audacity: A Digital Audio Editor

  AboutDialog.cpp

  Dominic Mazzoni
  Vaughan Johnson
  James Crook

********************************************************************//**

\class AboutDialog
\brief The AboutDialog shows the program version and developer credits.

It is a simple scrolling window with an 'OK... Audacious!' button to
close it.

*//*****************************************************************//**

\class AboutDialogCreditItem
\brief AboutDialogCreditItem is a structure used by the AboutDialog to
hold information about one contributor to Audacity.

*//********************************************************************/



#include "AboutDialog.h"



#include <wx/html/htmlwin.h>
#include <wx/statbmp.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>

#include "FileNames.h"
#include "HelpText.h"
#include "ShuttleGui.h"
#include "widgets/HelpSystem.h"

#include "AllThemeResources.h"
#include "Theme.h"

// DA: Logo for About box.
#ifdef EXPERIMENTAL_DA
#include "../images/DarkAudacityLogoWithName.xpm"
#else
#include "../images/AudacityLogoWithName.xpm"
#endif

// Notice this is a "system include".  This is on purpose and only until
// we convert over to CMake.  Once converted, the "RevisionIndent.h" file
// should be deleted and this can be changed back to a user include if
// desired.
//
// RevisionIdent.h may contain #defines like these ones:
//#define REV_LONG "28864acb238cb3ca71dda190a2d93242591dd80e"
//#define REV_TIME "Sun Apr 12 12:40:22 2015 +0100"
#include "RevisionIdent.h"

#ifndef REV_TIME
#define REV_TIME "unknown date and time"
#endif

#ifdef REV_LONG
#define REV_IDENT wxString( "[[https://github.com/audacity/audacity/commit/" )+ REV_LONG + "|" + wxString( REV_LONG ).Left(6) + "]] of " +  REV_TIME 
#else
#define REV_IDENT (XO("No revision identifier was provided").Translation())
#endif

// To substitute into many other translatable strings
static const auto ProgramName =
   //XO("Audacity");
   Verbatim("Audacity");

void AboutDialog::CreateCreditsList()
{
   const auto sysAdminFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, system administration");
   const auto coFounderFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, co-founder and developer");
   const auto developerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, developer");
   const auto developerAndSupprtFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, developer and support");
   const auto documentationAndSupportFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, documentation and support");
   const auto qaDocumentationAndSupportFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, QA tester, documentation and support");
   const auto documentationAndSupportFrenchFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, documentation and support, French");
   const auto qualityAssuranceFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, quality assurance");
   const auto accessibilityAdvisorFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, accessibility advisor");
   const auto graphicArtistFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, graphic artist");
   const auto composerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, composer");
   const auto testerFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, tester");
   const auto NyquistPluginsFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, Nyquist plug-ins");
   const auto webDeveloperFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, web developer");
   const auto graphicsFormat =
   /* i18n-hint: For "About Audacity..." credits, substituting a person's proper name */
      XO("%s, graphics");

   // The Audacity Team: developers and support
   AddCredit(L"James Crook", developerFormat, roleTeamMember);
   AddCredit(L"Roger Dannenberg", coFounderFormat, roleTeamMember);
   AddCredit(L"Steve Daulton", roleTeamMember);
   AddCredit(L"Anton Gerasimov", developerFormat, roleTeamMember);
   AddCredit(L"Greg Kozikowski", documentationAndSupportFormat, roleTeamMember);
   AddCredit(L"Paul Licameli", developerFormat, roleTeamMember);
   AddCredit(L"Vitaly Sverchinsky", developerFormat, roleTeamMember);
   AddCredit(L"Dmitry Vedenko", developerFormat, roleTeamMember);

   // Emeritus: people who were "lead developers" or made an
   // otherwise distinguished contribution, but who are no
   // longer active.
   AddCredit(
      L"[[https://wiki.audacityteam.org/wiki/User:Galeandrews|Gale Andrews]]",
      qualityAssuranceFormat, roleEmeritusTeam);
   AddCredit(L"Richard Ash", developerFormat, roleEmeritusTeam);
   AddCredit(L"Christian Brochec",
      documentationAndSupportFrenchFormat, roleEmeritusTeam);
   AddCredit(L"Matt Brubeck", developerFormat, roleEmeritusTeam);
   AddCredit(L"Arturo \"Buanzo\" Busleiman", sysAdminFormat, roleEmeritusTeam);
   AddCredit(L"Michael Chinen", developerFormat, roleEmeritusTeam);
   AddCredit(L"Al Dimond", developerFormat, roleEmeritusTeam);
   AddCredit(L"Benjamin Drung", developerFormat, roleEmeritusTeam);
   AddCredit(L"Joshua Haberman", developerFormat, roleEmeritusTeam);
   AddCredit(L"Ruslan Ijbulatov", developerFormat, roleEmeritusTeam);
   AddCredit(L"Vaughan Johnson", developerFormat, roleEmeritusTeam);
   AddCredit(L"Leland Lucius", developerFormat, roleEmeritusTeam);
   AddCredit(L"Dominic Mazzoni", coFounderFormat, roleEmeritusTeam);
   AddCredit(L"Markus Meyer", developerFormat, roleEmeritusTeam);
   AddCredit(L"Monty Montgomery", developerFormat, roleEmeritusTeam);
   AddCredit(L"Shane Mueller", developerFormat, roleEmeritusTeam);
   AddCredit(L"Tony Oetzmann", documentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Alexandre Prokoudine", documentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Peter Sampson", qaDocumentationAndSupportFormat, roleEmeritusTeam);
   AddCredit(L"Martyn Shaw", developerFormat, roleEmeritusTeam);
   AddCredit(L"Bill Wharrie", documentationAndSupportFormat, roleEmeritusTeam);

   // Contributors
   AddCredit(L"Lynn Allan", developerFormat, roleContributor);
   AddCredit(L"Brian Armstrong", developerFormat, roleContributor);
   AddCredit(L"David Avery", developerFormat, roleContributor);
   AddCredit(L"David Bailes", accessibilityAdvisorFormat, roleContributor);
   AddCredit(L"William Bland", developerFormat, roleContributor);
   AddCredit(L"Sami Boukortt", developerFormat, roleContributor);
   AddCredit(L"Jeremy R. Brown", developerFormat, roleContributor);
   AddCredit(L"Alex S. Brown", developerFormat, roleContributor);
   AddCredit(L"Chris Cannam", developerFormat, roleContributor);
   AddCredit(L"Cory Cook", developerFormat, roleContributor);
   AddCredit(L"Craig DeForest", developerFormat, roleContributor);
   AddCredit(L"Edgar Franke (Edgar-RFT)", developerFormat, roleContributor);
   AddCredit(L"Mitch Golden", developerFormat, roleContributor);
   AddCredit(L"Brian Gunlogson", developerFormat, roleContributor);
   AddCredit(L"Andrew Hallendorff", developerFormat, roleContributor);
   AddCredit(L"Robert H\u00E4nggi", developerFormat, roleContributor);
   AddCredit(L"Daniel Horgan", developerFormat, roleContributor);
   AddCredit(L"David Hostetler", developerFormat, roleContributor);
   AddCredit(L"Steve Jolly", developerFormat, roleContributor);
   AddCredit(L"Steven Jones", developerFormat, roleContributor);
   AddCredit(L"Henric Jungheim", developerFormat, roleContributor);
   AddCredit(L"Myungchul Keum", developerFormat, roleContributor);
   AddCredit(L"Arun Kishore", developerFormat, roleContributor);
   AddCredit(L"Paul Livesey", developerFormat, roleContributor);
   AddCredit(L"Harvey Lubin", graphicArtistFormat, roleContributor);
   AddCredit(L"Max Maisel", developerFormat, roleContributor);
   AddCredit(L"Greg Mekkes", developerFormat, roleContributor);
   AddCredit(L"Abe Milde", developerFormat, roleContributor);
   AddCredit(L"Paul Nasca", developerFormat, roleContributor);
   AddCredit(L"Clayton Otey", developerFormat, roleContributor);
   AddCredit(L"Mark Phillips", developerFormat, roleContributor);
   AddCredit(L"Andr\u00E9 Pinto", developerFormat, roleContributor);
   AddCredit(L"Jean Claude Risset", composerFormat, roleContributor);
   AddCredit(L"Augustus Saunders", developerFormat, roleContributor);
   AddCredit(L"Benjamin Schwartz", developerFormat, roleContributor);
   AddCredit(L"Cliff Scott", testerFormat, roleContributor);
   AddCredit(L"David R. Sky", NyquistPluginsFormat, roleContributor);
   AddCredit(L"Rob Sykes", developerFormat, roleContributor);
   AddCredit(L"Mike Underwood", developerFormat, roleContributor);
   AddCredit(L"Philip Van Baren", developerFormat, roleContributor);
   AddCredit(L"Salvo Ventura", developerFormat, roleContributor);
   AddCredit(L"Darrell Walisser", developerFormat, roleContributor);
   AddCredit(L"Jun Wan", developerFormat, roleContributor);
   AddCredit(L"Daniel Winzen", developerFormat, roleContributor);
   AddCredit(L"Tom Woodhams", developerFormat, roleContributor);
   AddCredit(L"Mark Young", developerFormat, roleContributor);
   AddCredit(L"Wing Yu", developerFormat, roleContributor);

   // Website and Graphics
   AddCredit(L"Shinta Carolinasari", webDeveloperFormat, roleGraphics);
   AddCredit(L"Bayu Rizaldhan Rayes", graphicsFormat, roleGraphics);

   // Libraries

   AddCredit(L"[[https://libexpat.github.io/|expat]]", roleLibrary);
   AddCredit(L"[[https://xiph.org/flac/|FLAC]]", roleLibrary);
   AddCredit(L"[[http://lame.sourceforge.net/|LAME]]", roleLibrary);
   AddCredit(L"[[https://www.underbit.com/products/mad/|libmad]]", roleLibrary);
   AddCredit(L"[[http://www.mega-nerd.com/libsndfile/|libsndfile]]", roleLibrary);
   AddCredit(L"[[https://sourceforge.net/p/soxr/wiki/Home/|libsoxr]]", roleLibrary);
   AddCredit(
      XO("%s (incorporating %s, %s, %s, %s and %s)")
         .Format(
            "[[http://lv2plug.in/|lv2]]",
            "lilv",
            "msinttypes",
            "serd",
            "sord",
            "sratom"
         ).Translation(),
      roleLibrary);
   AddCredit(L"[[https://www.cs.cmu.edu/~music/nyquist/|Nyquist]]", roleLibrary);
   AddCredit(L"[[https://xiph.org/vorbis/|Ogg Vorbis]]", roleLibrary);
   AddCredit(L"[[http://www.portaudio.com/|PortAudio]]", roleLibrary);
   AddCredit(L"[[http://www.portmedia.sourceforge.net/portmidi/|PortMidi]]", roleLibrary);
   AddCredit(L"[[https://sourceforge.net/p/portmedia/wiki/portsmf/|portsmf]]", roleLibrary);
   AddCredit(L"[[http://sbsms.sourceforge.net/|sbsms]]", roleLibrary);
   AddCredit(L"[[https://www.surina.net/soundtouch/|SoundTouch]]", roleLibrary);
   AddCredit(L"[[http://www.twolame.org/|TwoLAME]]", roleLibrary);
   AddCredit(L"[[http://www.vamp-plugins.org/|Vamp]]", roleLibrary);
   AddCredit(L"[[https://wxwidgets.org/|wxWidgets]]", roleLibrary);

   // Thanks

   AddCredit(L"Dave Beydler", roleThanks);
   AddCredit(L"Brian Cameron", roleThanks);
   AddCredit(L"Jason Cohen", roleThanks);
   AddCredit(L"Dave Fancella", roleThanks);
   AddCredit(L"Steve Harris", roleThanks);
   AddCredit(L"Daniel James", roleThanks);
   AddCredit(L"Daniil Kolpakov", roleThanks);
   AddCredit(L"Robert Leidle", roleThanks);
   AddCredit(L"Logan Lewis", roleThanks);
   AddCredit(L"David Luff", roleThanks);
   AddCredit(L"Jason Pepas", roleThanks);
   AddCredit(L"Jonathan Ryshpan", roleThanks);
   AddCredit(L"Michael Schwendt", roleThanks);
   AddCredit(L"Patrick Shirkey", roleThanks);
   AddCredit(L"Tuomas Suutari", roleThanks);
   AddCredit(L"Mark Tomlinson", roleThanks);
   AddCredit(L"David Topper", roleThanks);
   AddCredit(L"Rudy Trubitt", roleThanks);
   AddCredit(L"StreetIQ.com", roleThanks);
   AddCredit(L"UmixIt Technologies, LLC", roleThanks);
   AddCredit(L"Verilogix, Inc.", roleThanks);
}

// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(AboutDialog, wxDialogWrapper)
   EVT_BUTTON(wxID_OK, AboutDialog::OnOK)
END_EVENT_TABLE()

IMPLEMENT_CLASS(AboutDialog, wxDialogWrapper)

namespace {
   AboutDialog *sActiveInstance{};
}

AboutDialog *AboutDialog::ActiveIntance()
{
   return sActiveInstance;
}

AboutDialog::AboutDialog(wxWindow * parent)
   /* i18n-hint: information about the program */
   :  wxDialogWrapper(parent, -1, XO("About %s").Format( ProgramName ),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
   wxASSERT(!sActiveInstance);
   sActiveInstance = this;

   SetName();
   this->SetBackgroundColour(theTheme.Colour( clrAboutBoxBackground ));
   //this->SetBackgroundColour(theTheme.Colour( clrMedium ));
   icon = NULL;
   ShuttleGui S( this, eIsCreating );
   S.StartNotebook();
   {
      PopulateAudacityPage( S );
      PopulateInformationPage( S );
      PopulateLicensePage( S );
   }
   S.EndNotebook();

   S
      .Id(wxID_OK)
      .Prop(0)
      .AddButton(XXO("OK"), wxALIGN_CENTER, true);

   Fit();
   this->Centre();
}

#define ABOUT_DIALOG_WIDTH 506

void AboutDialog::PopulateAudacityPage( ShuttleGui & S )
{
   CreateCreditsList();

   auto par1Str =
// DA: Says that it is a customised version.
#ifdef EXPERIMENTAL_DA
L"Audacity, which this is a customised version of, is a free program written by a worldwide team of [[https://www.audacityteam.org/about/credits|volunteers]]. \
Audacity is [[https://www.audacityteam.org/download|available]] for Windows, Mac, and GNU/Linux (and other Unix-like systems)."
#else
/* Do the i18n of a string with markup carefully with hints.
 (Remember languages with cases.) */
      XO(
/* i18n-hint: First and third %s will be the program's name,
  second %s will be "volunteers", fourth "available" */
"%s is a free program written by a worldwide team of %s. \
%s is %s for Windows, Mac, and GNU/Linux (and other Unix-like systems).")
         .Format(
            ProgramName,
            Verbatim("[[https://www.audacityteam.org/about/credits|%s]]")
               /* i18n-hint: substitutes into "a worldwide team of %s" */
               .Format( XO("volunteers") ),
            ProgramName,
            Verbatim("[[https://www.audacityteam.org/download|%s]]")
               /* i18n-hint: substitutes into "Audacity is %s" */
               .Format( XO("available") ) )
#endif
   ;

   // This trick here means that the English language version won't mention using
   // English, whereas all translated versions will.
   auto par2Str = XO(
/* i18n-hint first and third %s will be "forum", second "wiki" */
"If you find a bug or have a suggestion for us, please write, in English, to our %s. \
For help, view the tips and tricks on our %s or \
visit our %s.")
      .Format(
         Verbatim("[[https://forum.audacityteam.org/|%s]]")
            /* i18n-hint substitutes into "write to our %s" */
            .Format( XC("forum", "dative") ),
         Verbatim("[[https://wiki.audacityteam.org/|%s]]")
            /* i18n-hint substitutes into "view the tips and tricks on our %s" */
            .Format( XO("wiki") ),
         Verbatim("[[https://forum.audacityteam.org/|%s]]")
            /* i18n-hint substitutes into "visit our %s" */
            .Format( XC("forum", "accusative") ) );
   auto par2StrTranslated = par2Str.Translation();

   if( par2StrTranslated == par2Str.MSGID().GET() )
      par2StrTranslated.Replace( L", in English,", L"" );

   /* i18n-hint: The translation of "translator_credits" will appear
    *  in the credits in the About Audacity window.  Use this to add
    *  your own name(s) to the credits.
    *
    *  For example:  "English translation by Dominic Mazzoni." */
   auto translatorCreditsMsgid = XO("translator_credits");
   auto translatorCredits = translatorCreditsMsgid.Translation();
   if ( translatorCredits == translatorCreditsMsgid.MSGID().GET() )
      // We're in an English locale
      translatorCredits.clear();
   else
      translatorCredits += L"<br>";

   wxStringOutputStream o;
   wxTextOutputStream informationStr( o );   // string to build up list of information in
   informationStr
      << L"<center>"
// DA: Description and provenance in About box
#ifdef EXPERIMENTAL_DA
      #undef _
      #define _(s) wxGetTranslation((s))
      << L"<h3>DarkAudacity "
      << wxString(AUDACITY_VERSION_STRING)
      << L"</center></h3>"
      << L"Customised version of the Audacity free, open source, cross-platform software "
      << L"for recording and editing sounds."
      << L"<p><br>&nbsp; &nbsp; <b>Audacity<sup>&reg;</sup></b> software is copyright &copy; 1999-2021 Audacity Team.<br>"
      << L"&nbsp; &nbsp; The name <b>Audacity</b> is a registered trademark of Dominic Mazzoni.<br><br>"

#else
      << XO("<h3>")
      << ProgramName
      << L" "
      << wxString(AUDACITY_VERSION_STRING)
      << L"</center></h3>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s the free, open source, cross-platform software for recording and editing sounds.")
            .Format(ProgramName)
#endif

      // << L"<p><br>"
      // << par1Str
      // << L"<p>"
      // << par2Str
      << L"<h3>"
      << XO("Credits")
      << L"</h3>"
      << L"<p>"

// DA: Customisation credit
#ifdef EXPERIMENTAL_DA
      << L"<p><b>"
      << XO("DarkAudacity Customisation")
      << L"</b><br>"
      << L"James Crook, art, coding &amp; design<br>"
#endif

      << L"<p><b>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s Team Members").Format( ProgramName )
      << L"</b><br>"
      << GetCreditsByRole(roleTeamMember)

      << L"<p><b> "
      << XO("Emeritus:")
      << L"</b><br>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("Distinguished %s Team members, not currently active")
         .Format( ProgramName )
      << L"<br><br>"
      << GetCreditsByRole(roleEmeritusTeam)

      << L"<p><b>"
      << XO("Contributors")
      << L"</b><br>"
      << GetCreditsByRole(roleContributor)

      << L"<p><b>"
      << XO("Website and Graphics")
      << L"</b><br>"
      << GetCreditsByRole(roleGraphics)
   ;

   if(!translatorCredits.empty()) informationStr
      << L"<p><b>"
      << XO("Translators")
      << L"</b><br>"
      << translatorCredits
   ;

   informationStr
      << L"<p><b>"
      << XO("Libraries")
      << L"</b><br>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s includes code from the following projects:").Format( ProgramName )
      << L"<br><br>"
      << GetCreditsByRole(roleLibrary)

      << L"<p><b>"
      << XO("Special thanks:")
      << L"</b><br>"
      << GetCreditsByRole(roleThanks)

      << L"<p><br>"
      /* i18n-hint: The program's name substitutes for %s */
      << XO("%s website: ").Format( ProgramName )
      << L"[[https://www.audacityteam.org/|https://www.audacityteam.org/]]"

// DA: Link for DA url too
#ifdef EXPERIMENTAL_DA
      << L"<br>DarkAudacity website: [[http://www.darkaudacity.com/|https://www.darkaudacity.com/]]"
#else
      << L"<p><br>&nbsp; &nbsp; "
      /* i18n-hint Audacity's name substitutes for first and third %s,
       and a "copyright" symbol for the second */
      << XO("%s software is copyright %s 1999-2021 %s Team.")
         .Format(
            Verbatim("<b>%s<sup>&reg;</sup></b>").Format( ProgramName ),
            L"&copy;",
            ProgramName )
      << L"<br>"

      << L"&nbsp; &nbsp; "
      /* i18n-hint Audacity's name substitutes for %s */
      << XO("The name %s is a registered trademark.")
         .Format( Verbatim("<b>%s</b>").Format( ProgramName ) )
      << L"<br><br>"
#endif

      << L"</center>"
   ;

   auto pPage =
   S
      .StartNotebookPage( ProgramName );

   S.StartVerticalLay(1);
   {
      //v For now, change to AudacityLogoWithName via old-fashioned way, not Theme.
      wxBitmap logo(AudacityLogoWithName_xpm); //v

      // JKC: Resize to 50% of size.  Later we may use a smaller xpm as
      // our source, but this allows us to tweak the size - if we want to.
      // It also makes it easier to revert to full size if we decide to.
      const float fScale = 0.5f;// smaller size.
      wxImage RescaledImage(logo.ConvertToImage());
      wxColour MainColour( 
         RescaledImage.GetRed(1,1), 
         RescaledImage.GetGreen(1,1), 
         RescaledImage.GetBlue(1,1));
      pPage->SetBackgroundColour(MainColour);
      // wxIMAGE_QUALITY_HIGH not supported by wxWidgets 2.6.1, or we would use it here.
      RescaledImage.Rescale((int)(LOGOWITHNAME_WIDTH * fScale), (int)(LOGOWITHNAME_HEIGHT *fScale));
      wxBitmap RescaledBitmap(RescaledImage);

      icon =
         safenew wxStaticBitmap(S.GetParent(), -1,
         //*logo, //v
         //v theTheme.Bitmap(bmpAudacityLogo), wxPoint(93, 10), wxSize(215, 190));
         //v theTheme.Bitmap(bmpAudacityLogoWithName),
         RescaledBitmap,
         wxDefaultPosition,
         wxSize((int)(LOGOWITHNAME_WIDTH*fScale), (int)(LOGOWITHNAME_HEIGHT*fScale)));
   }

   S
      .Prop(0)
      .AddWindow( icon );

   HtmlWindow *html = safenew LinkingHtmlWindow(S.GetParent(), -1,
                                         wxDefaultPosition,
                                         wxSize(ABOUT_DIALOG_WIDTH, 359),
                                         wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);
   html->SetPage( FormatHtmlText( o.GetString() ) );

   /* locate the html renderer where it fits in the dialogue */
   S
      .Prop(1)
      .Position( wxEXPAND )
      .Focus()
      .AddWindow( html );

   S.EndVerticalLay();
   S.EndNotebookPage();
}

/** \brief: Fills out the "Information" tab of the preferences dialogue
 *
 * Provides as much information as possible about build-time options and
 * the libraries used, to try and make Linux support easier. Basically anything
 * about the build we might wish to know should be visible here */
void AboutDialog::PopulateInformationPage( ShuttleGui & S )
{
   wxStringOutputStream o;
   wxTextOutputStream informationStr( o );   // string to build up list of information in

   S
      .StartNotebookPage( XO("Build Information") );  // start the tab

   S.StartVerticalLay(2);  // create the window
   HtmlWindow *html = safenew LinkingHtmlWindow(S.GetParent(), -1, wxDefaultPosition,
                           wxSize(ABOUT_DIALOG_WIDTH, 264),
                           wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);
   // create a html pane in it to put the content in.
   auto enabled = XO("Enabled");
   auto disabled = XO("Disabled");
   wxString blank;

   /* this builds up the list of information to go in the window in the string
    * informationStr */
   informationStr
      << L"<h2><center>"
      << XO("Build Information")
      << L"</center></h2>\n"
      << VerCheckHtml();
 
   informationStr
      << L"<h3>"
   /* i18n-hint: Information about when audacity was compiled follows */
      << XO("The Build")
      << L"</h3>\n<table>"; // start build info table

   // Current date
   AddBuildinfoRow(&informationStr, XO("Program build date:"), __TDATE__);
   AddBuildinfoRow(&informationStr, XO("Commit Id:"), REV_IDENT );

   auto buildType =
#ifdef _DEBUG
      XO("Debug build (debug level %d)").Format(wxDEBUG_LEVEL);
#else
      XO("Release build (debug level %d)").Format(wxDEBUG_LEVEL);
#endif
   ;
   if( (sizeof(void*) == 8) )
      buildType = XO("%s, 64 bits").Format( buildType );

// Remove this once the transition to CMake is complete
#if defined(CMAKE)
   buildType = Verbatim("CMake %s").Format( buildType );
#endif

   AddBuildinfoRow(&informationStr, XO("Build type:"), buildType.Translation());

#ifdef _MSC_FULL_VER
   AddBuildinfoRow(&informationStr, XO("Compiler:"),
	   wxString::Format(L"MSVC %02d.%02d.%05d.%02d", _MSC_VER / 100, _MSC_VER % 100, _MSC_FULL_VER % 100000, _MSC_BUILD));
#endif

#ifdef __GNUC_PATCHLEVEL__
#ifdef __MINGW32__
   AddBuildinfoRow(&informationStr, XO("Compiler:"), L"MinGW " wxMAKE_VERSION_DOT_STRING_T(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__));
#else
   AddBuildinfoRow(&informationStr, XO("Compiler:"), L"GCC " wxMAKE_VERSION_DOT_STRING_T(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__));
#endif
#endif

#ifdef __clang_version__
   AddBuildinfoRow(&informationStr, XO("Compiler:"), L"clang " __clang_version__);
#endif

   // Install prefix
#ifdef __WXGTK__
   /* i18n-hint: The directory audacity is installed into (on *nix systems) */
   AddBuildinfoRow(&informationStr, XO("Installation Prefix:"), \
         (L"" INSTALL_PREFIX));
#endif

   // Location of settings
   AddBuildinfoRow(&informationStr, XO("Settings folder:"), \
      FileNames::DataDir());

   informationStr << L"</table>\n"; // end of build info table


   informationStr
      << L"<h3>"
      /* i18n-hint: Libraries that are essential to audacity */
      << XO("Core Libraries")
      << L"</h3>\n<table>";  // start table of core libraries

   AddBuildinfoRow(&informationStr, L"wxWidgets",
         XO("Cross-platform GUI library"), Verbatim(wxVERSION_NUM_DOT_STRING_T));

   AddBuildinfoRow(&informationStr, L"PortAudio",
         XO("Audio playback and recording"), Verbatim(L"v19"));

   AddBuildinfoRow(&informationStr, L"libsoxr",
         XO("Sample rate conversion"), enabled);

   informationStr << L"</table>\n"; // end table of core libraries

   informationStr
      << L"<h3>"
      << XO("File Format Support")
      << L"</h3>\n<p>";

   informationStr
      << L"<table>";   // start table of file formats supported


   #ifdef USE_LIBMAD
   /* i18n-hint: This is what the library (libmad) does - imports MP3 files */
   AddBuildinfoRow(&informationStr, L"libmad", XO("MP3 Importing"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"libmad", XO("MP3 Importing"), disabled);
   #endif

   #ifdef USE_LIBVORBIS
   AddBuildinfoRow(&informationStr, L"libvorbis",
   /* i18n-hint: Ogg is the container format. Vorbis is the compression codec.
    * Both are proper nouns and shouldn't be translated */
         XO("Ogg Vorbis Import and Export"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"libvorbis",
         XO("Ogg Vorbis Import and Export"), disabled);
   #endif

   #ifdef USE_LIBID3TAG
   AddBuildinfoRow(&informationStr, L"libid3tag", XO("ID3 tag support"),
         enabled);
   #else
   AddBuildinfoRow(&informationStr, L"libid3tag", XO("ID3 tag support"),
         disabled);
   #endif

   # if USE_LIBFLAC
   /* i18n-hint: FLAC stands for Free Lossless Audio Codec, but is effectively
    * a proper noun and so shouldn't be translated */
   AddBuildinfoRow(&informationStr, L"libflac", XO("FLAC import and export"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"libflac", XO("FLAC import and export"),
         disabled);
   # endif

   # if USE_LIBTWOLAME
   AddBuildinfoRow(&informationStr, L"libtwolame", XO("MP2 export"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"libtwolame", XO("MP2 export"),
         disabled);
   # endif

   # if USE_QUICKTIME
   AddBuildinfoRow(&informationStr, L"QuickTime", XO("Import via QuickTime"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"QuickTime", XO("Import via QuickTime"),
         disabled);
   # endif

   #ifdef USE_FFMPEG
   AddBuildinfoRow(&informationStr, L"ffmpeg", XO("FFmpeg Import/Export"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"ffmpeg", XO("FFmpeg Import/Export"), disabled);
   #endif

   #ifdef USE_GSTREAMER
   AddBuildinfoRow(&informationStr, L"gstreamer", XO("Import via GStreamer"), enabled);
   #else
   AddBuildinfoRow(&informationStr, L"gstreamer", XO("Import via GStreamer"), disabled);
   #endif

   informationStr << L"</table>\n";  //end table of file formats supported

   informationStr
      << L"<h3>"
      << XO("Features")
      << L"</h3>\n<table>";  // start table of features

#ifdef EXPERIMENTAL_DA
   AddBuildinfoRow(&informationStr, L"Theme", XO("Dark Theme Extras"), enabled);
#else
   AddBuildinfoRow(&informationStr, L"Theme", XO("Dark Theme Extras"), disabled);
#endif

   # if USE_NYQUIST
   AddBuildinfoRow(&informationStr, L"Nyquist", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"Nyquist", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_LADSPA
   AddBuildinfoRow(&informationStr, L"LADSPA", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"LADSPA", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_VAMP
   AddBuildinfoRow(&informationStr, L"Vamp", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"Vamp", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_AUDIO_UNITS
   AddBuildinfoRow(&informationStr, L"Audio Units", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"Audio Units", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_VST
   AddBuildinfoRow(&informationStr, L"VST", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"VST", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_LV2
   AddBuildinfoRow(&informationStr, L"LV2", XO("Plug-in support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"LV2", XO("Plug-in support"),
         disabled);
   # endif

   # if USE_PORTMIXER
   AddBuildinfoRow(&informationStr, L"PortMixer", XO("Sound card mixer support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"PortMixer", XO("Sound card mixer support"),
         disabled);
   # endif

   # if USE_SOUNDTOUCH
   AddBuildinfoRow(&informationStr, L"SoundTouch", XO("Pitch and Tempo Change support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"SoundTouch", XO("Pitch and Tempo Change support"),
         disabled);
   # endif

   # if USE_SBSMS
   AddBuildinfoRow(&informationStr, L"SBSMS", XO("Extreme Pitch and Tempo Change support"),
         enabled);
   # else
   AddBuildinfoRow(&informationStr, L"SBSMS", XO("Extreme Pitch and Tempo Change support"),
         disabled);
   # endif

   informationStr << L"</table>\n";   // end of table of features

   html->SetPage( FormatHtmlText( o.GetString() ) );   // push the page into the html renderer

   S
      .Prop(2)
      .Position( wxEXPAND )
      .AddWindow( html ); // make it fill the page

   // I think the 2 here goes with the StartVerticalLay() call above?
   S.EndVerticalLay();     // end window
   S.EndNotebookPage(); // end the tab
}


void AboutDialog::PopulateLicensePage( ShuttleGui & S )
{
   S
      .StartNotebookPage( XO("GPL License") );

   S.StartVerticalLay(1);
   HtmlWindow *html = safenew LinkingHtmlWindow(S.GetParent(), -1,
                                         wxDefaultPosition,
                                         wxSize(ABOUT_DIALOG_WIDTH, 264),
                                         wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER);

// I tried using <pre> here to get a monospaced font,
// as is normally used for the GPL.
// However can't reduce the font size in that case.  It looks
// better proportionally spaced.
//
// The GPL is not to be translated....
   wxString PageText= FormatHtmlText(
L"		    <center>GNU GENERAL PUBLIC LICENSE\n</center>"
L"		       <center>Version 2, June 1991\n</center>"
L"<p><p>"
L" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n"
L" 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA\n"
L" Everyone is permitted to copy and distribute verbatim copies\n"
L" of this license document, but changing it is not allowed.\n"
L"\n"
L"			   <center>Preamble\n</center>"
L"<p><p>\n"
L"  The licenses for most software are designed to take away your\n"
L"freedom to share and change it.  By contrast, the GNU General Public\n"
L"License is intended to guarantee your freedom to share and change free\n"
L"software--to make sure the software is free for all its users.  This\n"
L"General Public License applies to most of the Free Software\n"
L"Foundation's software and to any other program whose authors commit to\n"
L"using it.  (Some other Free Software Foundation software is covered by\n"
L"the GNU Library General Public License instead.)  You can apply it to\n"
L"your programs, too.\n"
L"<p><p>\n"
L"  When we speak of free software, we are referring to freedom, not\n"
L"price.  Our General Public Licenses are designed to make sure that you\n"
L"have the freedom to distribute copies of free software (and charge for\n"
L"this service if you wish), that you receive source code or can get it\n"
L"if you want it, that you can change the software or use pieces of it\n"
L"in new free programs; and that you know you can do these things.\n"
L"<p><p>\n"
L"  To protect your rights, we need to make restrictions that forbid\n"
L"anyone to deny you these rights or to ask you to surrender the rights.\n"
L"These restrictions translate to certain responsibilities for you if you\n"
L"distribute copies of the software, or if you modify it.\n"
L"<p><p>\n"
L"  For example, if you distribute copies of such a program, whether\n"
L"gratis or for a fee, you must give the recipients all the rights that\n"
L"you have.  You must make sure that they, too, receive or can get the\n"
L"source code.  And you must show them these terms so they know their\n"
L"rights.\n"
L"<p><p>\n"
L"  We protect your rights with two steps: (1) copyright the software, and\n"
L"(2) offer you this license which gives you legal permission to copy,\n"
L"distribute and/or modify the software.\n"
L"<p><p>\n"
L"  Also, for each author's protection and ours, we want to make certain\n"
L"that everyone understands that there is no warranty for this free\n"
L"software.  If the software is modified by someone else and passed on, we\n"
L"want its recipients to know that what they have is not the original, so\n"
L"that any problems introduced by others will not reflect on the original\n"
L"authors' reputations.\n"
L"<p><p>\n"
L"  Finally, any free program is threatened constantly by software\n"
L"patents.  We wish to avoid the danger that redistributors of a free\n"
L"program will individually obtain patent licenses, in effect making the\n"
L"program proprietary.  To prevent this, we have made it clear that any\n"
L"patent must be licensed for everyone's free use or not licensed at all.\n"
L"<p><p>\n"
L"  The precise terms and conditions for copying, distribution and\n"
L"modification follow.\n"
L"<p><p>\n"
L"		   <center>GNU GENERAL PUBLIC LICENSE\n</center>"
L"   <center>TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n</center>"
L"<p><p>\n"
L"  0. This License applies to any program or other work which contains\n"
L"a notice placed by the copyright holder saying it may be distributed\n"
L"under the terms of this General Public License.  The \"Program\", below,\n"
L"refers to any such program or work, and a \"work based on the Program\"\n"
L"means either the Program or any derivative work under copyright law:\n"
L"that is to say, a work containing the Program or a portion of it,\n"
L"either verbatim or with modifications and/or translated into another\n"
L"language.  (Hereinafter, translation is included without limitation in\n"
L"the term \"modification\".)  Each licensee is addressed as \"you\".\n"
L"<p><p>\n"
L"Activities other than copying, distribution and modification are not\n"
L"covered by this License; they are outside its scope.  The act of\n"
L"running the Program is not restricted, and the output from the Program\n"
L"is covered only if its contents constitute a work based on the\n"
L"Program (independent of having been made by running the Program).\n"
L"Whether that is true depends on what the Program does.\n"
L"<p><p>\n"
L"  1. You may copy and distribute verbatim copies of the Program's\n"
L"source code as you receive it, in any medium, provided that you\n"
L"conspicuously and appropriately publish on each copy an appropriate\n"
L"copyright notice and disclaimer of warranty; keep intact all the\n"
L"notices that refer to this License and to the absence of any warranty;\n"
L"and give any other recipients of the Program a copy of this License\n"
L"along with the Program.\n"
L"<p><p>\n"
L"You may charge a fee for the physical act of transferring a copy, and\n"
L"you may at your option offer warranty protection in exchange for a fee.\n"
L"<p><p>\n"
L"  2. You may modify your copy or copies of the Program or any portion\n"
L"of it, thus forming a work based on the Program, and copy and\n"
L"distribute such modifications or work under the terms of Section 1\n"
L"above, provided that you also meet all of these conditions:\n"
L"<p><p>\n"
L"<blockquote>"
L"    a) You must cause the modified files to carry prominent notices\n"
L"    stating that you changed the files and the date of any change.\n"
L"<p><p>\n"
L"    b) You must cause any work that you distribute or publish, that in\n"
L"    whole or in part contains or is derived from the Program or any\n"
L"    part thereof, to be licensed as a whole at no charge to all third\n"
L"    parties under the terms of this License.\n"
L"<p><p>\n"
L"    c) If the modified program normally reads commands interactively\n"
L"    when run, you must cause it, when started running for such\n"
L"    interactive use in the most ordinary way, to print or display an\n"
L"    announcement including an appropriate copyright notice and a\n"
L"    notice that there is no warranty (or else, saying that you provide\n"
L"    a warranty) and that users may redistribute the program under\n"
L"    these conditions, and telling the user how to view a copy of this\n"
L"    License.  (Exception: if the Program itself is interactive but\n"
L"    does not normally print such an announcement, your work based on\n"
L"    the Program is not required to print an announcement.)\n"
L"</blockquote>"
L"<p><p>\n"
L"These requirements apply to the modified work as a whole.  If\n"
L"identifiable sections of that work are not derived from the Program,\n"
L"and can be reasonably considered independent and separate works in\n"
L"themselves, then this License, and its terms, do not apply to those\n"
L"sections when you distribute them as separate works.  But when you\n"
L"distribute the same sections as part of a whole which is a work based\n"
L"on the Program, the distribution of the whole must be on the terms of\n"
L"this License, whose permissions for other licensees extend to the\n"
L"entire whole, and thus to each and every part regardless of who wrote it.\n"
L"<p><p>\n"
L"Thus, it is not the intent of this section to claim rights or contest\n"
L"your rights to work written entirely by you; rather, the intent is to\n"
L"exercise the right to control the distribution of derivative or\n"
L"collective works based on the Program.\n"
L"<p><p>\n"
L"In addition, mere aggregation of another work not based on the Program\n"
L"with the Program (or with a work based on the Program) on a volume of\n"
L"a storage or distribution medium does not bring the other work under\n"
L"the scope of this License.\n"
L"<p><p>\n"
L"  3. You may copy and distribute the Program (or a work based on it,\n"
L"under Section 2) in object code or executable form under the terms of\n"
L"Sections 1 and 2 above provided that you also do one of the following:\n"
L"<p><p>\n"
L"<blockquote>"
L"    a) Accompany it with the complete corresponding machine-readable\n"
L"    source code, which must be distributed under the terms of Sections\n"
L"    1 and 2 above on a medium customarily used for software interchange; or,\n"
L"<p><p>\n"
L"    b) Accompany it with a written offer, valid for at least three\n"
L"    years, to give any third party, for a charge no more than your\n"
L"    cost of physically performing source distribution, a complete\n"
L"    machine-readable copy of the corresponding source code, to be\n"
L"    distributed under the terms of Sections 1 and 2 above on a medium\n"
L"    customarily used for software interchange; or,\n"
L"<p><p>\n"
L"    c) Accompany it with the information you received as to the offer\n"
L"    to distribute corresponding source code.  (This alternative is\n"
L"    allowed only for noncommercial distribution and only if you\n"
L"    received the program in object code or executable form with such\n"
L"    an offer, in accord with Subsection b above.)\n"
L"</blockquote>"
L"<p><p>\n"
L"The source code for a work means the preferred form of the work for\n"
L"making modifications to it.  For an executable work, complete source\n"
L"code means all the source code for all modules it contains, plus any\n"
L"associated interface definition files, plus the scripts used to\n"
L"control compilation and installation of the executable.  However, as a\n"
L"special exception, the source code distributed need not include\n"
L"anything that is normally distributed (in either source or binary\n"
L"form) with the major components (compiler, kernel, and so on) of the\n"
L"operating system on which the executable runs, unless that component\n"
L"itself accompanies the executable.\n"
L"<p><p>\n"
L"If distribution of executable or object code is made by offering\n"
L"access to copy from a designated place, then offering equivalent\n"
L"access to copy the source code from the same place counts as\n"
L"distribution of the source code, even though third parties are not\n"
L"compelled to copy the source along with the object code.\n"
L"<p><p>\n"
L"  4. You may not copy, modify, sublicense, or distribute the Program\n"
L"except as expressly provided under this License.  Any attempt\n"
L"otherwise to copy, modify, sublicense or distribute the Program is\n"
L"void, and will automatically terminate your rights under this License.\n"
L"However, parties who have received copies, or rights, from you under\n"
L"this License will not have their licenses terminated so long as such\n"
L"parties remain in full compliance.\n"
L"<p><p>\n"
L"  5. You are not required to accept this License, since you have not\n"
L"signed it.  However, nothing else grants you permission to modify or\n"
L"distribute the Program or its derivative works.  These actions are\n"
L"prohibited by law if you do not accept this License.  Therefore, by\n"
L"modifying or distributing the Program (or any work based on the\n"
L"Program), you indicate your acceptance of this License to do so, and\n"
L"all its terms and conditions for copying, distributing or modifying\n"
L"the Program or works based on it.\n"
L"<p><p>\n"
L"  6. Each time you redistribute the Program (or any work based on the\n"
L"Program), the recipient automatically receives a license from the\n"
L"original licensor to copy, distribute or modify the Program subject to\n"
L"these terms and conditions.  You may not impose any further\n"
L"restrictions on the recipients' exercise of the rights granted herein.\n"
L"You are not responsible for enforcing compliance by third parties to\n"
L"this License.\n"
L"<p><p>\n"
L"  7. If, as a consequence of a court judgment or allegation of patent\n"
L"infringement or for any other reason (not limited to patent issues),\n"
L"conditions are imposed on you (whether by court order, agreement or\n"
L"otherwise) that contradict the conditions of this License, they do not\n"
L"excuse you from the conditions of this License.  If you cannot\n"
L"distribute so as to satisfy simultaneously your obligations under this\n"
L"License and any other pertinent obligations, then as a consequence you\n"
L"may not distribute the Program at all.  For example, if a patent\n"
L"license would not permit royalty-free redistribution of the Program by\n"
L"all those who receive copies directly or indirectly through you, then\n"
L"the only way you could satisfy both it and this License would be to\n"
L"refrain entirely from distribution of the Program.\n"
L"<p><p>\n"
L"If any portion of this section is held invalid or unenforceable under\n"
L"any particular circumstance, the balance of the section is intended to\n"
L"apply and the section as a whole is intended to apply in other\n"
L"circumstances.\n"
L"<p><p>\n"
L"It is not the purpose of this section to induce you to infringe any\n"
L"patents or other property right claims or to contest validity of any\n"
L"such claims; this section has the sole purpose of protecting the\n"
L"integrity of the free software distribution system, which is\n"
L"implemented by public license practices.  Many people have made\n"
L"generous contributions to the wide range of software distributed\n"
L"through that system in reliance on consistent application of that\n"
L"system; it is up to the author/donor to decide if he or she is willing\n"
L"to distribute software through any other system and a licensee cannot\n"
L"impose that choice.\n"
L"<p><p>\n"
L"This section is intended to make thoroughly clear what is believed to\n"
L"be a consequence of the rest of this License.\n"
L"<p><p>\n"
L"  8. If the distribution and/or use of the Program is restricted in\n"
L"certain countries either by patents or by copyrighted interfaces, the\n"
L"original copyright holder who places the Program under this License\n"
L"may add an explicit geographical distribution limitation excluding\n"
L"those countries, so that distribution is permitted only in or among\n"
L"countries not thus excluded.  In such case, this License incorporates\n"
L"the limitation as if written in the body of this License.\n"
L"<p><p>\n"
L"  9. The Free Software Foundation may publish revised and/or new versions\n"
L"of the General Public License from time to time.  Such new versions will\n"
L"be similar in spirit to the present version, but may differ in detail to\n"
L"address new problems or concerns.\n"
L"<p><p>\n"
L"Each version is given a distinguishing version number.  If the Program\n"
L"specifies a version number of this License which applies to it and \"any\n"
L"later version\", you have the option of following the terms and conditions\n"
L"either of that version or of any later version published by the Free\n"
L"Software Foundation.  If the Program does not specify a version number of\n"
L"this License, you may choose any version ever published by the Free Software\n"
L"Foundation.\n"
L"<p><p>\n"
L"  10. If you wish to incorporate parts of the Program into other free\n"
L"programs whose distribution conditions are different, write to the author\n"
L"to ask for permission.  For software which is copyrighted by the Free\n"
L"Software Foundation, write to the Free Software Foundation; we sometimes\n"
L"make exceptions for this.  Our decision will be guided by the two goals\n"
L"of preserving the free status of all derivatives of our free software and\n"
L"of promoting the sharing and reuse of software generally.\n"
L"<p><p>\n"
L"			    <center>NO WARRANTY\n</center>"
L"<p><p>\n"
L"  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n"
L"FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n"
L"OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
L"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n"
L"OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"
L"MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n"
L"TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n"
L"PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n"
L"REPAIR OR CORRECTION.\n"
L"<p><p>\n"
L"  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n"
L"WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n"
L"REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n"
L"INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n"
L"OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n"
L"TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n"
L"YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n"
L"PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n"
L"POSSIBILITY OF SUCH DAMAGES.\n");

   html->SetPage( PageText );

   S
      .Prop(1)
      .Position( wxEXPAND )
      .AddWindow( html );

   S.EndVerticalLay();
   S.EndNotebookPage();
}

void AboutDialog::AddCredit( const wxString &name, Role role )
{
   AddCredit( name, {}, role );
}

void AboutDialog::AddCredit(
   const wxString &name, TranslatableString format, Role role )
{
   auto str = format.empty()
      ? Verbatim( name )
      : TranslatableString{ format }.Format( name );
   creditItems.emplace_back(std::move(str), role);
}

wxString AboutDialog::GetCreditsByRole(AboutDialog::Role role)
{
   wxString s;

   for (const auto &item : creditItems)
   {
      if (item.role == role)
      {
         s += item.description.Translation();
         s += L"<br>";
      }
   }

   // Strip last <br>, if any
   if (s.Right(4) == L"<br>")
      s = s.Left(s.length() - 4);

   return s;
}

/** \brief Add a table row saying if a library is used or not
 *
 * Used when creating the build information tab to show if each optional
 * library is enabled or not, and what it does */
void AboutDialog::AddBuildinfoRow(
   wxTextOutputStream *str, const wxChar * libname,
   const TranslatableString &libdesc, const TranslatableString &status)
{
   *str
      << L"<tr><td>"
      << libname
      << L"</td><td>("
      << libdesc
      << L")</td><td>"
      << status
      << L"</td></tr>";
}

/** \brief Add a table row saying if a library is used or not
 *
 * Used when creating the build information tab to show build dates and
 * file paths */
void AboutDialog::AddBuildinfoRow(
   wxTextOutputStream *str,
   const TranslatableString &description, const wxChar *spec)
{
   *str
      << L"<tr><td>"
      << description
      << L"</td><td>"
      << spec
      << L"</td></tr>";
}

AboutDialog::~AboutDialog()
{
   sActiveInstance = {};
}

void AboutDialog::OnOK(wxCommandEvent & WXUNUSED(event))
{
#ifdef __WXMAC__
   Destroy();
#else
   EndModal(wxID_OK);
#endif
}
