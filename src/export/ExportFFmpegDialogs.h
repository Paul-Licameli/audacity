/**********************************************************************

Audacity: A Digital Audio Editor

ExportFFmpegDialogs.h

LRN

**********************************************************************/

#if !defined(__EXPORT_FFMPEG_DIALOGS_H__)
#define __EXPORT_FFMPEG_DIALOGS_H__



#if defined(USE_FFMPEG)

#include "../FFmpeg.h"

#include "../xml/XMLFileReader.h"
#include "../FileNames.h"

#include <unordered_map>
#include "Identifier.h"

class wxArrayStringEx;

class wxArrayString;
class wxCheckBox;
class wxStaticText;
class wxTextCtrl;

/// Identifiers for pre-set export types.
enum FFmpegExposedFormat
{
   FMT_M4A,
   FMT_AC3,
   FMT_AMRNB,
   FMT_OPUS,
   FMT_WMA2,
   FMT_OTHER,
   FMT_LAST
};

#define AV_CANMETA (AV_VERSION_INT(255, 255, 255))

/// Describes export type
struct ExposedFormat
{
   FFmpegExposedFormat fmtid; //!< one of the FFmpegExposedFormat
   const wxChar *name;        //!< format name (internal, should be unique; if not - export dialog may show unusual behaviour)
   const FileExtension extension;   //!< default extension for this format. More extensions may be added later via AddExtension.
   const wxChar *shortname;   //!< used to guess the format
   unsigned maxchannels;      //!< how many channels this format could handle
   const int canmetadata;           //!< !=0 if format supports metadata, AV_CANMETA any avformat version, otherwise version support added
   bool canutf8;              //!< true if format supports metadata in UTF-8, false otherwise
   const TranslatableString description; //!< format description (will be shown in export dialog)
   AVCodecID codecid;         //!< codec ID (see libavcodec/avcodec.h)
   bool compiledIn;           //!< support for this codec/format is compiled in (checked at runtime)
};

/// AC3 export options dialog
class ExportFFmpegAC3Options final : public wxPanelWrapper
{
public:

   ExportFFmpegAC3Options(wxWindow *parent, int format);
   virtual ~ExportFFmpegAC3Options();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataFromWindow() override;

   /// Sample Rates supported by AC3 encoder (must end with zero-element)
   /// It is not used in dialog anymore, but will be required later
   static const int iAC3SampleRates[];

private:

   wxChoice *mBitRateChoice;
   int mBitRateFromChoice;
};

class ExportFFmpegAACOptions final : public wxPanelWrapper
{
public:

   ExportFFmpegAACOptions(wxWindow *parent, int format);
   virtual ~ExportFFmpegAACOptions();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataFromWindow() override;
};

class ExportFFmpegAMRNBOptions final : public wxPanelWrapper
{
public:

   ExportFFmpegAMRNBOptions(wxWindow *parent, int format);
   virtual ~ExportFFmpegAMRNBOptions();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataFromWindow() override;

private:

   wxChoice *mBitRateChoice;
   int mBitRateFromChoice;
};

class ExportFFmpegOPUSOptions final : public wxPanelWrapper
{
public:

   ExportFFmpegOPUSOptions(wxWindow *parent, int format);
   ~ExportFFmpegOPUSOptions();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

   static const int iOPUSSampleRates[];

private:

   wxSlider *mBitRateSlider;
   int mBitRateFromSlider;

   wxChoice *mVbrChoice;
   int mVbrFromChoice;

   wxSlider *mComplexitySlider;
   int mComplexityFromSlider;

   wxChoice *mFramesizeChoice;
   int mFramesizeFromChoice;

   wxChoice *mApplicationChoice;
   int mApplicationFromChoice;

   wxChoice *mCuttoffChoice;
   int mCutoffFromChoice;
};

class ExportFFmpegWMAOptions final : public wxPanelWrapper
{
public:

   ExportFFmpegWMAOptions(wxWindow *parent, int format);
   ~ExportFFmpegWMAOptions();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataFromWindow() override;

   static const int iWMASampleRates[];

private:

   wxChoice *mBitRateChoice;
   int mBitRateFromChoice;
};

class ExportFFmpegCustomOptions final : public wxPanelWrapper
{
public:

   ExportFFmpegCustomOptions(wxWindow *parent, int format);
   ~ExportFFmpegCustomOptions();

   void PopulateOrExchange(ShuttleGui & S);
   bool TransferDataToWindow() override; // remove this?
   bool TransferDataFromWindow() override;

   void OnOpen();

private:
   wxTextCtrl *mFormat;
   wxTextCtrl *mCodec;
};

class FFmpegPresets;

/// Custom FFmpeg export dialog
class ExportFFmpegOptions final : public wxDialogWrapper
{
public:

   ExportFFmpegOptions(wxWindow *parent);
   ~ExportFFmpegOptions();
   void PopulateOrExchange(ShuttleGui & S);
   void OnOK();
   void OnGetURL();
   void OnFormatList(wxCommandEvent& event);
   void DoOnFormatList();
   void OnCodecList(wxCommandEvent& event);
   void DoOnCodecList();
   void OnAllFormats();
   void OnAllCodecs();
   void OnSavePreset();
   void OnLoadPreset();
   void OnDeletePreset();
   void OnImportPresets();
   void OnExportPresets();
   bool SavePreset( bool bCheckForOverwrite);


   // Static tables
   static ExposedFormat fmts[];
   static const int iAACSampleRates[];

private:

   wxArrayString mShownFormatNames;
   wxArrayString mShownCodecNames;
   wxArrayStringEx mFormatNames;
   wxArrayString mFormatLongNames;
   wxArrayStringEx mCodecNames;
   wxArrayString mCodecLongNames;

   wxListBox *mFormatList;
   wxListBox *mCodecList;

   wxStaticText *mFormatName;
   wxStaticText *mCodecName;

   int mBitRateFromChoice;
   int mSampleRateFromChoice;

   std::unique_ptr<FFmpegPresets> mPresets;

   Identifiers mPresetNames;
   int mPresetNamesUpdated = 0;

   int FindSelectedFormat();
   int FindSelectedCodec();

   /// Retrieves format list from libavformat
   void FetchFormatList();

   /// Retrieves a list of formats compatible to codec
   ///\param id Codec ID
   ///\param selfmt format selected at the moment
   ///\return index of the selfmt in NEW format list or -1 if it is not in the list
   int FetchCompatibleFormatList(AVCodecID id, wxString *selfmt);

   /// Retrieves codec list from libavcodec
   void FetchCodecList();

   /// Retrieves a list of codecs compatible to format
   ///\param fmt Format short name
   ///\param id id of the codec selected at the moment
   ///\return index of the id in NEW codec list or -1 if it is not in the list
   int FetchCompatibleCodecList(const wxChar *fmt, AVCodecID id);

   /// Retrieves list of presets from configuration file
   void FetchPresetList();

   bool ReportIfBadCombination();

   DECLARE_EVENT_TABLE()
};

//----------------------------------------------------------------------------
// FFmpegPresets
//----------------------------------------------------------------------------

class FFmpegPreset
{
public:
   FFmpegPreset();
   ~FFmpegPreset();

   wxString mPresetName;
   wxArrayString mControlState;

};

using FFmpegPresetMap = std::unordered_map<wxString, FFmpegPreset>;

class FFmpegPresets : XMLTagHandler
{
public:
   FFmpegPresets();
   ~FFmpegPresets();

   void GetPresetList(Identifiers &list);
   void LoadPreset(ExportFFmpegOptions *parent, wxString &name);
   bool SavePreset(ExportFFmpegOptions *parent, wxString &name);
   void DeletePreset(wxString &name);
   bool OverwriteIsOk( wxString &name );
   FFmpegPreset *FindPreset(wxString &name);

   void ImportPresets(wxString &filename);
   void ExportPresets(wxString &filename);

   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   XMLTagHandler *HandleXMLChild(const wxChar *tag) override;
   void WriteXMLHeader(XMLWriter &xmlFile) const;
   void WriteXML(XMLWriter &xmlFile) const;

private:

   FFmpegPresetMap mPresets;
   FFmpegPreset *mPreset; // valid during XML parsing only
   bool mAbortImport; // tells importer to ignore the rest of the import
};

extern StringSetting
     FFmpegCodec
   , FFmpegFormat
;

#endif

extern BoolSetting
     FFmpegBitReservoir
   , FFmpegUseLPC // not used
   , FFmpegVariableBlockLen
;

extern IntSetting
     AACQuality
   , AC3BitRate
   , AMRNBBitRate
   , FFmpegBitRate
   , FFmpegCompLevel
   , FFmpegCutOff
   , FFmpegFrameSize
   , FFmpegLPCCoefPrec
   , FFmpegMaxPartOrder
   , FFmpegMinPartOrder
   , FFmpegMaxPredOrder
   , FFmpegMinPredOrder
   , FFmpegMuxRate
   , FFmpegPacketSize
   , FFmpegPredictionOrderMethod
   , FFmpegQuality
   , FFmpegSampleRate
   , WMABitRate
;

extern StringSetting
     FFmpegLanguage
   , FFmpegTag
;

#endif //__EXPORT_FFMPEG_DIALOGS_H__
