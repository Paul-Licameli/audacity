/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMP3.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EXPORTMP3__
#define __AUDACITY_EXPORTMP3__

/* --------------------------------------------------------------------------*/

#include <memory>

enum MP3RateMode : unsigned {
   MODE_SET = 0,
   MODE_VBR,
   MODE_ABR,
   MODE_CBR,
};

template< typename Enum > class EnumLabelSetting;
extern EnumLabelSetting< MP3RateMode > MP3RateModeSetting;

#if defined(__WXMSW__) || defined(__WXMAC__)
#define MP3_EXPORT_BUILT_IN 1
#endif

class TranslatableString;
class wxWindow;

//----------------------------------------------------------------------------
// Get MP3 library version
//----------------------------------------------------------------------------
TranslatableString GetMP3Version(wxWindow *parent, bool prompt);

class IntSetting;
// Set bitrate preference
extern IntSetting MP3SBitrate;
// Variable bitrate preference
extern IntSetting MP3VBitrate;
// Average bitrate preference
extern IntSetting MP3ABitrate;
// Constant bitrate preference
extern IntSetting MP3CBitrate;

#endif

