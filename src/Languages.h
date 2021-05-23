/**********************************************************************

  Audacity: A Digital Audio Editor

  Languages.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_LANGUAGES__
#define __AUDACITY_LANGUAGES__

class wxArrayString;
class wxString;

#include "Identifier.h"
#include "Internat.h"

namespace Languages {

AUDACITY_DLL_API
/*!
 @param pathList paths to search for .mo files, grouped into subdirectories for the different
    languages
 @param[out] langCodes two-letter language abbreviations (like "fr") or language and country
    (like "pt_BR")
 @param[out] langNames corresponding autonyms of those languages (like "Português")
 */
void GetLanguages( FilePaths pathList,
   wxArrayString &langCodes, TranslatableStrings &langNames);

AUDACITY_DLL_API
/*!
 @param pathList paths to search for .mo files, grouped into subdirectories for the different languages
 */
wxString GetSystemLanguageCode(const FilePaths &pathList);

AUDACITY_DLL_API
/*!
 @param audacityPathList paths to search for .mo files, grouped into subdirectories for the different languages
 @param lang a language code; or if empty or "System", then default to system language.
 @return the language code actually used which is not lang if lang cannot be found. */
wxString SetLang( const FilePaths &audacityPathList, const wxString & lang );

AUDACITY_DLL_API
/*! @return the last language code that was set */
wxString GetLang();

AUDACITY_DLL_API
/*! @return the last language code that was set (minus country code) */
wxString GetLangShort();

AUDACITY_DLL_API
/*! @return a string as from setlocale() */
wxString GetLocaleName();

}

#endif // __AUDACITY_LANGUAGES__
