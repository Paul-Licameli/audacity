/**********************************************************************

  Audacity: A Digital Audio Editor

  Languages.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_LANGUAGES__
#define __AUDACITY_LANGUAGES__

class wxArrayString;
class wxString;

#include "audacity/Types.h"

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

}

#endif // __AUDACITY_LANGUAGES__
