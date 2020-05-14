/**********************************************************************

  Audacity: A Digital Audio Editor

  QualityPrefs.cpp

  Joshua Haberman
  James Crook


*******************************************************************//**

\class QualityPrefs
\brief A PrefsPanel used for setting audio quality.

*//*******************************************************************/


#include "QualityPrefs.h"
#include "QualitySettings.h"

#include <wx/defs.h>

#include "../AudioIOBase.h"
#include "../Dither.h"
#include "../Prefs.h"
#include "../Resample.h"
#include "../ShuttleGui.h"

//////////
QualityPrefs::QualityPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: meaning accuracy in reproduction of sounds */
:  PrefsPanel(parent, winid, XO("Quality"))
{
   Populate();
}

QualityPrefs::~QualityPrefs()
{
}

ComponentInterfaceSymbol QualityPrefs::GetSymbol()
{
   return QUALITY_PREFS_PLUGIN_SYMBOL;
}

TranslatableString QualityPrefs::GetDescription()
{
   return XO("Preferences for Quality");
}

wxString QualityPrefs::HelpPageName()
{
   return "Quality_Preferences";
}

void QualityPrefs::Populate()
{
   // First any pre-processing for constructing the GUI.
   GetNamesAndLabels();
   mOtherSampleRateValue = QualitySettings::DefaultSampleRate.Read();
}

enum { BogusRate = -1 };

/// Gets the lists of names and lists of labels which are
/// used in the choice controls.
/// The names are what the user sees in the choice control.
/// The corresponding labels are what gets stored.
void QualityPrefs::GetNamesAndLabels()
{
   //------------ Sample Rate Names
   // JKC: I don't understand the following comment.
   //      Can someone please explain or correct it?
   // XXX: This should use a previously changed, but not yet saved
   //      sound card setting from the "I/O" preferences tab.
   // LLL: It means that until the user clicks "Ok" in preferences, the
   //      GetSupportedSampleRates() call should use the devices they
   //      may have changed on the Audio I/O page.  As coded, the sample
   //      rates it will return could be completely invalid as they will
   //      be what's supported by the devices that were selected BEFORE
   //      coming into preferences.
   //
   //      GetSupportedSampleRates() allows passing in device names, but
   //      how do you get at them as they are on the Audio I/O page????
   for (int i = 0; i < AudioIOBase::NumStandardRates; i++) {
      int iRate = AudioIOBase::StandardRates[i];
      mSampleRateLabels.push_back(iRate);
      mSampleRateNames.push_back( XO("%i Hz").Format( iRate ) );
   }

   mSampleRateNames.push_back(XO("Other..."));

   // The label for the 'Other...' case can be any value at all.
   mSampleRateLabels.push_back(BogusRate); // If chosen, this value will be overwritten
}

void QualityPrefs::PopulateOrExchange(ShuttleGui & S)
{
   using namespace DialogDefinition;
   S.StartScroller(0, 2);

   S.StartStatic(XO("Sampling"));
   {
      S.StartMultiColumn(2);
      {
         S
            .AddPrompt(XXO("Default Sample &Rate:"));

         S.StartMultiColumn(2);
         {
            // First the choice...
            S
               .Target( NumberChoice( QualitySettings::DefaultSampleRate,
                  mSampleRateNames, mSampleRateLabels ) )
               .AddChoice( {} );

            // Now do the edit box...
            S
               .Enable( [this]{ return UseOtherRate(); } )
               .Target( mOtherSampleRateValue )
               .AddTextBox( {}, {}, 15);
         }
         S.EndHorizontalLay();

         S
            .Target( QualitySettings::SampleFormatSetting )
            .AddChoice( XXO("Default Sample &Format:") );
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S
      .StartStatic(XO("Real-time Conversion"));
   {
      S.StartMultiColumn(2, wxEXPAND);
      {
         S
            .Target( Resample::FastMethodSetting )
            .AddChoice( XXO("Sample Rate Con&verter:") );

         S
            .Target( Dither::FastSetting )
            /* i18n-hint: technical term for randomization to reduce undesirable resampling artifacts */
            .AddChoice( XXO("&Dither:") );
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

   S.StartStatic(XO("High-quality Conversion"));
   {
      S.StartMultiColumn(2);
      {
         S
            .Target( Resample::BestMethodSetting )
            .AddChoice( XXO("Sample Rate Conver&ter:") );

         S
            .Target( Dither::BestSetting )
            /* i18n-hint: technical term for randomization to reduce undesirable resampling artifacts */
            .AddChoice( XXO("Dit&her:") );
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
   S.EndScroller();

}

bool QualityPrefs::UseOtherRate() const
{
   return QualitySettings::DefaultSampleRate.Read() == BogusRate;
}

bool QualityPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();

   // The complex compound control may have value 'other' in which case the
   // value in prefs comes from the second field.
   if ( UseOtherRate() ) {
      QualitySettings::DefaultSampleRate.Write( mOtherSampleRateValue );
      gPrefs->Flush();
   }

   // Tell CopySamples() to use these ditherers now
   InitDitherers();

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "Quality",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew QualityPrefs(parent, winid);
   }
};
}

