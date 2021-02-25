/**********************************************************************

  Audacity: A Digital Audio Editor

  HelpText.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_HELP_TEXT__
#define __AUDACITY_HELP_TEXT__

class TranslatableString;
class wxString;
#include "Identifier.h"

struct URLStringTag;
//! Distinct type for URLs
using URLString = TaggedIdentifier< URLStringTag >;

HELP_SYSTEM_API  wxString HelpText( const wxString & Key );
HELP_SYSTEM_API TranslatableString TitleText( const wxString & Key );

extern HELP_SYSTEM_API const wxString VerCheckArgs();
extern HELP_SYSTEM_API const URLString VerCheckUrl();
extern HELP_SYSTEM_API const wxString VerCheckHtml();
extern HELP_SYSTEM_API wxString FormatHtmlText( const wxString & Text );

#endif
