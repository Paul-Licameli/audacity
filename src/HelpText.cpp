/**********************************************************************

  Audacity: A Digital Audio Editor

  HelpText.cpp

  James Crook

********************************************************************//**

\file HelpText.cpp
\brief Given a key, returns some html.
*//********************************************************************/


#include "HelpText.h"



#include <wx/string.h>
#include <wx/intl.h>
#include <wx/sstream.h>
#include <wx/txtstrm.h>

#include "FileNames.h"
#include "Internat.h"
#include "AllThemeResources.h"
#include "Theme.h"


wxString HtmlColourOfIndex( int i ){
   wxColour c =  theTheme.Colour(i);
   return wxString::Format("\"#%02X%02X%02X\"",
      c.Red(), c.Green(), c.Blue() );
}

static wxString WrapText( const wxString & Text )
{
   return wxString(L"")+
      L"<html><head></head>" +
      L"<body bgcolor=" + HtmlColourOfIndex(clrTrackInfo) + L">" +
      L"<font color=" + HtmlColourOfIndex(clrTrackPanelText) + L">" +
      L"<p>" + Text +
      L"</font>"+
      L"</body></html>";
}

static wxString InnerLink( const wxString &Key, const wxString& Text )
{
   return wxString(L"") +
      L"<a href='innerlink:" +
      Key +
      L"'>" +
      Text +
      L"</a>";
}

static wxString WikiLink( const wxString &Key, const wxString& Text )
{
   return wxString(L"") +
      L"<a href='https://www.audacityteam.org/wiki/index.php?title=" +
      Key +
      L"'>" +
      Text +
      L"</a>";
}

static wxString FileLink( const wxString &Key, const wxString& Text )
{
   return wxString::Format( L"<a href='file:%s%s'>%s</a>",
      FileNames::HtmlHelpDir(), Key, Text );
}

static wxString TypedLink( const wxString &Key, const wxString& Text )
{
   return wxString(L"") +
      L"<a href='" +
      Key +
      L"'>" +
      Text +
      L"</a>";
}

static wxString LinkExpand( const wxString & Text )
{
   wxString Temp = Text;
   int i,j,k;
   while( (i=Temp.First( L"[[" ))!= wxNOT_FOUND )
   {
      wxString Key = Temp.Mid(i+2);
      j = Key.First( L"|" );
      if( j==wxNOT_FOUND )
         return Temp;
      wxString LinkText = Key.Mid( j+1);
      k = LinkText.First( L"]]" );
      if( k==wxNOT_FOUND )
         return Temp;
      Key = Key.Mid( 0, j );
      LinkText = LinkText.Mid( 0, k );

      LinkText=wxString("<font color=") + HtmlColourOfIndex(clrSample) + L">" +LinkText+"</font>";
      wxString Replacement;
      if( Key.StartsWith( L"wiki:" ))
      {
         Replacement = WikiLink( Key.Mid( 5 ), LinkText );
      }
      else if( Key.StartsWith( L"file:" ))
      {
         Replacement = FileLink( Key.Mid( 5 ), LinkText );
      }
      else if( Key.StartsWith( L"http:" ))
      {
         Replacement = TypedLink( Key, LinkText );
      }
      else if( Key.StartsWith( L"https:" ))
      {
         Replacement = TypedLink( Key, LinkText );
      }
      else if( Key.StartsWith( L"mailto:" ))
      {
         Replacement = TypedLink( Key, LinkText );
      }
      else if( Key.StartsWith( L"*URL*" ))
      {
         Replacement = TypedLink( Key, LinkText );
      }
      else
      {
         Replacement = InnerLink( Key, LinkText );
      }


      Temp = Temp.Mid( 0, i ) + Replacement + Temp.Mid( i + j + k + 5 );// 5 for the [[|]]
   }
   return Temp;
}

TranslatableString TitleText( const wxString & Key )
{
   if(Key==L"welcome")
   {
      return XO("Welcome!");
   }

   if(Key ==L"play" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Playing Audio");
   }
   if((Key ==L"record" ) || (Key ==L"norecord" ))
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Recording Audio");
   }
   if(Key ==L"inputdevice" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Recording - Choosing the Recording Device");
   }
   if(Key ==L"inputsource" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Recording - Choosing the Recording Source");
   }
   if(Key ==L"inputlevel" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Recording - Setting the Recording Level");
   }
   if((Key ==L"edit" ) || (Key==L"grey"))
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Editing and greyed out Menus");
   }
   if(Key ==L"export" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Exporting an Audio File");
   }
   if(Key ==L"save" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Saving an Audacity Project");
   }
   if(Key ==L"wma-proprietary" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Support for Other Formats");
   }
   if(Key ==L"burncd" )
   {
      /* i18n-hint: Title for a topic.*/
      return XO("Burn to CD" );
   }
   if(Key ==  L"remotehelp" )
   {
      return XO("No Local Help");
   }
   // Uh oh, no translation...
   return Verbatim( Key );
}

// Allow TranslatableString to work with shift output operators
template< typename Sink >
inline Sink &operator <<( Sink &sink, const TranslatableString &str )
{
   return sink << str.Translation();
}

static wxString HelpTextBuiltIn( const wxString & Key )
{
   // PRL:  Is it necessary to define these outside of conditional compilation so that both get into the .pot file?
   const auto alphamsg = XO(
"<br><br>The version of Audacity you are using is an <b>Alpha test version</b>.");
   const auto betamsg = XO(
"<br><br>The version of Audacity you are using is a <b>Beta test version</b>.");

   if (Key == L"welcome")
   {
      wxStringOutputStream o;
      wxTextOutputStream s(o);
      s
#if defined(IS_ALPHA) || defined(IS_BETA)
         << L"<hr><center><h3>"
         << XO("Get the Official Released Version of Audacity")
         << L"</h3></center>"
         << VerCheckHtml()
#ifdef IS_ALPHA
         << alphamsg
#else
         << betamsg
#endif
         << L" "
         << XO(
"We strongly recommend that you use our latest stable released version, which has full documentation and support.<br><br>")
         << XO(
"You can help us get Audacity ready for release by joining our [[https://www.audacityteam.org/community/|community]].<hr><br><br>")
#endif

// DA: Support methods text.
#ifdef EXPERIMENTAL_DA
         // Deliberately not translated.
         << L"<center><h3>DarkAudacity "
         << AUDACITY_VERSION_STRING
         << L"</h3></center>"
         << L"<br><br>DarkAudacity is based on Audacity:"
         << L"<ul><li>"
         << L" [[http://www.darkaudacity.com|www.darkaudacity.com]] - for differences between them."
         << L"</li><li>"
         <<
L" email to [[mailto:james@audacityteam.org|james@audacityteam.org]] - for help using DarkAudacity."
         << L"</li><li>"
         <<
L" [[http://www.darkaudacity.com/video.html|Tutorials]] - for getting started with DarkAudacity."
         << L"</li></ul>"
         << L"<br><br>Audacity has these support methods:"
         << L"<ul><li>"
         << L" [[https://manual.audacityteam.org/|Manual]] - for comprehensive Audacity documentation"
         << L"</li><li>"
         << L" [[https://forum.audacityteam.org/|Forum]] - for large knowledge base on using Audacity."
         << L"</li></ul>"
#else
         << L"<center><h3>Audacity "
         << AUDACITY_VERSION_STRING
         << L"</h3><h3>"
         << XO("How to get help")
         << L"</h3></center>"
         << XO("These are our support methods:")
         << L"<p><ul><li>"
         /* i18n-hint: Preserve '[[help:Quick_Help|' as it's the name of a link.*/
         << XO("[[help:Quick_Help|Quick Help]] - if not installed locally, [[https://manual.audacityteam.org/quick_help.html|view online]]")
         << L"</li><li>"
         << XO(
/* i18n-hint: Preserve '[[help:Main_Page|' as it's the name of a link.*/
" [[help:Main_Page|Manual]] - if not installed locally, [[https://manual.audacityteam.org/|view online]]")
         << L"</li><li>"
         << XO(
" [[https://forum.audacityteam.org/|Forum]] - ask your question directly, online.")
         << L"</li></ul></p><p>"
         << L"<b>"
         << XO("More:</b> Visit our [[https://wiki.audacityteam.org/index.php|Wiki]] for tips, tricks, extra tutorials and effects plug-ins.")
         << L"</p>"
#endif
   ;

      auto result = o.GetString();
#ifdef USE_ALPHA_MANUAL
      result.Replace( "//manual.audacityteam.org/quick_help.html","//alphamanual.audacityteam.org/man/Quick_Help" );
      result.Replace( "//manual.audacityteam.org/","//alphamanual.audacityteam.org/man/" );
#endif

      return WrapText( result );
   }
   if(Key==L"wma-proprietary")
   {
      wxStringOutputStream o;
      wxTextOutputStream s(o);
      s
         << L"<p>"
         << XO(
"Audacity can import unprotected files in many other formats (such as M4A and WMA, \
compressed WAV files from portable recorders and audio from video files) if you download and install \
the optional [[https://manual.audacityteam.org/man/faq_opening_and_saving_files.html#foreign| \
FFmpeg library]] to your computer.")
         << L"</p><p>"
         <<  XO(
"You can also read our help on importing \
[[https://manual.audacityteam.org/man/playing_and_recording.html#midi|MIDI files]] \
and tracks from [[https://manual.audacityteam.org/man/faq_opening_and_saving_files.html#fromcd| \
audio CDs]].")
         << L"</p>"
      ;
      return WrapText( o.GetString() );
   }

   // Remote help allows us to link to a local copy of the help if it exists,
   // or provide a message that takes you to the Internet if it does not.
   // It's used by the menu item Help > Index
   if(Key ==  L"remotehelp" )
   {
      wxStringOutputStream o;
      wxTextOutputStream s(o);
      s
// *URL* will be replaced by whatever URL we are looking for.
// DA: View the manual on line is expected.
#ifdef EXPERIMENTAL_DA
         << XO(
"The Manual does not appear to be installed. \
Please [[*URL*|view the Manual online]].<br><br>\
To always view the Manual online, change \"Location of Manual\" in \
Interface Preferences to \"From Internet\".")
#else
         << XO(
"The Manual does not appear to be installed. \
Please [[*URL*|view the Manual online]] or \
[[https://manual.audacityteam.org/man/unzipping_the_manual.html| \
download the Manual]].<br><br>\
To always view the Manual online, change \"Location of Manual\" in \
Interface Preferences to \"From Internet\".")
#endif
      ;
      return WrapText( o.GetString() );
   }
   return {};
}

wxString HelpText( const wxString & Key )
{

   // Possible future enhancement...
   // We could look for the text as a local file and use
   // that if we find it...
   // if( wxFileExists( Path+Key ) )
   // ...

   wxString Text;
   Text = HelpTextBuiltIn( Key );

   if( !Text.empty())
      return LinkExpand( Text );

   // Perhaps useful for debugging - we'll return key that we didn't find.
   return WrapText( Key );
}


wxString FormatHtmlText( const wxString & Text ){

   wxString localeStr = wxLocale::GetSystemEncodingName();

   return 
      L"<html><head><META http-equiv=\"Content-Type\" content=\"text/html; charset=" +
      localeStr +
      L"\"></head>" +
      WrapText( LinkExpand( Text ))+
      L"</html>";
}

// Function to give the extra arguments to put on the version check string.
const wxString VerCheckArgs(){
   wxString result = wxString("from_ver=") + AUDACITY_VERSION_STRING;
#ifdef REV_LONG
   result += wxString("&CommitId=")+wxString(REV_LONG).Left(6);
#endif
   result += wxString("&Time=") + wxString( __DATE__ ) + wxString( __TIME__ );
   result.Replace(" ","");
   return result;
}

// Text of hyperlink to check versions.
const wxString VerCheckHtml(){
   wxStringOutputStream o;
   wxTextOutputStream s(o);
   s
      << "<center>[["
      << VerCheckUrl()
      << "|"
      << XO("Check Online")
      << "]]</center>\n";
   return o.GetString();
}

// Url with Version check args attached.
const wxString VerCheckUrl(){
   //The version we intend to use for live Audacity.
#define VER_CHECK_URL "https://www.audacityteam.org/download/?"
//For testing of our scriptlet.
//#define VER_CHECK_URL "http://www.audacityteam.org/slug/?"
//For testing locally
//#define VER_CHECK_URL "http://localhost:63342/WorkingDocs/demos/download.html?"

   return wxString( (L"" VER_CHECK_URL)) +VerCheckArgs();
}
