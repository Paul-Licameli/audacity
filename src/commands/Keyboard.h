/**********************************************************************

  Audacity: A Digital Audio Editor

  Keyboard.h

  Dominic Mazzoni
  Brian Gunlogson

**********************************************************************/

#ifndef __AUDACITY_KEYBOARD__
#define __AUDACITY_KEYBOARD__

#include "Identifier.h"
#include <wx/defs.h>

class wxKeyEvent;

struct DisplayKeyStringTag;

// Holds an operating-system-specific description of a key
// (such as, using the "cloverleaf" character for "command" on Mac
using DisplayKeyString = TaggedIdentifier<DisplayKeyStringTag>;

// This makes DisplayKeyString work as an argument in wxString::Format()
// without calling GET().
// They are reported to the user in dialogs, so we permit this.
template<>
struct wxArgNormalizerNative<const DisplayKeyString&>
: public wxArgNormalizerNative<const wxString&>
{
   wxArgNormalizerNative(const DisplayKeyString& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const wxString&>( s.GET(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative& ) = delete;
   wxArgNormalizerNative& operator=( const wxArgNormalizerNative& ) = delete;
};

template<>
struct wxArgNormalizerNative<DisplayKeyString>
: public wxArgNormalizerNative<const DisplayKeyString&>
{
   wxArgNormalizerNative(const DisplayKeyString& s,
                         const wxFormatString *fmt,
                         unsigned index)
   : wxArgNormalizerNative<const DisplayKeyString&>( s.GET(), fmt, index ) {}
   wxArgNormalizerNative( const wxArgNormalizerNative& ) = delete;
   wxArgNormalizerNative& operator=( const wxArgNormalizerNative& ) = delete;
};

struct NormalizedKeyStringTag;
// Case insensitive comparisons
using NormalizedKeyStringBase = TaggedIdentifier<NormalizedKeyStringTag, false>;

// Holds an operating-system-neutral description of a key
struct AUDACITY_DLL_API NormalizedKeyString : NormalizedKeyStringBase
{
   NormalizedKeyString() = default;
   NormalizedKeyString( const wxString &key );
   NormalizedKeyString( const char *const key )
      : NormalizedKeyString{ wxString{key} } {}
   NormalizedKeyString( const wxChar *const key )
      : NormalizedKeyString{ wxString{key} } {}

   // Convert to and from display form
   DisplayKeyString Display(bool usesSpecialChars = false) const;
   NormalizedKeyString(
      const DisplayKeyString &str, bool usesSpecialChars = false );
};

namespace std
{
   template<> struct hash< NormalizedKeyString >
      : hash< NormalizedKeyStringBase > {};
}

#endif
