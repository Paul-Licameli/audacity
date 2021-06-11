/**********************************************************************

   Audacity: A Digital Audio Editor

   ExportFFmpegDialogs.cpp

   Audacity(R) is copyright (c) 1999-2010 Audacity Team.
   License: GPL v2.  See License.txt.

   LRN

******************************************************************//**

\class ExportFFmpegAC3Options
\brief Options dialog for FFmpeg exporting of AC3 format.

*//***************************************************************//**

\class ExportFFmpegAACOptions
\brief Options dialog for FFmpeg exporting of AAC format.

*//***************************************************************//**

\class ExportFFmpegAMRNBOptions
\brief Options dialog for FFmpeg exporting of AMRNB format.

*//***************************************************************//**

\class ExportFFmpegOPUSOptions
\brief Options dialog for FFmpeg exporting of OPUS format.

*//***************************************************************//**

\class ExportFFmpegWMAOptions
\brief Options dialog for FFmpeg exporting of WMA format.

*//***************************************************************//**

\class ExportFFmpegOptions
\brief Options dialog for Custom FFmpeg export format.

*//*******************************************************************/


#include "ExportFFmpegDialogs.h"

#include "../FFmpeg.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/window.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include <wx/stattext.h>

#include "../widgets/FileDialog/FileDialog.h"

#include "../Mix.h"
#include "../Tags.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/HelpSystem.h"

#include "Export.h"

#if defined(USE_FFMPEG)

/// This construction defines a enumeration of UI element IDs, and a static
/// array of their string representations (this way they're always synchronized).
/// Do not store the enumerated values in external files, as they may change;
/// the strings may be stored.
#define FFMPEG_EXPORT_CTRL_ID_ENTRIES \
   FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY(FEFirstID, 20000), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEFormatID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECodecID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEBitrateID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEQualityID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FESampleRateID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELanguageID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FETagID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECutoffID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEFrameSizeID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEBufSizeID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEProfileID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECompLevelID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEUseLPCID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELPCCoeffsID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMinPredID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMaxPredID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEPredOrderID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMinPartOrderID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMaxPartOrderID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEMuxRateID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEPacketSizeID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEBitReservoirID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEVariableBlockLenID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELastID), \
 \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEFormatLabelID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECodecLabelID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEFormatNameID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FECodecNameID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEPresetID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FESavePresetID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FELoadPresetID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEDeletePresetID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEAllFormatsID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEAllCodecsID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEImportPresetsID), \
   FFMPEG_EXPORT_CTRL_ID_ENTRY(FEExportPresetsID) \

// First the enumeration
#define FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY(name, num)  name = num
#define FFMPEG_EXPORT_CTRL_ID_ENTRY(name)             name

enum FFmpegExportCtrlID {
   FFMPEG_EXPORT_CTRL_ID_ENTRIES
};

// Now the string representations
#undef FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY
#define FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY(name, num)  (L"" #name)
#undef FFMPEG_EXPORT_CTRL_ID_ENTRY
#define FFMPEG_EXPORT_CTRL_ID_ENTRY(name)             (L"" #name)
static const wxChar *FFmpegExportCtrlIDNames[] = {
   FFMPEG_EXPORT_CTRL_ID_ENTRIES
};

#undef FFMPEG_EXPORT_CTRL_ID_ENTRIES
#undef FFMPEG_EXPORT_CTRL_ID_ENTRY
#undef FFMPEG_EXPORT_CTRL_ID_FIRST_ENTRY

//----------------------------------------------------------------------------
// ExportFFmpegAC3Options Class
//----------------------------------------------------------------------------

namespace
{

// i18n-hint kbps abbreviates "thousands of bits per second"
inline TranslatableString n_kbps(int n) { return XO("%d kbps").Format( n ); }

const TranslatableStrings AC3BitRateNames{
   n_kbps( 32 ),
   n_kbps( 40 ),
   n_kbps( 48 ),
   n_kbps( 56 ),
   n_kbps( 64 ),
   n_kbps( 80 ),
   n_kbps( 96 ),
   n_kbps( 112 ),
   n_kbps( 128 ),
   n_kbps( 160 ),
   n_kbps( 192 ),
   n_kbps( 224 ),
   n_kbps( 256 ),
   n_kbps( 320 ),
   n_kbps( 384 ),
   n_kbps( 448 ),
   n_kbps( 512 ),
   n_kbps( 576 ),
   n_kbps( 640 ),
};

const std::vector< int > AC3BitRateValues{
   32000,
   40000,
   48000,
   56000,
   64000,
   80000,
   96000,
   112000,
   128000,
   160000,
   192000,
   224000,
   256000,
   320000,
   384000,
   448000,
   512000,
   576000,
   640000,
};

}

const int ExportFFmpegAC3Options::iAC3SampleRates[] = { 32000, 44100, 48000, 0 };

ExportFFmpegAC3Options::ExportFFmpegAC3Options(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegAC3Options::~ExportFFmpegAC3Options()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegAC3Options::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S.TieNumberAsChoice(
               XXO("Bit Rate:"),
               {L"/FileFormats/AC3BitRate",
                160000},
               AC3BitRateNames,
               &AC3BitRateValues
            );
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegAC3Options::TransferDataToWindow()
{
   return true;
}

///
///
bool ExportFFmpegAC3Options::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegAACOptions Class
//----------------------------------------------------------------------------

ExportFFmpegAACOptions::ExportFFmpegAACOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegAACOptions::~ExportFFmpegAACOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegAACOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxEXPAND);
      {
         S.SetSizerProportion(1);
         S.StartMultiColumn(2, wxCENTER);
         {
            S.SetStretchyCol(1);
            S.Prop(1).TieSlider(
               XXO("Quality (kbps):"),
               {L"/FileFormats/AACQuality", 160},320, 98);
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegAACOptions::TransferDataToWindow()
{
   return true;
}

///
///
bool ExportFFmpegAACOptions::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegAMRNBOptions Class
//----------------------------------------------------------------------------

namespace {

// i18n-hint kbps abbreviates "thousands of bits per second"
inline TranslatableString f_kbps( double d ) { return XO("%.2f kbps").Format( d ); }

/// Bit Rates supported by libAMR-NB encoder
/// Sample Rate is always 8 kHz
const TranslatableStrings AMRNBBitRateNames
{
   f_kbps( 4.75 ),
   f_kbps( 5.15 ),
   f_kbps( 5.90 ),
   f_kbps( 6.70 ),
   f_kbps( 7.40 ),
   f_kbps( 7.95 ),
   f_kbps( 10.20 ),
   f_kbps( 12.20 ),
};

const std::vector< int > AMRNBBitRateValues
{
   4750,
   5150,
   5900,
   6700,
   7400,
   7950,
   10200,
   12200,
};

}

ExportFFmpegAMRNBOptions::ExportFFmpegAMRNBOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegAMRNBOptions::~ExportFFmpegAMRNBOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegAMRNBOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S.TieNumberAsChoice(
               XXO("Bit Rate:"),
               {L"/FileFormats/AMRNBBitRate",
                12200},
               AMRNBBitRateNames,
               &AMRNBBitRateValues
            );
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegAMRNBOptions::TransferDataToWindow()
{
   return true;
}

///
///
bool ExportFFmpegAMRNBOptions::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegOPUSOptions Class
//----------------------------------------------------------------------------

namespace {

   /// Bit Rates supported by OPUS encoder. Setting bit rate to other values will not result in different file size.
   ChoiceSetting OPUSBitrate
   {
      L"/FileFormats/OPUSBitrate",
      {
         ByColumns,
         {
            n_kbps( 6 ),
            n_kbps( 8 ),
            n_kbps( 16 ),
            n_kbps( 24 ),
            n_kbps( 32 ),
            n_kbps( 40 ),
            n_kbps( 48 ),
            n_kbps( 64 ),
            n_kbps( 80 ),
            n_kbps( 96 ),
            n_kbps( 128 ),
            n_kbps( 160 ),
            n_kbps( 192 ),
            n_kbps( 256 ),
         },
         {
            L"6000",
            L"8000",
            L"16000",
            L"24000",
            L"32000",
            L"40000",
            L"48000",
            L"64000",
            L"80000",
            L"96000",
            L"128000",
            L"160000",
            L"192000",
            L"256000",
         }
      },
      7 // "128 kbps"
   };

   ChoiceSetting OPUSCompression
   {
      L"/FileFormats/OPUSCompression",
      {
         ByColumns,
         {
            XO("0"),
            XO("1"),
            XO("2"),
            XO("3"),
            XO("4"),
            XO("5"),
            XO("6"),
            XO("7"),
            XO("8"),
            XO("9"),
            XO("10"),
         },
         {
            L"0",
            L"1",
            L"2",
            L"3",
            L"4",
            L"5",
            L"6",
            L"7",
            L"8",
            L"9",
            L"10",
         }
      },
      10 // "10"
   };


   ChoiceSetting OPUSVbrMode
   {
      L"/FileFormats/OPUSVbrMode",
      {
         ByColumns,
         {
            XO("Off"),
            XO("On"),
            XO("Constrained"),
         },
         {
            L"off",
            L"on",
            L"constrained",
         }
      },
      1 // "On"
   };

   ChoiceSetting OPUSApplication
   {
      L"/FileFormats/OPUSApplication",
      {
         ByColumns,
         {
            XO("VOIP"),
            XO("Audio"),
            XO("Low Delay"),
         },
         {
            L"voip",
            L"audio",
            L"lowdelay",
         }
      },
      1 // "Audio"
   };

   ChoiceSetting OPUSFrameDuration
   {
      L"/FileFormats/OPUSFrameDuration",
      {
         ByColumns,
         {
            XO("2.5 ms"),
            XO("5 ms"),
            XO("10 ms"),
            XO("20 ms"),
            XO("40 ms"),
            XO("60 ms"),
         },
         {
            L"2.5",
            L"5",
            L"10",
            L"20",
            L"40",
            L"60",
         }
      },
      3 // "20"
   };

   ChoiceSetting OPUSCutoff
   {
      L"/FileFormats/OPUSCutoff",
      {
         ByColumns,
         {
            XO("Disabled"),
            XO("Narrowband"),
            XO("Mediumband"),
            XO("Wideband"),
            XO("Super Wideband"),
            XO("Fullband"),
         },
         {
            L"0",
            L"4000",
            L"6000",
            L"8000",
            L"12000",
            L"20000",
         }
      },
      0 // "Disabled"
   };
}

ExportFFmpegOPUSOptions::ExportFFmpegOPUSOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegOPUSOptions::~ExportFFmpegOPUSOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegOPUSOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.SetSizerProportion(1);
   S.SetBorder(4);
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S.StartMultiColumn(2, wxCENTER);
            {
               S.TieChoice(
                  XXO("Bit Rate:"),
                  OPUSBitrate);

               S.TieChoice(
                  XXO("Compression"),
                  OPUSCompression);

               S.TieChoice(
                  XXO("Frame Duration:"),
                  OPUSFrameDuration);
            }
            S.EndMultiColumn();

            S.StartMultiColumn(2, wxCENTER);
            {
               S.TieChoice(
                  XXO("Vbr Mode:"),
                  OPUSVbrMode);

               S.TieChoice(
                  XXO("Application:"),
                  OPUSApplication);

               S.TieChoice(
                  XXO("Cutoff:"),
                  OPUSCutoff);

            }
            S.EndMultiColumn();
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegOPUSOptions::TransferDataToWindow()
{
   return true;
}

///
///
bool ExportFFmpegOPUSOptions::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegWMAOptions Class
//----------------------------------------------------------------------------

const int ExportFFmpegWMAOptions::iWMASampleRates[] =
{ 8000, 11025, 16000, 22050, 44100, 0};

namespace {

/// Bit Rates supported by WMA encoder. Setting bit rate to other values will not result in different file size.
const TranslatableStrings WMABitRateNames
{
   n_kbps(24),
   n_kbps(32),
   n_kbps(40),
   n_kbps(48),
   n_kbps(64),
   n_kbps(80),
   n_kbps(96),
   n_kbps(128),
   n_kbps(160),
   n_kbps(192),
   n_kbps(256),
   n_kbps(320),
};

const std::vector< int > WMABitRateValues{
   24000,
   32000,
   40000,
   48000,
   64000,
   80000,
   96000,
   128000,
   160000,
   192000,
   256000,
   320000,
};

}

ExportFFmpegWMAOptions::ExportFFmpegWMAOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegWMAOptions::~ExportFFmpegWMAOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegWMAOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay();
   {
      S.StartHorizontalLay(wxCENTER);
      {
         S.StartMultiColumn(2, wxCENTER);
         {
            S.TieNumberAsChoice(
               XXO("Bit Rate:"),
               {L"/FileFormats/WMABitRate",
                128000},
               WMABitRateNames,
               &WMABitRateValues
            );
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
}

///
///
bool ExportFFmpegWMAOptions::TransferDataToWindow()
{
   return true;
}

///
///
bool ExportFFmpegWMAOptions::TransferDataFromWindow()
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   return true;
}

//----------------------------------------------------------------------------
// ExportFFmpegCustomOptions Class
//----------------------------------------------------------------------------

#define OpenID 9000

BEGIN_EVENT_TABLE(ExportFFmpegCustomOptions, wxPanelWrapper)
   EVT_BUTTON(OpenID, ExportFFmpegCustomOptions::OnOpen)
END_EVENT_TABLE()

ExportFFmpegCustomOptions::ExportFFmpegCustomOptions(wxWindow *parent, int WXUNUSED(format))
:  wxPanelWrapper(parent, wxID_ANY),
   mFormat(NULL),
   mCodec(NULL)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);

   TransferDataToWindow();
}

ExportFFmpegCustomOptions::~ExportFFmpegCustomOptions()
{
   TransferDataFromWindow();
}

///
///
void ExportFFmpegCustomOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartHorizontalLay(wxCENTER);
   {
      S.StartVerticalLay(wxCENTER, 0);
      {
         S.Id(OpenID).AddButton(XXO("Open custom FFmpeg format options"));
         S.StartMultiColumn(2, wxCENTER);
         {
            S.AddPrompt(XXO("Current Format:"));
            mFormat = S.Style(wxTE_READONLY).AddTextBox({}, L"", 25);
            S.AddPrompt(XXO("Current Codec:"));
            mCodec = S.Style(wxTE_READONLY).AddTextBox({}, L"", 25);
         }
         S.EndMultiColumn();
      }
      S.EndHorizontalLay();
   }
   S.EndHorizontalLay();
}

///
///
bool ExportFFmpegCustomOptions::TransferDataToWindow()
{
   if (mFormat)
   {
      mFormat->SetValue(gPrefs->Read(L"/FileFormats/FFmpegFormat", L""));
      mCodec->SetValue(gPrefs->Read(L"/FileFormats/FFmpegCodec", L""));
   }
   return true;
}

///
///
bool ExportFFmpegCustomOptions::TransferDataFromWindow()
{
   return true;
}

///
///
void ExportFFmpegCustomOptions::OnOpen(wxCommandEvent & WXUNUSED(evt))
{
   // Show "Locate FFmpeg" dialog
   PickFFmpegLibs();
   if (!FFmpegLibsInst()->ValidLibsLoaded())
   {
      FFmpegLibsInst()->FindLibs(NULL);
      FFmpegLibsInst()->FreeLibs();
      if (!LoadFFmpeg(true))
      {
         return;
      }
   }
   DropFFmpegLibs();

#ifdef __WXMAC__
   // Bug 2077 Must be a parent window on OSX or we will appear behind.
   auto pWin = wxGetTopLevelParent( this );
#else
   // Use GetTopWindow on windows as there is no hWnd with top level parent.
   auto pWin = wxTheApp->GetTopWindow();
#endif

   ExportFFmpegOptions od(pWin);
   od.ShowModal();

   TransferDataToWindow();
}

FFmpegPreset::FFmpegPreset()
{
   mControlState.resize(FELastID - FEFirstID);
}

FFmpegPreset::~FFmpegPreset()
{
}

FFmpegPresets::FFmpegPresets()
{
   mPreset = NULL;
   mAbortImport = false;

   XMLFileReader xmlfile;
   wxFileNameWrapper xmlFileName{ FileNames::DataDir(), FilePath{ L"ffmpeg_presets.xml" } };
   xmlfile.Parse(this,xmlFileName.GetFullPath());
}

FFmpegPresets::~FFmpegPresets()
{
   // We're in a destructor!  Don't let exceptions out!
   GuardedCall( [&] {
      wxFileNameWrapper xmlFileName{
         FileNames::DataDir(), FilePath{ L"ffmpeg_presets.xml" } };
      XMLFileWriter writer{
         xmlFileName.GetFullPath(), XO("Error Saving FFmpeg Presets") };
      WriteXMLHeader(writer);
      WriteXML(writer);
      writer.Commit();
   } );
}

void FFmpegPresets::ImportPresets(wxString &filename)
{
   mPreset = NULL;
   mAbortImport = false;

   FFmpegPresetMap savePresets = mPresets;

   XMLFileReader xmlfile;
   bool success = xmlfile.Parse(this,filename);
   if (!success || mAbortImport) {
      mPresets = savePresets;
   }
}

void FFmpegPresets::ExportPresets(wxString &filename)
{
   GuardedCall( [&] {
      XMLFileWriter writer{ filename, XO("Error Saving FFmpeg Presets") };
      WriteXMLHeader(writer);
      WriteXML(writer);
      writer.Commit();
   } );
}

void FFmpegPresets::GetPresetList(wxArrayString &list)
{
   list.clear();
   FFmpegPresetMap::iterator iter;
   for (iter = mPresets.begin(); iter != mPresets.end(); ++iter)
   {
      list.push_back(iter->second.mPresetName);
   }

   std::sort( list.begin(), list.end() );
}

void FFmpegPresets::DeletePreset(wxString &name)
{
   FFmpegPresetMap::iterator iter = mPresets.find(name);
   if (iter != mPresets.end())
   {
      mPresets.erase(iter);
   }
}

FFmpegPreset *FFmpegPresets::FindPreset(wxString &name)
{
   FFmpegPresetMap::iterator iter = mPresets.find(name);
   if (iter != mPresets.end())
   {
      return &iter->second;
   }

   return NULL;
}

// return false if overwrite was not allowed.
bool FFmpegPresets::OverwriteIsOk( wxString &name )
{
   FFmpegPreset *preset = FindPreset(name);
   if (preset)
   {
      auto query = XO("Overwrite preset '%s'?").Format(name);
      int action = AudacityMessageBox(
         query,
         XO("Confirm Overwrite"),
         wxYES_NO | wxCENTRE);
      if (action == wxNO) return false;
   }
   return true;
}


bool FFmpegPresets::SavePreset(ExportFFmpegOptions *parent, wxString &name)
{
   wxString format;
   wxString codec;
   FFmpegPreset *preset;

   {
      wxWindow *wnd;
      wxListBox *lb;

      wnd = dynamic_cast<wxWindow*>(parent)->FindWindowById(FEFormatID,parent);
      lb = dynamic_cast<wxListBox*>(wnd);
      if (lb->GetSelection() < 0)
      {
         AudacityMessageBox( XO("Please select format before saving a profile") );
         return false;
      }
      format = lb->GetStringSelection();

      wnd = dynamic_cast<wxWindow*>(parent)->FindWindowById(FECodecID,parent);
      lb = dynamic_cast<wxListBox*>(wnd);
      if (lb->GetSelection() < 0)
      {
         /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
         AudacityMessageBox( XO("Please select codec before saving a profile") );
         return false;
      }
      codec = lb->GetStringSelection();
   }

   preset = &mPresets[name];
   preset->mPresetName = name;

   wxSpinCtrl *sc;
   wxTextCtrl *tc;
   wxCheckBox *cb;
   wxChoice *ch;

   for (int id = FEFirstID; id < FELastID; id++)
   {
      wxWindow *wnd = dynamic_cast<wxWindow*>(parent)->FindWindowById(id,parent);
      if (wnd != NULL)
      {
         switch(id)
         {
         case FEFormatID:
            preset->mControlState[id - FEFirstID] = format;
            break;
         case FECodecID:
            preset->mControlState[id - FEFirstID] = codec;
            break;
         // Spin control
         case FEBitrateID:
         case FEQualityID:
         case FESampleRateID:
         case FECutoffID:
         case FEFrameSizeID:
         case FEBufSizeID:
         case FECompLevelID:
         case FELPCCoeffsID:
         case FEMinPredID:
         case FEMaxPredID:
         case FEMinPartOrderID:
         case FEMaxPartOrderID:
         case FEMuxRateID:
         case FEPacketSizeID:
            sc = dynamic_cast<wxSpinCtrl*>(wnd);
            preset->mControlState[id - FEFirstID] = wxString::Format(L"%d",sc->GetValue());
            break;
         // Text control
         case FELanguageID:
         case FETagID:
            tc = dynamic_cast<wxTextCtrl*>(wnd);
            preset->mControlState[id - FEFirstID] = tc->GetValue();
            break;
         // Choice
         case FEProfileID:
         case FEPredOrderID:
            ch = dynamic_cast<wxChoice*>(wnd);
            preset->mControlState[id - FEFirstID] = wxString::Format(L"%d",ch->GetSelection());
            break;
         // Check box
         case FEUseLPCID:
         case FEBitReservoirID:
         case FEVariableBlockLenID:
            cb = dynamic_cast<wxCheckBox*>(wnd);
            preset->mControlState[id - FEFirstID] = wxString::Format(L"%d",cb->GetValue());
            break;
         }
      }
   }
   return true;
}

void FFmpegPresets::LoadPreset(ExportFFmpegOptions *parent, wxString &name)
{
   FFmpegPreset *preset = FindPreset(name);
   if (!preset)
   {
      AudacityMessageBox( XO("Preset '%s' does not exist." ).Format(name));
      return;
   }

   wxListBox *lb;
   wxSpinCtrl *sc;
   wxTextCtrl *tc;
   wxCheckBox *cb;
   wxChoice *ch;

   for (int id = FEFirstID; id < FELastID; id++)
   {
      wxWindow *wnd = parent->FindWindowById(id,parent);
      if (wnd != NULL)
      {
         wxString readstr;
         long readlong;
         bool readbool;
         switch(id)
         {
         // Listbox
         case FEFormatID:
         case FECodecID:
            lb = dynamic_cast<wxListBox*>(wnd);
            readstr = preset->mControlState[id - FEFirstID];
            readlong = lb->FindString(readstr);
            if (readlong > -1) lb->Select(readlong);
            break;
         // Spin control
         case FEBitrateID:
         case FEQualityID:
         case FESampleRateID:
         case FECutoffID:
         case FEFrameSizeID:
         case FEBufSizeID:
         case FECompLevelID:
         case FELPCCoeffsID:
         case FEMinPredID:
         case FEMaxPredID:
         case FEMinPartOrderID:
         case FEMaxPartOrderID:
         case FEMuxRateID:
         case FEPacketSizeID:
            sc = dynamic_cast<wxSpinCtrl*>(wnd);
            preset->mControlState[id - FEFirstID].ToLong(&readlong);
            sc->SetValue(readlong);
            break;
         // Text control
         case FELanguageID:
         case FETagID:
            tc = dynamic_cast<wxTextCtrl*>(wnd);
            tc->SetValue(preset->mControlState[id - FEFirstID]);
            break;
         // Choice
         case FEProfileID:
         case FEPredOrderID:
            ch = dynamic_cast<wxChoice*>(wnd);
            preset->mControlState[id - FEFirstID].ToLong(&readlong);
            if (readlong > -1) ch->Select(readlong);
            break;
         // Check box
         case FEUseLPCID:
         case FEBitReservoirID:
         case FEVariableBlockLenID:
            cb = dynamic_cast<wxCheckBox*>(wnd);
            preset->mControlState[id - FEFirstID].ToLong(&readlong);
            if (readlong) readbool = true; else readbool = false;
            cb->SetValue(readbool);
            break;
         }
      }
   }
}

bool FFmpegPresets::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (mAbortImport)
   {
      return false;
   }

   if (!wxStrcmp(tag,L"ffmpeg_presets"))
   {
      return true;
   }

   if (!wxStrcmp(tag,L"preset"))
   {
      while (*attrs)
      {
         const auto attr = *attrs++;
         wxString value = *attrs++;

         if (!value)
            break;

         if (!wxStrcmp(attr,L"name"))
         {
            mPreset = FindPreset(value);
            if (mPreset)
            {
               auto query = XO("Replace preset '%s'?").Format( value );
               int action = AudacityMessageBox(
                  query,
                  XO("Confirm Overwrite"),
                  wxYES_NO | wxCANCEL | wxCENTRE);
               if (action == wxCANCEL)
               {
                  mAbortImport = true;
                  return false;
               }
               if (action == wxNO)
               {
                  mPreset = NULL;
                  return false;
               }
               *mPreset = FFmpegPreset();
            }
            else
            {
               mPreset = &mPresets[value];
            }
            mPreset->mPresetName = value;
         }
      }
      return true;
   }

   if (!wxStrcmp(tag,L"setctrlstate") && mPreset)
   {
      long id = -1;
      while (*attrs)
      {
         const auto attr = *attrs++;
         const auto value = *attrs++;

         if (!value)
            break;

         if (!wxStrcmp(attr,L"id"))
         {
            for (long i = FEFirstID; i < FELastID; i++)
               if (!wxStrcmp(FFmpegExportCtrlIDNames[i - FEFirstID],value))
                  id = i;
         }
         else if (!wxStrcmp(attr,L"state"))
         {
            if (id > FEFirstID && id < FELastID)
               mPreset->mControlState[id - FEFirstID] = wxString(value);
         }
      }
      return true;
   }

   return false;
}

XMLTagHandler *FFmpegPresets::HandleXMLChild(const wxChar *tag)
{
   if (mAbortImport)
   {
      return NULL;
   }

   if (!wxStrcmp(tag, L"preset"))
   {
      return this;
   }
   else if (!wxStrcmp(tag, L"setctrlstate"))
   {
      return this;
   }
   return NULL;
}

void FFmpegPresets::WriteXMLHeader(XMLWriter &xmlFile) const
// may throw
{
   xmlFile.Write(L"<?xml ");
   xmlFile.Write(L"version=\"1.0\" ");
   xmlFile.Write(L"standalone=\"no\" ");
   xmlFile.Write(L"?>\n");

   wxString dtdName = L"-//audacityffmpegpreset-1.0.0//DTD//EN";
   wxString dtdURI =
      L"http://audacity.sourceforge.net/xml/audacityffmpegpreset-1.0.0.dtd";

   xmlFile.Write(L"<!DOCTYPE ");
   xmlFile.Write(L"project ");
   xmlFile.Write(L"PUBLIC ");
   xmlFile.Write(L"\"-//audacityffmpegpreset-1.0.0//DTD//EN\" ");
   xmlFile.Write(L"\"http://audacity.sourceforge.net/xml/audacityffmpegpreset-1.0.0.dtd\" ");
   xmlFile.Write(L">\n");
}

void FFmpegPresets::WriteXML(XMLWriter &xmlFile) const
// may throw
{
   xmlFile.StartTag(L"ffmpeg_presets");
   xmlFile.WriteAttr(L"version",L"1.0");
   FFmpegPresetMap::const_iterator iter;
   for (iter = mPresets.begin(); iter != mPresets.end(); ++iter)
   {
      auto preset = &iter->second;
      xmlFile.StartTag(L"preset");
      xmlFile.WriteAttr(L"name",preset->mPresetName);
      for (long i = FEFirstID + 1; i < FELastID; i++)
      {
         xmlFile.StartTag(L"setctrlstate");
         xmlFile.WriteAttr(L"id",wxString(FFmpegExportCtrlIDNames[i - FEFirstID]));
         xmlFile.WriteAttr(L"state",preset->mControlState[i - FEFirstID]);
         xmlFile.EndTag(L"setctrlstate");
      }
      xmlFile.EndTag(L"preset");
   }
   xmlFile.EndTag(L"ffmpeg_presets");
}

//----------------------------------------------------------------------------
// ExportFFmpegOptions Class
//----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ExportFFmpegOptions, wxDialogWrapper)
   EVT_BUTTON(wxID_OK,ExportFFmpegOptions::OnOK)
   EVT_BUTTON(wxID_HELP,ExportFFmpegOptions::OnGetURL)
   EVT_LISTBOX(FEFormatID,ExportFFmpegOptions::OnFormatList)
   EVT_LISTBOX(FECodecID,ExportFFmpegOptions::OnCodecList)
   EVT_BUTTON(FEAllFormatsID,ExportFFmpegOptions::OnAllFormats)
   EVT_BUTTON(FEAllCodecsID,ExportFFmpegOptions::OnAllCodecs)
   EVT_BUTTON(FESavePresetID,ExportFFmpegOptions::OnSavePreset)
   EVT_BUTTON(FELoadPresetID,ExportFFmpegOptions::OnLoadPreset)
   EVT_BUTTON(FEDeletePresetID,ExportFFmpegOptions::OnDeletePreset)
   EVT_BUTTON(FEImportPresetsID,ExportFFmpegOptions::OnImportPresets)
   EVT_BUTTON(FEExportPresetsID,ExportFFmpegOptions::OnExportPresets)
END_EVENT_TABLE()

/// Format-codec compatibility list
/// Must end with NULL entry
CompatibilityEntry ExportFFmpegOptions::CompatibilityList[] =
{
   { L"adts", AV_CODEC_ID_AAC },

   { L"aiff", AV_CODEC_ID_PCM_S16BE },
   { L"aiff", AV_CODEC_ID_PCM_S8 },
   { L"aiff", AV_CODEC_ID_PCM_S24BE },
   { L"aiff", AV_CODEC_ID_PCM_S32BE },
   { L"aiff", AV_CODEC_ID_PCM_ALAW },
   { L"aiff", AV_CODEC_ID_PCM_MULAW },
   { L"aiff", AV_CODEC_ID_MACE3 },
   { L"aiff", AV_CODEC_ID_MACE6 },
   { L"aiff", AV_CODEC_ID_GSM },
   { L"aiff", AV_CODEC_ID_ADPCM_G726 },
   { L"aiff", AV_CODEC_ID_PCM_S16LE },
   { L"aiff", AV_CODEC_ID_ADPCM_IMA_QT },
   { L"aiff", AV_CODEC_ID_QDM2 },

   { L"amr", AV_CODEC_ID_AMR_NB },
   { L"amr", AV_CODEC_ID_AMR_WB },

   { L"asf", AV_CODEC_ID_PCM_S16LE },
   { L"asf", AV_CODEC_ID_PCM_U8 },
   { L"asf", AV_CODEC_ID_PCM_S24LE },
   { L"asf", AV_CODEC_ID_PCM_S32LE },
   { L"asf", AV_CODEC_ID_ADPCM_MS },
   { L"asf", AV_CODEC_ID_PCM_ALAW },
   { L"asf", AV_CODEC_ID_PCM_MULAW },
   { L"asf", AV_CODEC_ID_WMAVOICE },
   { L"asf", AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"asf", AV_CODEC_ID_ADPCM_YAMAHA },
   { L"asf", AV_CODEC_ID_TRUESPEECH },
   { L"asf", AV_CODEC_ID_GSM_MS },
   { L"asf", AV_CODEC_ID_ADPCM_G726 },
   //{ L"asf", AV_CODEC_ID_MP2 }, Bug 59
   { L"asf", AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"asf", AV_CODEC_ID_VOXWARE },
#endif
   { L"asf", AV_CODEC_ID_AAC },
   { L"asf", AV_CODEC_ID_WMAV1 },
   { L"asf", AV_CODEC_ID_WMAV2 },
   { L"asf", AV_CODEC_ID_WMAPRO },
   { L"asf", AV_CODEC_ID_ADPCM_CT },
   { L"asf", AV_CODEC_ID_ATRAC3 },
   { L"asf", AV_CODEC_ID_IMC },
   { L"asf", AV_CODEC_ID_AC3 },
   { L"asf", AV_CODEC_ID_DTS },
   { L"asf", AV_CODEC_ID_FLAC },
   { L"asf", AV_CODEC_ID_ADPCM_SWF },
   { L"asf", AV_CODEC_ID_VORBIS },

   { L"au", AV_CODEC_ID_PCM_MULAW },
   { L"au", AV_CODEC_ID_PCM_S8 },
   { L"au", AV_CODEC_ID_PCM_S16BE },
   { L"au", AV_CODEC_ID_PCM_ALAW },

   { L"avi", AV_CODEC_ID_PCM_S16LE },
   { L"avi", AV_CODEC_ID_PCM_U8 },
   { L"avi", AV_CODEC_ID_PCM_S24LE },
   { L"avi", AV_CODEC_ID_PCM_S32LE },
   { L"avi", AV_CODEC_ID_ADPCM_MS },
   { L"avi", AV_CODEC_ID_PCM_ALAW },
   { L"avi", AV_CODEC_ID_PCM_MULAW },
   { L"avi", AV_CODEC_ID_WMAVOICE },
   { L"avi", AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"avi", AV_CODEC_ID_ADPCM_YAMAHA },
   { L"avi", AV_CODEC_ID_TRUESPEECH },
   { L"avi", AV_CODEC_ID_GSM_MS },
   { L"avi", AV_CODEC_ID_ADPCM_G726 },
   // { L"avi", AV_CODEC_ID_MP2 }, //Bug 59
   { L"avi", AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"avi", AV_CODEC_ID_VOXWARE },
#endif
   { L"avi", AV_CODEC_ID_AAC },
   { L"avi", AV_CODEC_ID_WMAV1 },
   { L"avi", AV_CODEC_ID_WMAV2 },
   { L"avi", AV_CODEC_ID_WMAPRO },
   { L"avi", AV_CODEC_ID_ADPCM_CT },
   { L"avi", AV_CODEC_ID_ATRAC3 },
   { L"avi", AV_CODEC_ID_IMC },
   { L"avi", AV_CODEC_ID_AC3 },
   { L"avi", AV_CODEC_ID_DTS },
   { L"avi", AV_CODEC_ID_FLAC },
   { L"avi", AV_CODEC_ID_ADPCM_SWF },
   { L"avi", AV_CODEC_ID_VORBIS },

   { L"crc", AV_CODEC_ID_NONE },

   { L"dv", AV_CODEC_ID_PCM_S16LE },

   { L"ffm", AV_CODEC_ID_NONE },

   { L"flv", AV_CODEC_ID_MP3 },
   { L"flv", AV_CODEC_ID_PCM_S8 },
   { L"flv", AV_CODEC_ID_PCM_S16BE },
   { L"flv", AV_CODEC_ID_PCM_S16LE },
   { L"flv", AV_CODEC_ID_ADPCM_SWF },
   { L"flv", AV_CODEC_ID_AAC },
   { L"flv", AV_CODEC_ID_NELLYMOSER },

   { L"framecrc", AV_CODEC_ID_NONE },

   { L"gxf", AV_CODEC_ID_PCM_S16LE },

   { L"matroska", AV_CODEC_ID_PCM_S16LE },
   { L"matroska", AV_CODEC_ID_PCM_U8 },
   { L"matroska", AV_CODEC_ID_PCM_S24LE },
   { L"matroska", AV_CODEC_ID_PCM_S32LE },
   { L"matroska", AV_CODEC_ID_ADPCM_MS },
   { L"matroska", AV_CODEC_ID_PCM_ALAW },
   { L"matroska", AV_CODEC_ID_PCM_MULAW },
   { L"matroska", AV_CODEC_ID_WMAVOICE },
   { L"matroska", AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"matroska", AV_CODEC_ID_ADPCM_YAMAHA },
   { L"matroska", AV_CODEC_ID_TRUESPEECH },
   { L"matroska", AV_CODEC_ID_GSM_MS },
   { L"matroska", AV_CODEC_ID_ADPCM_G726 },
   // { L"matroska", AV_CODEC_ID_MP2 }, // Bug 59
   { L"matroska", AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"matroska", AV_CODEC_ID_VOXWARE },
#endif
   { L"matroska", AV_CODEC_ID_AAC },
   { L"matroska", AV_CODEC_ID_WMAV1 },
   { L"matroska", AV_CODEC_ID_WMAV2 },
   { L"matroska", AV_CODEC_ID_WMAPRO },
   { L"matroska", AV_CODEC_ID_ADPCM_CT },
   { L"matroska", AV_CODEC_ID_ATRAC3 },
   { L"matroska", AV_CODEC_ID_IMC },
   { L"matroska", AV_CODEC_ID_AC3 },
   { L"matroska", AV_CODEC_ID_DTS },
   { L"matroska", AV_CODEC_ID_FLAC },
   { L"matroska", AV_CODEC_ID_ADPCM_SWF },
   { L"matroska", AV_CODEC_ID_VORBIS },

   { L"mmf", AV_CODEC_ID_ADPCM_YAMAHA },

   { L"mov", AV_CODEC_ID_PCM_S32BE }, //mov
   { L"mov", AV_CODEC_ID_PCM_S32LE },
   { L"mov", AV_CODEC_ID_PCM_S24BE },
   { L"mov", AV_CODEC_ID_PCM_S24LE },
   { L"mov", AV_CODEC_ID_PCM_S16BE },
   { L"mov", AV_CODEC_ID_PCM_S16LE },
   { L"mov", AV_CODEC_ID_PCM_S8 },
   { L"mov", AV_CODEC_ID_PCM_U8 },
   { L"mov", AV_CODEC_ID_PCM_MULAW },
   { L"mov", AV_CODEC_ID_PCM_ALAW },
   { L"mov", AV_CODEC_ID_ADPCM_IMA_QT },
   { L"mov", AV_CODEC_ID_MACE3 },
   { L"mov", AV_CODEC_ID_MACE6 },
   { L"mov", AV_CODEC_ID_MP3 },
   { L"mov", AV_CODEC_ID_AAC },
   { L"mov", AV_CODEC_ID_AMR_NB },
   { L"mov", AV_CODEC_ID_AMR_WB },
   { L"mov", AV_CODEC_ID_GSM },
   { L"mov", AV_CODEC_ID_ALAC },
   { L"mov", AV_CODEC_ID_QCELP },
   { L"mov", AV_CODEC_ID_QDM2 },
   { L"mov", AV_CODEC_ID_DVAUDIO },
   { L"mov", AV_CODEC_ID_WMAV2 },
   { L"mov", AV_CODEC_ID_ALAC },

   { L"mp4", AV_CODEC_ID_AAC },
   { L"mp4", AV_CODEC_ID_QCELP },
   { L"mp4", AV_CODEC_ID_MP3 },
   { L"mp4", AV_CODEC_ID_VORBIS },

   { L"psp", AV_CODEC_ID_AAC },
   { L"psp", AV_CODEC_ID_QCELP },
   { L"psp", AV_CODEC_ID_MP3 },
   { L"psp", AV_CODEC_ID_VORBIS },

   { L"ipod", AV_CODEC_ID_AAC },
   { L"ipod", AV_CODEC_ID_QCELP },
   { L"ipod", AV_CODEC_ID_MP3 },
   { L"ipod", AV_CODEC_ID_VORBIS },

   { L"3gp", AV_CODEC_ID_AAC },
   { L"3gp", AV_CODEC_ID_AMR_NB },
   { L"3gp", AV_CODEC_ID_AMR_WB },

   { L"3g2", AV_CODEC_ID_AAC },
   { L"3g2", AV_CODEC_ID_AMR_NB },
   { L"3g2", AV_CODEC_ID_AMR_WB },

   { L"mp3", AV_CODEC_ID_MP3 },

   { L"mpeg", AV_CODEC_ID_AC3 },
   { L"mpeg", AV_CODEC_ID_DTS },
   { L"mpeg", AV_CODEC_ID_PCM_S16BE },
   //{ L"mpeg", AV_CODEC_ID_MP2 },// Bug 59

   { L"vcd", AV_CODEC_ID_AC3 },
   { L"vcd", AV_CODEC_ID_DTS },
   { L"vcd", AV_CODEC_ID_PCM_S16BE },
   //{ L"vcd", AV_CODEC_ID_MP2 },// Bug 59

   { L"vob", AV_CODEC_ID_AC3 },
   { L"vob", AV_CODEC_ID_DTS },
   { L"vob", AV_CODEC_ID_PCM_S16BE },
   //{ L"vob", AV_CODEC_ID_MP2 },// Bug 59

   { L"svcd", AV_CODEC_ID_AC3 },
   { L"svcd", AV_CODEC_ID_DTS },
   { L"svcd", AV_CODEC_ID_PCM_S16BE },
   //{ L"svcd", AV_CODEC_ID_MP2 },// Bug 59

   { L"dvd", AV_CODEC_ID_AC3 },
   { L"dvd", AV_CODEC_ID_DTS },
   { L"dvd", AV_CODEC_ID_PCM_S16BE },
   //{ L"dvd", AV_CODEC_ID_MP2 },// Bug 59

   { L"nut", AV_CODEC_ID_PCM_S16LE },
   { L"nut", AV_CODEC_ID_PCM_U8 },
   { L"nut", AV_CODEC_ID_PCM_S24LE },
   { L"nut", AV_CODEC_ID_PCM_S32LE },
   { L"nut", AV_CODEC_ID_ADPCM_MS },
   { L"nut", AV_CODEC_ID_PCM_ALAW },
   { L"nut", AV_CODEC_ID_PCM_MULAW },
   { L"nut", AV_CODEC_ID_WMAVOICE },
   { L"nut", AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"nut", AV_CODEC_ID_ADPCM_YAMAHA },
   { L"nut", AV_CODEC_ID_TRUESPEECH },
   { L"nut", AV_CODEC_ID_GSM_MS },
   { L"nut", AV_CODEC_ID_ADPCM_G726 },
   //{ L"nut", AV_CODEC_ID_MP2 },// Bug 59
   { L"nut", AV_CODEC_ID_MP3 },
 #if LIBAVCODEC_VERSION_MAJOR < 58
   { L"nut", AV_CODEC_ID_VOXWARE },
 #endif
   { L"nut", AV_CODEC_ID_AAC },
   { L"nut", AV_CODEC_ID_WMAV1 },
   { L"nut", AV_CODEC_ID_WMAV2 },
   { L"nut", AV_CODEC_ID_WMAPRO },
   { L"nut", AV_CODEC_ID_ADPCM_CT },
   { L"nut", AV_CODEC_ID_ATRAC3 },
   { L"nut", AV_CODEC_ID_IMC },
   { L"nut", AV_CODEC_ID_AC3 },
   { L"nut", AV_CODEC_ID_DTS },
   { L"nut", AV_CODEC_ID_FLAC },
   { L"nut", AV_CODEC_ID_ADPCM_SWF },
   { L"nut", AV_CODEC_ID_VORBIS },

   { L"ogg", AV_CODEC_ID_VORBIS },
   { L"ogg", AV_CODEC_ID_FLAC },

   { L"ac3", AV_CODEC_ID_AC3 },

   { L"dts", AV_CODEC_ID_DTS },

   { L"flac", AV_CODEC_ID_FLAC },

   { L"RoQ", AV_CODEC_ID_ROQ_DPCM },

   { L"rm", AV_CODEC_ID_AC3 },

   { L"swf", AV_CODEC_ID_MP3 },

   { L"avm2", AV_CODEC_ID_MP3 },

   { L"voc", AV_CODEC_ID_PCM_U8 },

   { L"wav", AV_CODEC_ID_PCM_S16LE },
   { L"wav", AV_CODEC_ID_PCM_U8 },
   { L"wav", AV_CODEC_ID_PCM_S24LE },
   { L"wav", AV_CODEC_ID_PCM_S32LE },
   { L"wav", AV_CODEC_ID_ADPCM_MS },
   { L"wav", AV_CODEC_ID_PCM_ALAW },
   { L"wav", AV_CODEC_ID_PCM_MULAW },
   { L"wav", AV_CODEC_ID_WMAVOICE },
   { L"wav", AV_CODEC_ID_ADPCM_IMA_WAV },
   { L"wav", AV_CODEC_ID_ADPCM_YAMAHA },
   { L"wav", AV_CODEC_ID_TRUESPEECH },
   { L"wav", AV_CODEC_ID_GSM_MS },
   { L"wav", AV_CODEC_ID_ADPCM_G726 },
   //{ L"wav", AV_CODEC_ID_MP2 }, Bug 59 - It crashes.
   { L"wav", AV_CODEC_ID_MP3 },
#if LIBAVCODEC_VERSION_MAJOR < 58
   { L"wav", AV_CODEC_ID_VOXWARE },
#endif
   { L"wav", AV_CODEC_ID_AAC },
   // { L"wav", AV_CODEC_ID_WMAV1 },
   // { L"wav", AV_CODEC_ID_WMAV2 },
   { L"wav", AV_CODEC_ID_WMAPRO },
   { L"wav", AV_CODEC_ID_ADPCM_CT },
   { L"wav", AV_CODEC_ID_ATRAC3 },
   { L"wav", AV_CODEC_ID_IMC },
   { L"wav", AV_CODEC_ID_AC3 },
   //{ L"wav", AV_CODEC_ID_DTS },
   { L"wav", AV_CODEC_ID_FLAC },
   { L"wav", AV_CODEC_ID_ADPCM_SWF },
   { L"wav", AV_CODEC_ID_VORBIS },

   { NULL, AV_CODEC_ID_NONE }
};

/// AAC profiles
// The FF_PROFILE_* enumeration is defined in the ffmpeg library
// PRL:  I cant find where this preference is used!
ChoiceSetting AACProfiles { L"/FileFormats/FFmpegAACProfile",
   {
      {L"1" /*FF_PROFILE_AAC_LOW*/, XO("LC")},
      {L"0" /*FF_PROFILE_AAC_MAIN*/, XO("Main")},
      // {L"2" /*FF_PROFILE_AAC_SSR*/, XO("SSR")}, //SSR is not supported
      {L"3" /*FF_PROFILE_AAC_LTP*/, XO("LTP")},
   },
   0, // "1"
};

/// List of export types
ExposedFormat ExportFFmpegOptions::fmts[] =
{
   {FMT_M4A,   L"M4A",    L"m4a",  L"ipod", 48,  AV_CANMETA,              true,  XO("M4A (AAC) Files (FFmpeg)"),         AV_CODEC_ID_AAC,    true},
   {FMT_AC3,   L"AC3",    L"ac3",  L"ac3",  7,   AV_VERSION_INT(0,0,0),   false, XO("AC3 Files (FFmpeg)"),               AV_CODEC_ID_AC3,    true},
   {FMT_AMRNB, L"AMRNB",  L"amr",  L"amr",  1,   AV_VERSION_INT(0,0,0),   false, XO("AMR (narrow band) Files (FFmpeg)"), AV_CODEC_ID_AMR_NB, true},
   {FMT_OPUS,  L"OPUS",   L"opus", L"opus", 255, AV_CANMETA,              true,  XO("Opus (OggOpus) Files (FFmpeg)"),    AV_CODEC_ID_OPUS,   true},
   {FMT_WMA2,  L"WMA",    L"wma",  L"asf",  2,   AV_VERSION_INT(52,53,0), false, XO("WMA (version 2) Files (FFmpeg)"),   AV_CODEC_ID_WMAV2,  true},
   {FMT_OTHER, L"FFMPEG", L"",     L"",     255, AV_CANMETA,              true,  XO("Custom FFmpeg Export"),             AV_CODEC_ID_NONE,   true}
};

/// Some controls (parameters they represent) are only applicable to a number
/// of codecs and/or formats.
/// Syntax: first, enable a control for each applicable format-codec combination
/// then disable it for anything else
/// "any" - any format
/// AV_CODEC_ID_NONE - any codec
/// This list must end with {FALSE,FFmpegExportCtrlID(0),AV_CODEC_ID_NONE,NULL}
ApplicableFor ExportFFmpegOptions::apptable[] =
{
   {TRUE,FEQualityID,AV_CODEC_ID_AAC,"any"},
   {TRUE,FEQualityID,AV_CODEC_ID_MP3,"any"},
   {TRUE,FEQualityID,AV_CODEC_ID_VORBIS,"any"},
   {FALSE,FEQualityID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FECutoffID,AV_CODEC_ID_AC3,"any"},
   {TRUE,FECutoffID,AV_CODEC_ID_AAC,"any"},
   {TRUE,FECutoffID,AV_CODEC_ID_VORBIS,"any"},
   {FALSE,FECutoffID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEFrameSizeID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEFrameSizeID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEProfileID,AV_CODEC_ID_AAC,"any"},
   {FALSE,FEProfileID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FECompLevelID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FECompLevelID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEUseLPCID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEUseLPCID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FELPCCoeffsID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FELPCCoeffsID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEMinPredID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEMinPredID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEMaxPredID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEMaxPredID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEPredOrderID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEPredOrderID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEMinPartOrderID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEMinPartOrderID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEMaxPartOrderID,AV_CODEC_ID_FLAC,"any"},
   {FALSE,FEMaxPartOrderID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEMuxRateID,AV_CODEC_ID_NONE,"mpeg"},
   {TRUE,FEMuxRateID,AV_CODEC_ID_NONE,"vcd"},
   {TRUE,FEMuxRateID,AV_CODEC_ID_NONE,"vob"},
   {TRUE,FEMuxRateID,AV_CODEC_ID_NONE,"svcd"},
   {TRUE,FEMuxRateID,AV_CODEC_ID_NONE,"dvd"},
   {FALSE,FEMuxRateID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEPacketSizeID,AV_CODEC_ID_NONE,"mpeg"},
   {TRUE,FEPacketSizeID,AV_CODEC_ID_NONE,"vcd"},
   {TRUE,FEPacketSizeID,AV_CODEC_ID_NONE,"vob"},
   {TRUE,FEPacketSizeID,AV_CODEC_ID_NONE,"svcd"},
   {TRUE,FEPacketSizeID,AV_CODEC_ID_NONE,"dvd"},
   {FALSE,FEPacketSizeID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"matroska"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"mov"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"3gp"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"mp4"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"psp"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"3g2"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"ipod"},
   {TRUE,FELanguageID,AV_CODEC_ID_NONE,"mpegts"},
   {FALSE,FELanguageID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEBitReservoirID,AV_CODEC_ID_MP3,"any"},
   {TRUE,FEBitReservoirID,AV_CODEC_ID_WMAV1,"any"},
   {TRUE,FEBitReservoirID,AV_CODEC_ID_WMAV2,"any"},
   {FALSE,FEBitReservoirID,AV_CODEC_ID_NONE,"any"},

   {TRUE,FEVariableBlockLenID,AV_CODEC_ID_WMAV1,"any"},
   {TRUE,FEVariableBlockLenID,AV_CODEC_ID_WMAV2,"any"},
   {FALSE,FEVariableBlockLenID,AV_CODEC_ID_NONE,"any"},

   {FALSE,FFmpegExportCtrlID(0),AV_CODEC_ID_NONE,NULL}
};

namespace {

/// Prediction order method - names.
const TranslatableStrings PredictionOrderMethodNames {
   XO("Estimate"),
   XO("2-level"),
   XO("4-level"),
   XO("8-level"),
   XO("Full search"),
   XO("Log search"),
};

}



ExportFFmpegOptions::~ExportFFmpegOptions()
{
   DropFFmpegLibs();
}

ExportFFmpegOptions::ExportFFmpegOptions(wxWindow *parent)
:  wxDialogWrapper(parent, wxID_ANY,
            XO("Configure custom FFmpeg options"))
{
   SetName();
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PickFFmpegLibs();
   //FFmpegLibsInst()->LoadLibs(NULL,true); //Loaded at startup or from Prefs now

   mPresets = std::make_unique<FFmpegPresets>();
   mPresets->GetPresetList(mPresetNames);

   if (FFmpegLibsInst()->ValidLibsLoaded())
   {
      FetchFormatList();
      FetchCodecList();

      PopulateOrExchange(S);

      //Select the format that was selected last time this dialog was closed
      mFormatList->Select(mFormatList->FindString(gPrefs->Read(L"/FileFormats/FFmpegFormat")));
      DoOnFormatList();

      //Select the codec that was selected last time this dialog was closed
      AVCodec *codec = avcodec_find_encoder_by_name(gPrefs->Read(L"/FileFormats/FFmpegCodec").ToUTF8());
      if (codec != NULL) mCodecList->Select(mCodecList->FindString(wxString::FromUTF8(codec->name)));
      DoOnCodecList();
   }

}

///
///
void ExportFFmpegOptions::FetchFormatList()
{
   // Enumerate all output formats
   AVOutputFormat *ofmt = NULL;
   while ((ofmt = av_oformat_next(ofmt))!=NULL)
   {
      // Any audio-capable format has default audio codec.
      // If it doesn't, then it doesn't supports any audio codecs
      if (ofmt->audio_codec != AV_CODEC_ID_NONE)
      {
         mFormatNames.push_back(wxString::FromUTF8(ofmt->name));
         mFormatLongNames.push_back(wxString::Format(L"%s - %s",mFormatNames.back(),wxString::FromUTF8(ofmt->long_name)));
      }
   }
   // Show all formats
   mShownFormatNames = mFormatNames;
   mShownFormatLongNames =  mFormatLongNames;
}

///
///
void ExportFFmpegOptions::FetchCodecList()
{
   // Enumerate all codecs
   AVCodec *codec = NULL;
   while ((codec = av_codec_next(codec))!=NULL)
   {
      // We're only interested in audio and only in encoders
      if (codec->type == AVMEDIA_TYPE_AUDIO && av_codec_is_encoder(codec))
      {
         // MP2 Codec is broken.  Don't allow it.
         if( codec->id == AV_CODEC_ID_MP2)
            continue;
         mCodecNames.push_back(wxString::FromUTF8(codec->name));
         mCodecLongNames.push_back(wxString::Format(L"%s - %s",mCodecNames.back(),wxString::FromUTF8(codec->long_name)));
      }
   }
   // Show all codecs
   mShownCodecNames = mCodecNames;
   mShownCodecLongNames = mCodecLongNames;
}

///
///
void ExportFFmpegOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartVerticalLay(1);
   S.StartMultiColumn(1, wxEXPAND);
   {
      S.SetStretchyRow(3);
      S.StartMultiColumn(7, wxEXPAND);
      {
         S.SetStretchyCol(1);
         mPresetCombo = S.Id(FEPresetID).AddCombo(XXO("Preset:"), gPrefs->Read(L"/FileFormats/FFmpegPreset",wxEmptyString), mPresetNames);
         S.Id(FELoadPresetID).AddButton(XXO("Load Preset"));
         S.Id(FESavePresetID).AddButton(XXO("Save Preset"));
         S.Id(FEDeletePresetID).AddButton(XXO("Delete Preset"));
         S.Id(FEImportPresetsID).AddButton(XXO("Import Presets"));
         S.Id(FEExportPresetsID).AddButton(XXO("Export Presets"));
      }
      S.EndMultiColumn();
      S.StartMultiColumn(4, wxALIGN_LEFT);
      {
         S.SetStretchyCol(1);
         S.SetStretchyCol(3);
         S.Id(FEFormatLabelID).AddFixedText(XO("Format:"));
         mFormatName = S.Id(FEFormatNameID).AddVariableText( {} );
         /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
         S.Id(FECodecLabelID).AddFixedText(XO("Codec:"));
         mCodecName = S.Id(FECodecNameID).AddVariableText( {} );
      }
      S.EndMultiColumn();
      S.AddVariableText(XO(
"Not all formats and codecs are compatible. Nor are all option combinations compatible with all codecs."),
         false);
      S.StartMultiColumn(2, wxEXPAND);
      {
         S.StartMultiColumn(2, wxEXPAND);
         {
            S.SetStretchyRow(1);
            S.Id(FEAllFormatsID).AddButton(XXO("Show All Formats"));
            S.Id(FEAllCodecsID).AddButton(XXO("Show All Codecs"));
            mFormatList = S.Id(FEFormatID).AddListBox(mFormatNames);
            mFormatList->DeselectAll();
            mCodecList = S.Id(FECodecID).AddListBox(mCodecNames);
            mCodecList->DeselectAll();
         }
         S.EndMultiColumn();
         S.StartVerticalLay();
         {
            //S.StartScroller( );
            S.SetBorder( 3 );
            S.StartStatic(XO("General Options"), 0);
            {
               S.StartMultiColumn(8, wxEXPAND);
               {
                  S.Id(FELanguageID)
                     .ToolTip(XO("ISO 639 3-letter language code\nOptional\nempty - automatic"))
                     .TieTextBox(XXO("Language:"), {L"/FileFormats/FFmpegLanguage", wxEmptyString}, 9);

                  S.AddSpace( 20,0 );
                  S.AddVariableText(XO("Bit Reservoir"));
                  S.Id(FEBitReservoirID).TieCheckBox( {}, {L"/FileFormats/FFmpegBitReservoir", true});

                  S.AddSpace( 20,0 );
                  S.AddVariableText(XO("VBL"));
                  S.Id(FEVariableBlockLenID).TieCheckBox( {}, {L"/FileFormats/FFmpegVariableBlockLen", true});
               }
               S.EndMultiColumn();
               S.StartMultiColumn(4, wxALIGN_LEFT);
               {
                  S.Id(FETagID)
                     /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
                     .ToolTip(XO("Codec tag (FOURCC)\nOptional\nempty - automatic"))
                     .TieTextBox(XXO("Tag:"), {L"/FileFormats/FFmpegTag", wxEmptyString}, 4);

                  S.Id(FEBitrateID)
                     .ToolTip(XO("Bit Rate (bits/second) - influences the resulting file size and quality\nSome codecs may only accept specific values (128k, 192k, 256k etc)\n0 - automatic\nRecommended - 192000"))
                     .TieSpinCtrl(XXO("Bit Rate:"), {L"/FileFormats/FFmpegBitRate", 0}, 1000000, 0);

                  S.Id(FEQualityID)
                     .ToolTip(XO("Overall quality, used differently by different codecs\nRequired for vorbis\n0 - automatic\n-1 - off (use bitrate instead)"))
                     .TieSpinCtrl(XXO("Quality:"), {L"/FileFormats/FFmpegQuality", 0}, 500, -1);

                  S.Id(FESampleRateID)
                     .ToolTip(XO("Sample rate (Hz)\n0 - don't change sample rate"))
                     .TieSpinCtrl(XXO("Sample Rate:"), {L"/FileFormats/FFmpegSampleRate", 0}, 200000, 0);

                  S.Id(FECutoffID)
                     .ToolTip(XO("Audio cutoff bandwidth (Hz)\nOptional\n0 - automatic"))
                     .TieSpinCtrl(XXO("Cutoff:"), {L"/FileFormats/FFmpegCutOff", 0}, 10000000, 0);

                  S.Id(FEProfileID)
                     .ToolTip(XO("AAC Profile\nLow Complexity - default\nMost players won't play anything other than LC"))
                     .MinSize( { 100, -1 } )
                     .TieChoice(XXO("Profile:"), AACProfiles);
               }
               S.EndMultiColumn();
            }
            S.EndStatic();
            S.StartStatic(XO("FLAC options"),0);
            {
               S.StartMultiColumn(4, wxALIGN_LEFT);
               {
                  S
                     .ToolTip(XO("Compression level\nRequired for FLAC\n-1 - automatic\nmin - 0 (fast encoding, large output file)\nmax - 10 (slow encoding, small output file)"))
                     .Id(FECompLevelID).TieSpinCtrl(XXO("Compression:"), {L"/FileFormats/FFmpegCompLevel", 0}, 10, -1);

                  S.Id(FEFrameSizeID)
                     .ToolTip(XO("Frame size\nOptional\n0 - default\nmin - 16\nmax - 65535"))
                     .TieSpinCtrl(XXO("Frame:"), {L"/FileFormats/FFmpegFrameSize", 0}, 65535, 0);

                  S.Id(FELPCCoeffsID)
                     .ToolTip(XO("LPC coefficients precision\nOptional\n0 - default\nmin - 1\nmax - 15"))
                     .TieSpinCtrl(XXO("LPC"), {L"/FileFormats/FFmpegLPCCoefPrec", 0}, 15, 0);

                  S.Id(FEPredOrderID)
                     .ToolTip(XO("Prediction Order Method\nEstimate - fastest, lower compression\nLog search - slowest, best compression\nFull search - default"))
                     .MinSize( { 100, -1 } )
                     .TieNumberAsChoice(
                        XXO("PdO Method:"),
                        {L"/FileFormats/FFmpegPredOrderMethod",
                         4}, // Full search
                        PredictionOrderMethodNames
                     );

                  S.Id(FEMinPredID)
                     .ToolTip(XO("Minimal prediction order\nOptional\n-1 - default\nmin - 0\nmax - 32 (with LPC) or 4 (without LPC)"))
                     .TieSpinCtrl(XXO("Min. PdO"), {L"/FileFormats/FFmpegMinPredOrder", -1}, 32, -1);

                  S.Id(FEMaxPredID)
                     .ToolTip(XO("Maximal prediction order\nOptional\n-1 - default\nmin - 0\nmax - 32 (with LPC) or 4 (without LPC)"))
                     .TieSpinCtrl(XXO("Max. PdO"), {L"/FileFormats/FFmpegMaxPredOrder", -1}, 32, -1);

                  S.Id(FEMinPartOrderID)
                     .ToolTip(XO("Minimal partition order\nOptional\n-1 - default\nmin - 0\nmax - 8"))
                     .TieSpinCtrl(XXO("Min. PtO"), {L"/FileFormats/FFmpegMinPartOrder", -1}, 8, -1);

                  S.Id(FEMaxPartOrderID)
                     .ToolTip(XO("Maximal partition order\nOptional\n-1 - default\nmin - 0\nmax - 8"))
                     .TieSpinCtrl(XXO("Max. PtO"), {L"/FileFormats/FFmpegMaxPartOrder", -1}, 8, -1);

                  /* i18n-hint:  Abbreviates "Linear Predictive Coding",
                     but this text needs to be kept very short */
                  S.AddVariableText(XO("Use LPC"));
                  // PRL:  This preference is not used anywhere!
                  S.Id(FEUseLPCID).TieCheckBox( {}, {L"/FileFormats/FFmpegUseLPC", true});
               }
               S.EndMultiColumn();
            }
            S.EndStatic();
            S.StartStatic(XO("MPEG container options"),0);
            {
               S.StartMultiColumn(4, wxALIGN_LEFT);
               {
                  S.Id(FEMuxRateID)
                     .ToolTip(XO("Maximum bit rate of the multiplexed stream\nOptional\n0 - default"))
                     /* i18n-hint: 'mux' is short for multiplexor, a device that selects between several inputs
                       'Mux Rate' is a parameter that has some bearing on compression ratio for MPEG
                       it has a hard to predict effect on the degree of compression */
                     .TieSpinCtrl(XXO("Mux Rate:"), {L"/FileFormats/FFmpegMuxRate", 0}, 10000000, 0);

                  S.Id(FEPacketSizeID)
                     /* i18n-hint: 'Packet Size' is a parameter that has some bearing on compression ratio for MPEG
                       compression.  It measures how big a chunk of audio is compressed in one piece. */
                     .ToolTip(XO("Packet size\nOptional\n0 - default"))
                     /* i18n-hint: 'Packet Size' is a parameter that has some bearing on compression ratio for MPEG
                       compression.  It measures how big a chunk of audio is compressed in one piece. */
                     .TieSpinCtrl(XXO("Packet Size:"), {L"/FileFormats/FFmpegPacketSize", 0}, 10000000, 0);
               }
               S.EndMultiColumn();
            }
            S.EndStatic();
            //S.EndScroller();
            S.SetBorder( 5 );
            S.AddStandardButtons(eOkButton | eCancelButton | eHelpButton );
         }
         S.EndVerticalLay();
      }
      S.EndMultiColumn();
   }
   S.EndMultiColumn();
   S.EndVerticalLay();

   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();

   return;
}

///
///
void ExportFFmpegOptions::FindSelectedFormat(wxString **name, wxString **longname)
{
   // Get current selection
   wxArrayInt selections;
   int n = mFormatList->GetSelections(selections);
   if (n <= 0) return;

   // Get selected format short name
   wxString selfmt = mFormatList->GetString(selections[0]);

   // Find its index
   int nFormat = make_iterator_range( mFormatNames ).index( selfmt );
   if (nFormat == wxNOT_FOUND) return;

   // Return short name and description
   if (name != NULL) *name = &mFormatNames[nFormat];
   if (longname != NULL) *longname = &mFormatLongNames[nFormat];
   return;
}
///
///
void ExportFFmpegOptions::FindSelectedCodec(wxString **name, wxString **longname)
{
   // Get current selection
   wxArrayInt selections;
   int n = mCodecList->GetSelections(selections);
   if (n <= 0) return;

   // Get selected codec short name
   wxString selcdc = mCodecList->GetString(selections[0]);

   // Find its index
   int nCodec = make_iterator_range( mCodecNames ).index( selcdc );
   if (nCodec == wxNOT_FOUND) return;

   // Return short name and description
   if (name != NULL) *name = &mCodecNames[nCodec];
   if (longname != NULL) *longname = &mCodecLongNames[nCodec];
}

///
///
int ExportFFmpegOptions::FetchCompatibleCodecList(const wxChar *fmt, AVCodecID id)
{
   // By default assume that id is not in the list
   int index = -1;
   // By default no codecs are compatible (yet)
   mShownCodecNames.clear();
   mShownCodecLongNames.clear();
   // Clear the listbox
   mCodecList->Clear();
   // Zero - format is not found at all
   int found = 0;
   wxString str(fmt);
   for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
   {
      if (str == CompatibilityList[i].fmt)
      {
         // Format is found in the list
         found = 1;
         if (CompatibilityList[i].codec == AV_CODEC_ID_NONE)
         {
            // Format is found in the list and it is compatible with AV_CODEC_ID_NONE (means that it is compatible to anything)
            found = 2;
            break;
         }
         // Find the codec, that is claimed to be compatible
         AVCodec *codec = avcodec_find_encoder(CompatibilityList[i].codec);
         // If it exists, is audio and has encoder
         if (codec != NULL && (codec->type == AVMEDIA_TYPE_AUDIO) && av_codec_is_encoder(codec))
         {
            // If it was selected - remember its NEW index
            if ((id >= 0) && codec->id == id) index = mShownCodecNames.size();
            mShownCodecNames.push_back(wxString::FromUTF8(codec->name));
            mShownCodecLongNames.push_back(wxString::Format(L"%s - %s",mShownCodecNames.back(),wxString::FromUTF8(codec->long_name)));
         }
      }
   }
   // All codecs are compatible with this format
   if (found == 2)
   {
      AVCodec *codec = NULL;
      while ((codec = av_codec_next(codec))!=NULL)
      {
         if (codec->type == AVMEDIA_TYPE_AUDIO && av_codec_is_encoder(codec))
         {
            // MP2 is broken.
            if( codec->id == AV_CODEC_ID_MP2)
               continue;
            if (! make_iterator_range( mShownCodecNames )
               .contains( wxString::FromUTF8(codec->name) ) )
            {
               if ((id >= 0) && codec->id == id) index = mShownCodecNames.size();
               mShownCodecNames.push_back(wxString::FromUTF8(codec->name));
               mShownCodecLongNames.push_back(wxString::Format(L"%s - %s",mShownCodecNames.back(),wxString::FromUTF8(codec->long_name)));
            }
         }
      }
   }
   // Format is not found - find format in libavformat and add its default audio codec
   // This allows us to provide limited support for NEW formats without modifying the compatibility list
   else if (found == 0)
   {
      wxCharBuffer buf = str.ToUTF8();
      AVOutputFormat *format = av_guess_format(buf,NULL,NULL);
      if (format != NULL)
      {
         AVCodec *codec = avcodec_find_encoder(format->audio_codec);
         if (codec != NULL && (codec->type == AVMEDIA_TYPE_AUDIO) && av_codec_is_encoder(codec))
         {
            if ((id >= 0) && codec->id == id) index = mShownCodecNames.size();
            mShownCodecNames.push_back(wxString::FromUTF8(codec->name));
            mShownCodecLongNames.push_back(wxString::Format(L"%s - %s",mShownCodecNames.back(),wxString::FromUTF8(codec->long_name)));
         }
      }
   }
   // Show NEW codec list
   mCodecList->Append(mShownCodecNames);

   return index;
}

///
///
int ExportFFmpegOptions::FetchCompatibleFormatList(AVCodecID id, wxString *selfmt)
{
   int index = -1;
   mShownFormatNames.clear();
   mShownFormatLongNames.clear();
   mFormatList->Clear();
   AVOutputFormat *ofmt = NULL;
   ofmt = NULL;
   wxArrayString FromList;
   // Find all formats compatible to this codec in compatibility list
   for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
   {
      if (CompatibilityList[i].codec == id || (CompatibilityList[i].codec == AV_CODEC_ID_NONE) )
      {
         if ((selfmt != NULL) && (*selfmt == CompatibilityList[i].fmt)) index = mShownFormatNames.size();
         FromList.push_back(CompatibilityList[i].fmt);
         mShownFormatNames.push_back(CompatibilityList[i].fmt);
         AVOutputFormat *tofmt = av_guess_format(wxString(CompatibilityList[i].fmt).ToUTF8(),NULL,NULL);
         if (tofmt != NULL) mShownFormatLongNames.push_back(wxString::Format(L"%s - %s",CompatibilityList[i].fmt,wxString::FromUTF8(tofmt->long_name)));
      }
   }
   bool found = false;
   if (selfmt != NULL)
   {
      for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
      {
         if (*selfmt == CompatibilityList[i].fmt)
         {
            found = true;
            break;
         }
      }
   }
   // Format was in the compatibility list
   if (found)
   {
      // Find all formats which have this codec as default and which are not in the list yet and add them too
      while ((ofmt = av_oformat_next(ofmt))!=NULL)
      {
         if (ofmt->audio_codec == id)
         {
            wxString ofmtname = wxString::FromUTF8(ofmt->name);
            found = false;
            for (unsigned int i = 0; i < FromList.size(); i++)
            {
               if (ofmtname == FromList[i])
               {
                  found = true;
                  break;
               }
            }
            if (!found)
            {
               if ((selfmt != NULL) &&
                  (*selfmt == wxString::FromUTF8(ofmt->name)))
                  index = mShownFormatNames.size();
               mShownFormatNames.push_back(wxString::FromUTF8(ofmt->name));
               mShownFormatLongNames.push_back(wxString::Format(L"%s - %s",mShownFormatNames.back(),wxString::FromUTF8(ofmt->long_name)));
            }
         }
      }
   }
   mFormatList->Append(mShownFormatNames);
   return index;
}

///
///
void ExportFFmpegOptions::OnDeletePreset(wxCommandEvent& WXUNUSED(event))
{
   wxComboBox *preset = dynamic_cast<wxComboBox*>(FindWindowById(FEPresetID,this));
   wxString presetname = preset->GetValue();
   if (presetname.empty())
   {
      AudacityMessageBox( XO("You can't delete a preset without name") );
      return;
   }

   auto query = XO("Delete preset '%s'?").Format( presetname );
   int action = AudacityMessageBox(
      query,
      XO("Confirm Deletion"),
      wxYES_NO | wxCENTRE);
   if (action == wxNO) return;

   mPresets->DeletePreset(presetname);
   long index = preset->FindString(presetname);
   preset->SetValue(wxEmptyString);
   preset->Delete(index);
   mPresetNames.erase(
      std::find( mPresetNames.begin(), mPresetNames.end(), presetname )
   );
}

///
///
void ExportFFmpegOptions::OnSavePreset(wxCommandEvent& WXUNUSED(event))
{  const bool kCheckForOverwrite = true;
   SavePreset(kCheckForOverwrite);
}

// Return false if failed to save.
bool ExportFFmpegOptions::SavePreset(bool bCheckForOverwrite)
{
   wxComboBox *preset = dynamic_cast<wxComboBox*>(FindWindowById(FEPresetID,this));
   wxString name = preset->GetValue();
   if (name.empty())
   {
      AudacityMessageBox( XO("You can't save a preset without a name") );
      return false;
   }
   if( bCheckForOverwrite && !mPresets->OverwriteIsOk(name))
      return false;
   if( !mPresets->SavePreset(this,name) )
      return false;
   int index = mPresetNames.Index(name,false);
   if (index == -1)
   {
      mPresetNames.push_back(name);
      mPresetCombo->Clear();
      mPresetCombo->Append(mPresetNames);
      mPresetCombo->Select(mPresetNames.Index(name,false));
   }
   return true;
}

///
///
void ExportFFmpegOptions::OnLoadPreset(wxCommandEvent& WXUNUSED(event))
{
   wxComboBox *preset = dynamic_cast<wxComboBox*>(FindWindowById(FEPresetID,this));
   wxString presetname = preset->GetValue();

   mShownFormatNames = mFormatNames;
   mShownFormatLongNames = mFormatLongNames;
   mFormatList->Clear();
   mFormatList->Append(mFormatNames);

   mShownCodecNames = mCodecNames;
   mShownCodecLongNames = mCodecLongNames;
   mCodecList->Clear();
   mCodecList->Append(mCodecNames);

   mPresets->LoadPreset(this,presetname);

   DoOnFormatList();
   DoOnCodecList();
}

static const FileNames::FileTypes &FileTypes()
{
   static const FileNames::FileTypes result{
      FileNames::XMLFiles, FileNames::AllFiles };
   return result;
};

///
///
void ExportFFmpegOptions::OnImportPresets(wxCommandEvent& WXUNUSED(event))
{
   wxString path;
   FileDialogWrapper dlg(this,
      XO("Select xml file with presets to import"),
      gPrefs->Read(L"/FileFormats/FFmpegPresetDir"),
      wxEmptyString,
      FileTypes(),
      wxFD_OPEN);
   if (dlg.ShowModal() == wxID_CANCEL) return;
   path = dlg.GetPath();
   mPresets->ImportPresets(path);
   mPresets->GetPresetList(mPresetNames);
   mPresetCombo->Clear();
   mPresetCombo->Append(mPresetNames);
}

///
///
void ExportFFmpegOptions::OnExportPresets(wxCommandEvent& WXUNUSED(event))
{
   const bool kCheckForOverwrite = true;
   // Bug 1180 save any pending preset before exporting the lot.
   // If saving fails, don't try to export.
   if( !SavePreset(!kCheckForOverwrite) )
      return;

   wxArrayString presets;
   mPresets->GetPresetList( presets);
   if( presets.Count() < 1)
   {
      AudacityMessageBox( XO("No presets to export") );
      return;
   }

   wxString path;
   FileDialogWrapper dlg(this,
      XO("Select xml file to export presets into"),
      gPrefs->Read(L"/FileFormats/FFmpegPresetDir"),
      wxEmptyString,
      FileTypes(),
      wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
   if (dlg.ShowModal() == wxID_CANCEL) return;
   path = dlg.GetPath();
   mPresets->ExportPresets(path);
}

///
///
void ExportFFmpegOptions::OnAllFormats(wxCommandEvent& WXUNUSED(event))
{
   mShownFormatNames = mFormatNames;
   mShownFormatLongNames = mFormatLongNames;
   mFormatList->Clear();
   mFormatList->Append(mFormatNames);
}

///
///
void ExportFFmpegOptions::OnAllCodecs(wxCommandEvent& WXUNUSED(event))
{
   mShownCodecNames = mCodecNames;
   mShownCodecLongNames = mCodecLongNames;
   mCodecList->Clear();
   mCodecList->Append(mCodecNames);
}

/// ReportIfBadCombination will trap 
/// bad combinations of format and codec and report
/// using a message box.
/// We may later extend it to catch bad parameters too.
/// @return true iff a bad combination was reported
/// At the moment we don't trap unrecognised format
/// or codec.  (We do not expect them to happen ever).
bool ExportFFmpegOptions::ReportIfBadCombination()
{
   wxString *selcdc = NULL;
   wxString *selcdclong = NULL;
   FindSelectedCodec(&selcdc, &selcdclong);
   if (selcdc == NULL)
      return false; // unrecognised codec. Treated as OK
   AVCodec *cdc = avcodec_find_encoder_by_name(selcdc->ToUTF8());
   if (cdc == NULL)
      return false; // unrecognised codec. Treated as OK

   wxString *selfmt = NULL;
   wxString *selfmtlong = NULL;
   FindSelectedFormat(&selfmt, &selfmtlong);
   if( selfmt == NULL )
      return false; // unrecognised format; Treated as OK
   
   // This is intended to test for illegal combinations.
   // However, the list updating now seems to be working correctly
   // making it impossible to select illegal combinations
   bool bFound = false;
   for (int i = 0; CompatibilityList[i].fmt != NULL; i++)
   {
      if (*selfmt == CompatibilityList[i].fmt) 
      {
         if (CompatibilityList[i].codec == cdc->id || (CompatibilityList[i].codec == AV_CODEC_ID_NONE) ){
            bFound = true;
            break;
         }
      }
   }

   // We can put extra code in here, to disallow combinations
   // We could also test for illegal parameters, and deliver
   // custom error messages in that case.
   // The below would make AAC codec disallowed.
   //if( cdc->id == AV_CODEC_ID_AAC)
   //   bFound = false;

   // Valid combination was found, so no reporting.
   if( bFound )
      return false;

   AudacityMessageBox(
      /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
      XO("Format %s is not compatible with codec %s.")
         .Format( *selfmt, *selcdc ),
      /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
      XO("Incompatible format and codec"));

   return true;
}



void ExportFFmpegOptions::EnableDisableControls(AVCodec *cdc, wxString *selfmt)
{
   int handled = -1;
   for (int i = 0; apptable[i].control != 0; i++)
   {
      if (apptable[i].control != handled)
      {
         bool codec = false;
         bool format = false;
         if (apptable[i].codec == AV_CODEC_ID_NONE) codec = true;
         else if (cdc != NULL && apptable[i].codec == cdc->id) codec = true;
         if (wxString::FromUTF8(apptable[i].format) == L"any") format = true;
         else if (selfmt != NULL &&
            *selfmt == wxString::FromUTF8(apptable[i].format)) format = true;
         if (codec && format)
         {
            handled = apptable[i].control;
            wxWindow *item = FindWindowById(apptable[i].control,this);
            if (item != NULL) item->Enable(apptable[i].enable);
         }
      }
   }
}

void ExportFFmpegOptions::DoOnFormatList()
{
   wxString *selfmt = NULL;
   wxString *selfmtlong = NULL;
   FindSelectedFormat(&selfmt, &selfmtlong);
   if (selfmt == NULL)
   {
      return;
   }

   wxString *selcdc = NULL;
   wxString *selcdclong = NULL;
   FindSelectedCodec(&selcdc, &selcdclong);

   AVOutputFormat *fmt = av_guess_format(selfmt->ToUTF8(),NULL,NULL);
   if (fmt == NULL)
   {
      //This shouldn't really happen
      mFormatName->SetLabel(wxString(_("Failed to guess format")));
      return;
   }
   mFormatName->SetLabel(wxString::Format(L"%s", *selfmtlong));
   int selcdcid = -1;

   if (selcdc != NULL)
   {
      AVCodec *cdc = avcodec_find_encoder_by_name(selcdc->ToUTF8());
      if (cdc != NULL)
      {
         selcdcid = cdc->id;
      }
   }
   int newselcdc = FetchCompatibleCodecList(*selfmt, (AVCodecID)selcdcid);
   if (newselcdc >= 0) mCodecList->Select(newselcdc);

   AVCodec *cdc = NULL;
   if (selcdc != NULL)
      cdc = avcodec_find_encoder_by_name(selcdc->ToUTF8());
   EnableDisableControls(cdc, selfmt);
   Layout();
   Fit();
   return;
}

void ExportFFmpegOptions::DoOnCodecList()
{
   wxString *selcdc = NULL;
   wxString *selcdclong = NULL;
   FindSelectedCodec(&selcdc, &selcdclong);
   if (selcdc == NULL)
   {
      return;
   }

   wxString *selfmt = NULL;
   wxString *selfmtlong = NULL;
   FindSelectedFormat(&selfmt, &selfmtlong);

   AVCodec *cdc = avcodec_find_encoder_by_name(selcdc->ToUTF8());
   if (cdc == NULL)
   {
      //This shouldn't really happen
      /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
      mCodecName->SetLabel(wxString(_("Failed to find the codec")));
      return;
   }
   mCodecName->SetLabel(wxString::Format(L"[%d] %s", (int) cdc->id, *selcdclong));

   if (selfmt != NULL)
   {
      AVOutputFormat *fmt = av_guess_format(selfmt->ToUTF8(),NULL,NULL);
      if (fmt == NULL)
      {
         selfmt = NULL;
         selfmtlong = NULL;
      }
   }

   int newselfmt = FetchCompatibleFormatList(cdc->id,selfmt);
   if (newselfmt >= 0) mFormatList->Select(newselfmt);

   EnableDisableControls(cdc, selfmt);
   Layout();
   Fit();
   return;
}

///
///
void ExportFFmpegOptions::OnFormatList(wxCommandEvent& WXUNUSED(event))
{
   DoOnFormatList();
}

///
///
void ExportFFmpegOptions::OnCodecList(wxCommandEvent& WXUNUSED(event))
{
   DoOnCodecList();
}


///
///
void ExportFFmpegOptions::OnOK(wxCommandEvent& WXUNUSED(event))
{
   if( ReportIfBadCombination() )
      return;

   int selcdc = mCodecList->GetSelection();
   int selfmt = mFormatList->GetSelection();
   if (selcdc > -1) gPrefs->Write(L"/FileFormats/FFmpegCodec",mCodecList->GetString(selcdc));
   if (selfmt > -1) gPrefs->Write(L"/FileFormats/FFmpegFormat",mFormatList->GetString(selfmt));
   gPrefs->Flush();

   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Flush();

   EndModal(wxID_OK);

   return;
}

void ExportFFmpegOptions::OnGetURL(wxCommandEvent & WXUNUSED(event))
{
   HelpSystem::ShowHelp(this, L"Custom_FFmpeg_Export_Options");
}


#endif
