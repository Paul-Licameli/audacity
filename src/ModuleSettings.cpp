/**********************************************************************

  Audacity: A Digital Audio Editor

  @file ModuleSettings.cpp

  Paul Licameli split from ModulePrefs.cpp

**********************************************************************/

#include "ModuleSettings.h"

#include "Prefs.h"

#include <unordered_set>
#include <wx/filename.h>

static const std::unordered_set<wxString> &autoEnabledModules()
{
   // Add names to this list, of modules that are expected to ship
   // with Audacity and enable automatically.
   static std::unordered_set<wxString> modules{
   };
   return modules;
}

// static function that tells us about a module.
int ModuleSettings::GetModuleStatus(const FilePath &fname)
{
   // Default status is NEW module, and we will ask once.
   int iStatus = kModuleNew;

   wxFileName FileName( fname );
   auto ShortName = FileName.GetName().Lower();

   auto PathPref = wxString( L"/ModulePath/" ) + ShortName;
   auto StatusPref = wxString( L"/Module/" ) + ShortName;
   auto DateTimePref = wxString( L"/ModuleDateTime/" ) + ShortName;

   auto ModulePath = gPrefs->Read( PathPref, wxEmptyString );
   if( ModulePath.IsSameAs( fname ) )
   {
      gPrefs->Read( StatusPref, &iStatus, kModuleNew );

      wxDateTime DateTime = FileName.GetModificationTime();
      wxDateTime OldDateTime;
      OldDateTime.ParseISOCombined( gPrefs->Read( DateTimePref, wxEmptyString ) );

      // Some platforms return milliseconds, some do not...level the playing field
      DateTime.SetMillisecond( 0 );
      OldDateTime.SetMillisecond( 0 );

      // fix up a bad status or reset for newer module
      if( iStatus > kModuleNew || !OldDateTime.IsEqualTo( DateTime ) )
      {
         iStatus=kModuleNew;
      }
   }
   else
   {
      // Remove previously saved since it's no longer valid
      gPrefs->DeleteEntry( PathPref );
      gPrefs->DeleteEntry( StatusPref );
      gPrefs->DeleteEntry( DateTimePref );
   }

   if (iStatus == kModuleNew) {
      if (autoEnabledModules().count(ShortName))
         iStatus = kModuleEnabled;
   }

   return iStatus;
}

void ModuleSettings::SetModuleStatus(const FilePath &fname, int iStatus)
{
   wxFileName FileName( fname );
   wxDateTime DateTime = FileName.GetModificationTime();
   auto ShortName = FileName.GetName().Lower();

   auto PrefName = wxString( L"/Module/" ) + ShortName;
   gPrefs->Write( PrefName, iStatus );

   PrefName = wxString( L"/ModulePath/" ) + ShortName;
   gPrefs->Write( PrefName, fname );

   PrefName = wxString( L"/ModuleDateTime/" ) + ShortName;
   gPrefs->Write( PrefName, DateTime.FormatISOCombined() );

   gPrefs->Flush();
}

