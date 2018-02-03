/**********************************************************************

Audacity: A Digital Audio Editor

FFmpeg.cpp

Audacity(R) is copyright (c) 1999-2009 Audacity Team.
License: GPL v2.  See License.txt.

******************************************************************//**

\class FFmpegLibs
\brief Class used to dynamically load FFmpeg libraries

*//*******************************************************************/

// Store function pointers here when including FFmpeg.h
#define DEFINE_FFMPEG_POINTERS


#include "FFmpeg.h"

#include "FileNames.h"
#include "widgets/HelpSystem.h"
#include "widgets/AudacityMessageBox.h"

#include <wx/checkbox.h>
#include <wx/dynlib.h>
#include <wx/file.h>
#include <wx/log.h>
#include <wx/textctrl.h>

#if !defined(USE_FFMPEG)
/// FFmpeg support may or may not be compiled in,
/// but Preferences dialog requires this function nevertheless
TranslatableString GetFFmpegVersion()
{
   return XO("FFmpeg support not compiled in");
}

#else

/** This pointer to the shared object has global scope and is used to track the
 * singleton object which wraps the FFmpeg codecs */
std::unique_ptr<FFmpegLibs> FFmpegLibsPtr{};
FFmpegLibs *FFmpegLibsInst()
{
   return FFmpegLibsPtr.get();
}

FFmpegLibs *PickFFmpegLibs()
{
   if (FFmpegLibsPtr)
      FFmpegLibsPtr->refcount++;
   else
      FFmpegLibsPtr = std::make_unique<FFmpegLibs>();

   return FFmpegLibsPtr.get();
}

void DropFFmpegLibs()
{
   if (FFmpegLibsPtr)
   {
      FFmpegLibsPtr->refcount--;
      if (FFmpegLibsPtr->refcount == 0)
         FFmpegLibsPtr.reset();
   }
}

bool LoadFFmpeg(bool showerror)
{
   PickFFmpegLibs();
   if (FFmpegLibsInst()->ValidLibsLoaded())
   {
      DropFFmpegLibs();
      return true;
   }
   if (!FFmpegLibsInst()->LoadLibs(NULL, showerror))
   {
      DropFFmpegLibs();
      gPrefs->Write(L"/FFmpeg/Enabled", false);
      gPrefs->Flush();
      return false;
   }
   else
   {
      gPrefs->Write(L"/FFmpeg/Enabled", true);
      gPrefs->Flush();
      return true;
   }
}

/** Called during Audacity start-up to try and load the ffmpeg libraries */
void FFmpegStartup()
{
   bool enabled = false;
   gPrefs->Read(L"/FFmpeg/Enabled",&enabled);
   // 'false' means that no errors should be shown whatsoever
   if (!LoadFFmpeg(false))
   {
      if (enabled)
      {
         AudacityMessageBox(XO(
"FFmpeg was configured in Preferences and successfully loaded before, \
\nbut this time Audacity failed to load it at startup. \
\n\nYou may want to go back to Preferences > Libraries and re-configure it."),
            XO("FFmpeg startup failed"));
      }
   }
}

TranslatableString GetFFmpegVersion()
{
   PickFFmpegLibs();

   auto versionString = XO("FFmpeg library not found");

   if (FFmpegLibsInst()->ValidLibsLoaded()) {
      versionString = Verbatim( FFmpegLibsInst()->GetLibraryVersion() );
   }

   DropFFmpegLibs();

   return versionString;
}

void av_log_wx_callback(void* ptr, int level, const char* fmt, va_list vl)
{
   //Most of this stuff is taken from FFmpeg tutorials and FFmpeg itself
   int av_log_level = AV_LOG_INFO;
   AVClass* avc = ptr ? *(AVClass**)ptr : NULL;
   if (level > av_log_level)
      return;
   wxString printstring(L"");

   if (avc) {
      printstring.Append(wxString::Format(L"[%s @ %p] ", wxString::FromUTF8(avc->item_name(ptr)), avc));
   }

   wxString frm(fmt,wxConvLibc);

   printstring.Append(wxString::FormatV(frm,vl));
   wxString cpt;
   switch (level)
   {
   case 0: cpt = L"Error"; break;
   case 1: cpt = L"Info"; break;
   case 2: cpt = L"Debug"; break;
   default: cpt = L"Log"; break;
   }
   wxLogDebug(L"%s: %s",cpt,printstring);
}

//======================= Unicode aware uri protocol for FFmpeg
// Code inspired from ffmpeg-users mailing list sample

static int ufile_read(void *opaque, uint8_t *buf, int size)
{
   int ret = (int)((wxFile *) opaque)->Read(buf, size);
   return ret;
}

static int ufile_write(void *opaque, uint8_t *buf, int size)
{
   auto bytes = (int) ((wxFile *) opaque)->Write(buf, size);
   if (bytes != size)
      return -ENOSPC;
   return bytes;
}

static int64_t ufile_seek(void *opaque, int64_t pos, int whence)
{
   wxSeekMode mode = wxFromStart;

#if !defined(AVSEEK_FORCE)
#define AVSEEK_FORCE 0
#endif

   switch (whence & ~AVSEEK_FORCE)
   {
   case (SEEK_SET):
      mode = wxFromStart;
      break;
   case (SEEK_CUR):
      mode = wxFromCurrent;
      break;
   case (SEEK_END):
      mode = wxFromEnd;
      break;
   case (AVSEEK_SIZE):
      return ((wxFile *) opaque)->Length();
   }

   return ((wxFile *) opaque)->Seek(pos, mode);
}

int ufile_close(AVIOContext *pb)
{
   std::unique_ptr<wxFile> f{ (wxFile *)pb->opaque };

   bool success = true;
   if (f) {
      if (pb->write_flag) {
         success = f->Flush();
      }
      if (success) {
         success = f->Close();
      }
      pb->opaque = nullptr;
   }

   // We're not certain that a close error is for want of space, but we'll
   // guess that
   return success ? 0 : -ENOSPC;

   // Implicitly destroy the wxFile object here
}

// Open a file with a (possibly) Unicode filename
int ufile_fopen(AVIOContext **s, const FilePath & name, int flags)
{
   wxFile::OpenMode mode;

   auto f = std::make_unique<wxFile>();
   if (!f) {
      return -ENOMEM;
   }

   if (flags == (AVIO_FLAG_READ | AVIO_FLAG_WRITE)) {
      return -EINVAL;
   } else if (flags == AVIO_FLAG_WRITE) {
      mode = wxFile::write;
   } else {
      mode = wxFile::read;
   }

   if (!f->Open(name, mode)) {
      return -ENOENT;
   }

   *s = avio_alloc_context((unsigned char*)av_malloc(32768), 32768,
                           flags & AVIO_FLAG_WRITE,
                           /*opaque*/f.get(),
                           ufile_read,
                           ufile_write,
                           ufile_seek);
   if (!*s) {
      return -ENOMEM;
   }

   f.release(); // s owns the file object now

   return 0;
}


// Detect type of input file and open it if recognized. Routine
// based on the av_open_input_file() libavformat function.
int ufile_fopen_input(std::unique_ptr<FFmpegContext> &context_ptr, FilePath & name)
{
   context_ptr.reset();
   auto context = std::make_unique<FFmpegContext>();

   wxFileName ff{ name };
   wxCharBuffer fname;
   const char *filename;
   int err;

   fname = ff.GetFullName().mb_str();
   filename = (const char *) fname;

   // Open the file to prepare for probing
   if ((err = ufile_fopen(&context->pb, name, AVIO_FLAG_READ)) < 0) {
      goto fail;
   }

   context->ic_ptr = avformat_alloc_context();
   context->ic_ptr->pb = context->pb;

   // And finally, attempt to associate an input stream with the file
   err = avformat_open_input(&context->ic_ptr, filename, NULL, NULL);
   if (err) {
      goto fail;
   }

   // success
   context_ptr = std::move(context);
   return 0;

fail:

   return err;
}

FFmpegContext::~FFmpegContext()
{
   if (FFmpegLibsInst()->ValidLibsLoaded())
   {
      if (ic_ptr)
         avformat_close_input(&ic_ptr);
      av_log_set_callback(av_log_default_callback);
   }

   if (pb) {
      ufile_close(pb);
      if (FFmpegLibsInst()->ValidLibsLoaded())
      {
         av_free(pb->buffer);
         av_free(pb);
      }
   }
}

streamContext *import_ffmpeg_read_next_frame(AVFormatContext* formatContext,
                                             streamContext** streams,
                                             unsigned int numStreams)
{
   streamContext *sc = NULL;
   AVPacketEx pkt;

   if (av_read_frame(formatContext, &pkt) < 0)
   {
      return NULL;
   }

   // Find a stream to which this frame belongs
   for (unsigned int i = 0; i < numStreams; i++)
   {
      if (streams[i]->m_stream->index == pkt.stream_index)
         sc = streams[i];
   }

   // Off-stream packet. Don't panic, just skip it.
   // When not all streams are selected for import this will happen very often.
   if (sc == NULL)
   {
      return (streamContext*)1;
   }

   // Copy the frame to the stream context
   sc->m_pkt.emplace(std::move(pkt));

   sc->m_pktDataPtr = sc->m_pkt->data;
   sc->m_pktRemainingSiz = sc->m_pkt->size;

   return sc;
}

int import_ffmpeg_decode_frame(streamContext *sc, bool flushing)
{
   int      nBytesDecoded;
   wxUint8 *pDecode = sc->m_pktDataPtr;
   int      nDecodeSiz = sc->m_pktRemainingSiz;

   sc->m_frameValid = 0;

   if (flushing)
   {
      // If we're flushing the decoders we don't actually have any NEW data to decode.
      pDecode = NULL;
      nDecodeSiz = 0;
   }
   else
   {
      if (!sc->m_pkt || (sc->m_pktRemainingSiz <= 0))
      {
         //No more data
         return -1;
      }
   }

   AVPacketEx avpkt;
   avpkt.data = pDecode;
   avpkt.size = nDecodeSiz;

   AVFrameHolder frame{ av_frame_alloc() };
   int got_output = 0;

   nBytesDecoded =
      avcodec_decode_audio4(sc->m_codecCtx,
                            frame.get(),                                   // out
                            &got_output,                             // out
                            &avpkt);                                 // in

   if (nBytesDecoded < 0)
   {
      // Decoding failed. Don't stop.
      return -1;
   }

   sc->m_samplefmt = sc->m_codecCtx->sample_fmt;
   sc->m_samplesize = static_cast<size_t>(av_get_bytes_per_sample(sc->m_samplefmt));

   int channels = sc->m_codecCtx->channels;
   auto newsize = sc->m_samplesize * frame->nb_samples * channels;
   sc->m_decodedAudioSamplesValidSiz = newsize;
   // Reallocate the audio sample buffer if it's smaller than the frame size.
   if (newsize > sc->m_decodedAudioSamplesSiz )
   {
      // Reallocate a bigger buffer.  But av_realloc is NOT compatible with the returns of av_malloc!
      // So do this:
      sc->m_decodedAudioSamples.reset(static_cast<uint8_t *>(av_malloc(newsize)));
      sc->m_decodedAudioSamplesSiz = newsize;
      if (!sc->m_decodedAudioSamples)
      {
         //Can't allocate bytes
         return -1;
      }
   }
   if (frame->data[1]) {
      for (int i = 0; i<frame->nb_samples; i++) {
         for (int ch = 0; ch<channels; ch++) {
            memcpy(sc->m_decodedAudioSamples.get() + sc->m_samplesize * (ch + channels*i),
                  frame->extended_data[ch] + sc->m_samplesize*i,
                  sc->m_samplesize);
         }
      }
   } else {
      memcpy(sc->m_decodedAudioSamples.get(), frame->data[0], newsize);
   }

   // We may not have read all of the data from this packet. If so, the user can call again.
   // Whether or not they do depends on if m_pktRemainingSiz == 0 (they can check).
   sc->m_pktDataPtr += nBytesDecoded;
   sc->m_pktRemainingSiz -= nBytesDecoded;

   // At this point it's normally safe to assume that we've read some samples. However, the MPEG
   // audio decoder is broken. If this is the case then we just return with m_frameValid == 0
   // but m_pktRemainingSiz perhaps != 0, so the user can call again.
   if (sc->m_decodedAudioSamplesValidSiz > 0)
   {
      sc->m_frameValid = 1;
   }
   return 0;
}



/*******************************************************/

class FFmpegNotFoundDialog;

//----------------------------------------------------------------------------
// FindFFmpegDialog
//----------------------------------------------------------------------------

/// Allows user to locate libav* libraries
class FindFFmpegDialog final : public wxDialogWrapper
{
public:

   FindFFmpegDialog(wxWindow *parent, const wxString &path, const wxString &name,
      FileNames::FileTypes types)
      :  wxDialogWrapper(parent, wxID_ANY, XO("Locate FFmpeg"))
   {
      SetName();
      ShuttleGui S(this);

      mPath = path;
      mName = name;
      mTypes = std::move( types );

      mLibPath.Assign(mPath, mName);

      PopulateOrExchange(S);
   }

   void PopulateOrExchange(ShuttleGui & S)
   {
      S.StartVerticalLay(true, 10);
      {
         S
            .AddTitle(
               XO(
"Audacity needs the file '%s' to import and export audio via FFmpeg.")
                  .Format( mName ) );

         S.StartHorizontalLay(wxALIGN_LEFT, true, 3);
         {
            S
               .AddTitle( XO("Location of '%s':").Format( mName ) );
         }
         S.EndHorizontalLay();

         S.StartMultiColumn(2,
                            GroupOptions{ wxEXPAND }
                               .Border(3)
                               .StretchyColumn(0));
         {
            //
            //
            if (mLibPath.GetFullPath().empty()) {
               S
                  .AddTextBox( {},
                     wxString::Format(_("To find '%s', click here -->"), mName), 0)
                  .Assign(mPathText);
            }
            else {
               mPathText =
               S
                  .AddTextBox( {}, mLibPath.GetFullPath(), 0)
                  .Assign(mPathText);
            }

            S
               .Action( [this]{ OnBrowse(); } )
               .AddButton(XXO("Browse..."), wxALIGN_RIGHT);
   
            S
               .AddVariableText(
                  XO("To get a free copy of FFmpeg, click here -->"), true);

            S
               .Action( [this]{ OnDownload(); } )
               .AddButton(XXO("Download"), wxALIGN_RIGHT);
         }
         S.EndMultiColumn();

         S
            .AddStandardButtons();
      }
      S.EndVerticalLay();

      Layout();
      Fit();
      SetMinSize(GetSize());
      Center();

      return;
   }

   void OnBrowse()
   {
      /* i18n-hint: It's asking for the location of a file, for
      example, "Where is lame_enc.dll?" - you could translate
      "Where would I find the file '%s'?" instead if you want. */
      auto question = XO("Where is '%s'?").Format( mName );

      wxString path = FileNames::SelectFile(FileNames::Operation::_None,
         question,
         mLibPath.GetPath(),
         mLibPath.GetFullName(),
         L"",
         mTypes,
         wxFD_OPEN | wxRESIZE_BORDER,
         this);
      if (!path.empty()) {
         mLibPath = path;
         mPathText->SetValue(path);
      }
   }

   void OnDownload()
   {
      HelpSystem::ShowHelp(this, L"FAQ:Installing_the_FFmpeg_Import_Export_Library");
   }

   wxString GetLibPath()
   {
      return mLibPath.GetFullPath();
   }

private:

   wxFileName mLibPath;

   wxString mPath;
   wxString mName;
   FileNames::FileTypes mTypes;

   wxTextCtrl *mPathText;
};


//----------------------------------------------------------------------------
// FFmpegNotFoundDialog
//----------------------------------------------------------------------------

FFmpegNotFoundDialog::FFmpegNotFoundDialog(wxWindow *parent)
   :  wxDialogWrapper(parent, wxID_ANY, XO("FFmpeg not found"))
{
   SetName();
   ShuttleGui S(this);
   PopulateOrExchange(S);
}

void FFmpegNotFoundDialog::PopulateOrExchange(ShuttleGui & S)
{
   wxString text;

   S.StartVerticalLay(true, 10);
   {
      S
         .AddFixedText( XO(
"Audacity attempted to use FFmpeg to import an audio file,\n\
but the libraries were not found.\n\n\
To use FFmpeg import, go to Edit > Preferences > Libraries\n\
to download or locate the FFmpeg libraries."
         ) );

      S
         .AddCheckBox(XXO("Do not show this warning again"),
            gPrefs->ReadBool(L"/FFmpeg/NotFoundDontShow", false) )
         .Assign( mDontShow );

      S
         .AddStandardButtons( 0, {
            S.Item( eOkButton ).Action( [this]{ OnOk(); } )
         });
   }
   S.EndVerticalLay();

   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();

   return;
}

void FFmpegNotFoundDialog::OnOk()
{
   if (mDontShow->GetValue())
   {
      gPrefs->Write(L"/FFmpeg/NotFoundDontShow",1);
      gPrefs->Flush();
   }
   this->EndModal(0);
}


//----------------------------------------------------------------------------
// FFmpegLibs
//----------------------------------------------------------------------------

FFmpegLibs::FFmpegLibs()
{
   mLibsLoaded = false;
   refcount = 1;
   if (gPrefs) {
      mLibAVFormatPath = gPrefs->Read(L"/FFmpeg/FFmpegLibPath", L"");
   }

}

FFmpegLibs::~FFmpegLibs()
{
   FreeLibs();
};

bool FFmpegLibs::FindLibs(wxWindow *parent)
{
   wxString path;
   wxString name;

   // If we're looking for the lib, use the standard name, as the
   // configured name is not found.
   name = GetLibAVFormatName();
   wxLogMessage(L"Looking for FFmpeg libraries...");
   if (!mLibAVFormatPath.empty()) {
      wxLogMessage(L"mLibAVFormatPath ('%s') is not empty.", mLibAVFormatPath);
      const wxFileName fn{ mLibAVFormatPath };
      path = fn.GetPath();
   }
   else {
      path = GetLibAVFormatPath();
      wxLogMessage(L"mLibAVFormatPath is empty, starting with path '%s', name '%s'.",
                  path, name);
   }

   FindFFmpegDialog fd(parent,
                        path,
                        name,
                        GetLibraryTypes());

   if (fd.ShowModal() == wxID_CANCEL) {
      wxLogMessage(L"User canceled the dialog. Failed to find FFmpeg libraries.");
      return false;
   }

   path = fd.GetLibPath();

   wxLogMessage(L"User-specified path = '%s'", path);
   if (!::wxFileExists(path)) {
      wxLogError(L"User-specified file does not exist. Failed to find FFmpeg libraries.");
      return false;
   }
   wxLogMessage(L"User-specified FFmpeg file exists. Success.");
   mLibAVFormatPath = path;
   gPrefs->Write(L"/FFmpeg/FFmpegLibPath", mLibAVFormatPath);
   gPrefs->Flush();

   return true;
}

bool FFmpegLibs::LoadLibs(wxWindow * WXUNUSED(parent), bool showerr)
{
#if defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
   mLibsLoaded = InitLibs(wxEmptyString, showerr);
   return mLibsLoaded;
#endif

   wxLogMessage(L"Trying to load FFmpeg libraries...");
   if (ValidLibsLoaded()) {
      wxLogMessage(L"FFmpeg libraries are already loaded.");
      FreeLibs();
   }

   // First try loading it from a previously located path
   if (!mLibAVFormatPath.empty()) {
      wxLogMessage(L"mLibAVFormatPath ('%s') is not empty. Loading from it.",mLibAVFormatPath);
      mLibsLoaded = InitLibs(mLibAVFormatPath,showerr);
   }

   // If not successful, try loading it from default path
   if (!mLibsLoaded && !GetLibAVFormatPath().empty()) {
      const wxFileName fn{ GetLibAVFormatPath(), GetLibAVFormatName() };
      wxString path = fn.GetFullPath();
      wxLogMessage(L"Trying to load FFmpeg libraries from default path, '%s'.", path);
      mLibsLoaded = InitLibs(path,showerr);
      if (mLibsLoaded) {
         mLibAVFormatPath = path;
      }
   }

#if defined(__WXMAC__)
   // If not successful, try loading it from legacy path
   if (!mLibsLoaded && !GetLibAVFormatPath().empty()) {
      const wxFileName fn{L"/usr/local/lib/audacity", GetLibAVFormatName()};
      wxString path = fn.GetFullPath();
      wxLogMessage(L"Trying to load FFmpeg libraries from legacy path, '%s'.", path);
      mLibsLoaded = InitLibs(path,showerr);
      if (mLibsLoaded) {
         mLibAVFormatPath = path;
      }
   }
#endif

   // If not successful, try loading using system search paths
   if (!ValidLibsLoaded()) {
      wxString path = GetLibAVFormatName();
      wxLogMessage(L"Trying to load FFmpeg libraries from system paths. File name is '%s'.", path);
      mLibsLoaded = InitLibs(path,showerr);
      if (mLibsLoaded) {
         mLibAVFormatPath = path;
      }
   }

   // If libraries aren't loaded - nag user about that
   /*
   if (!ValidLibsLoaded())
   {
      wxLogError(L"Failed to load libraries altogether.");
      int dontShowDlg;
      gPrefs->Read(L"/FFmpeg/NotFoundDontShow",&dontShowDlg,0);
      if ((dontShowDlg == 0) && (showerr))
         FFmpegNotFoundDialog{nullptr}.ShowModal();
   }
   */
   // Oh well, just give up
   if (!ValidLibsLoaded()) {
      auto msg = XO("Failed to find compatible FFmpeg libraries.");
      if (showerr)
         AudacityMessageBox( msg );
      wxLogError(msg.Debug());
      return false;
   }

   wxLogMessage(L"FFmpeg libraries loaded successfully.");
   return true;
}

bool FFmpegLibs::ValidLibsLoaded()
{
   return mLibsLoaded;
}

bool FFmpegLibs::InitLibs(const wxString &libpath_format, bool WXUNUSED(showerr))
{
#if !defined(DISABLE_DYNAMIC_LOADING_FFMPEG)
   FreeLibs();

#if defined(__WXMSW__)
   wxString oldpath;
   wxString syspath;
   bool pathExisted = false;

   // Return PATH to normal
   auto restorePath = finally([&]()
   {
      if (oldpath != syspath)
      {
         if (pathExisted)
         {
            wxLogMessage(L"Returning PATH to previous setting: %s", oldpath);
            wxSetEnv(L"PATH", oldpath);
         }
         else
         {
            wxLogMessage(L"Removing PATH environment variable");
            wxUnsetEnv(L"PATH");
         }
      }
   });

   wxLogMessage(L"Looking up PATH environment variable...");
   // First take PATH environment variable and store its content.
   pathExisted = wxGetEnv(L"PATH",&syspath);
   oldpath = syspath;

   wxLogMessage(L"PATH = '%s'", syspath);

   const wxString &fmtdir{ wxPathOnly(libpath_format) };
   wxString fmtdirsc = fmtdir + L";";
   wxString scfmtdir = L";" + fmtdir;

   if (syspath.Left(1) == wxT(';'))
   {
      wxLogMessage(L"Temporarily prepending '%s' to PATH...", fmtdir);
      syspath.Prepend(scfmtdir);
   }
   else
   {
      wxLogMessage(L"Temporarily prepending '%s' to PATH...", scfmtdir);
      syspath.Prepend(fmtdirsc);
   }

   if (!wxSetEnv(L"PATH",syspath))
   {
      wxLogSysError(L"Setting PATH via wxSetEnv('%s') failed.", syspath);
   }
#endif

   //Load libavformat
   // Initially we don't know where are the avcodec and avutl libs
   wxDynamicLibrary *codec = NULL;
   wxDynamicLibrary *util = NULL;
   wxFileName avcodec_filename;
   wxFileName avutil_filename;
   wxFileName name{ libpath_format };
   wxString nameFull{name.GetFullPath()};
   bool gotError = false;

   // Check for a monolithic avformat
   avformat = std::make_unique<wxDynamicLibrary>();
   wxLogMessage(L"Checking for monolithic avformat from '%s'.", nameFull);
   gotError = !avformat->Load(nameFull, wxDL_LAZY);

   // Verify it really is monolithic
   if (!gotError) {
      avutil_filename = FileNames::PathFromAddr(avformat->GetSymbol(L"avutil_version"));
      avcodec_filename = FileNames::PathFromAddr(avformat->GetSymbol(L"avcodec_version"));
      if (avutil_filename.GetFullPath() == nameFull) {
         if (avcodec_filename.GetFullPath() == nameFull) {
            util = avformat.get();
            codec = avformat.get();
         }
      }
      if (!avcodec_filename.FileExists()) {
         avcodec_filename = GetLibAVCodecName();
      }
      if (!avutil_filename.FileExists()) {
         avutil_filename = GetLibAVUtilName();
      }

      if (util == NULL || codec == NULL) {
         wxLogMessage(L"avformat not monolithic");
         avformat->Unload();
         util = NULL;
         codec = NULL;
      }
      else {
         wxLogMessage(L"avformat is monolithic");
      }
   }

   // The two wxFileNames don't change after this
   const wxString avcodec_filename_full{ avcodec_filename.GetFullPath() };
   const wxString avutil_filename_full{ avutil_filename.GetFullPath() };

   if (!util) {
      util = (avutil = std::make_unique<wxDynamicLibrary>()).get();
      wxLogMessage(L"Loading avutil from '%s'.", avutil_filename_full);
      util->Load(avutil_filename_full, wxDL_LAZY);
   }

   if (!codec) {
      codec = (avcodec = std::make_unique<wxDynamicLibrary>()).get();
      wxLogMessage(L"Loading avcodec from '%s'.", avcodec_filename_full);
      codec->Load(avcodec_filename_full, wxDL_LAZY);
   }

   if (!avformat->IsLoaded()) {
      name.SetFullName(libpath_format);
      nameFull = name.GetFullPath();
      wxLogMessage(L"Loading avformat from '%s'.", nameFull);
      gotError = !avformat->Load(nameFull, wxDL_LAZY);
   }

   if (gotError) {
      wxLogError(L"Failed to load FFmpeg libraries.");
      FreeLibs();
      return false;
   }

   // Show the actual libraries loaded
   if (avutil) {
      wxLogMessage(L"Actual avutil path %s",
                 FileNames::PathFromAddr(avutil->GetSymbol(L"avutil_version")));
   }
   if (avcodec) {
      wxLogMessage(L"Actual avcodec path %s",
                 FileNames::PathFromAddr(avcodec->GetSymbol(L"avcodec_version")));
   }
   if (avformat) {
      wxLogMessage(L"Actual avformat path %s",
                 FileNames::PathFromAddr(avformat->GetSymbol(L"avformat_version")));
   }

   wxLogMessage(L"Importing symbols...");
   FFMPEG_INITDYN(avformat, av_register_all);
   FFMPEG_INITDYN(avformat, avformat_find_stream_info);
   FFMPEG_INITDYN(avformat, av_read_frame);
   FFMPEG_INITDYN(avformat, av_seek_frame);
   FFMPEG_INITDYN(avformat, avformat_close_input);
   FFMPEG_INITDYN(avformat, avformat_write_header);
   FFMPEG_INITDYN(avformat, av_interleaved_write_frame);
   FFMPEG_INITDYN(avformat, av_oformat_next);
   FFMPEG_INITDYN(avformat, avformat_new_stream);
   FFMPEG_INITDYN(avformat, avformat_alloc_context);
   FFMPEG_INITDYN(avformat, av_write_trailer);
   FFMPEG_INITDYN(avformat, av_codec_get_tag);
   FFMPEG_INITDYN(avformat, avformat_version);
   FFMPEG_INITDYN(avformat, avformat_open_input);
   FFMPEG_INITDYN(avformat, avio_size);
   FFMPEG_INITDYN(avformat, avio_alloc_context);
   FFMPEG_INITALT(avformat, av_guess_format, avformat, guess_format);
   FFMPEG_INITDYN(avformat, avformat_free_context);

   FFMPEG_INITDYN(avcodec, av_init_packet);
   FFMPEG_INITDYN(avcodec, av_free_packet);
   FFMPEG_INITDYN(avcodec, avcodec_find_encoder);
   FFMPEG_INITDYN(avcodec, avcodec_find_encoder_by_name);
   FFMPEG_INITDYN(avcodec, avcodec_find_decoder);
   FFMPEG_INITDYN(avcodec, avcodec_get_name);
   FFMPEG_INITDYN(avcodec, avcodec_open2);
   FFMPEG_INITDYN(avcodec, avcodec_decode_audio4);
   FFMPEG_INITDYN(avcodec, avcodec_encode_audio2);
   FFMPEG_INITDYN(avcodec, avcodec_close);
   FFMPEG_INITDYN(avcodec, avcodec_register_all);
   FFMPEG_INITDYN(avcodec, avcodec_version);
   FFMPEG_INITDYN(avcodec, av_codec_next);
   FFMPEG_INITDYN(avcodec, av_codec_is_encoder);
   FFMPEG_INITDYN(avcodec, avcodec_fill_audio_frame);

   FFMPEG_INITDYN(avutil, av_free);
   FFMPEG_INITDYN(avutil, av_dict_free);
   FFMPEG_INITDYN(avutil, av_dict_get);
   FFMPEG_INITDYN(avutil, av_dict_set);
   FFMPEG_INITDYN(avutil, av_get_bytes_per_sample);
   FFMPEG_INITDYN(avutil, av_log_set_callback);
   FFMPEG_INITDYN(avutil, av_log_default_callback);
   FFMPEG_INITDYN(avutil, av_fifo_alloc);
   FFMPEG_INITDYN(avutil, av_fifo_generic_read);
   FFMPEG_INITDYN(avutil, av_fifo_realloc2);
   FFMPEG_INITDYN(avutil, av_fifo_free);
   FFMPEG_INITDYN(avutil, av_fifo_size);
   FFMPEG_INITDYN(avutil, av_malloc);
   FFMPEG_INITDYN(avutil, av_fifo_generic_write);
   // FFMPEG_INITDYN(avutil, av_freep);
   FFMPEG_INITDYN(avutil, av_rescale_q);
   FFMPEG_INITDYN(avutil, avutil_version);
   FFMPEG_INITALT(avutil, av_frame_alloc, avcodec, avcodec_alloc_frame);
   FFMPEG_INITALT(avutil, av_frame_free, avcodec, avcodec_free_frame);
   FFMPEG_INITDYN(avutil, av_samples_get_buffer_size);
   FFMPEG_INITDYN(avutil, av_get_default_channel_layout);
   FFMPEG_INITDYN(avutil, av_strerror);

   wxLogMessage(L"All symbols loaded successfully. Initializing the library.");
#endif

   //FFmpeg initialization
   avcodec_register_all();
   av_register_all();

   wxLogMessage(L"Retrieving FFmpeg library version numbers:");
   int avfver = avformat_version();
   int avcver = avcodec_version();
   int avuver = avutil_version();
   mAVCodecVersion = wxString::Format(L"%d.%d.%d",avcver >> 16 & 0xFF, avcver >> 8 & 0xFF, avcver & 0xFF);
   mAVFormatVersion = wxString::Format(L"%d.%d.%d",avfver >> 16 & 0xFF, avfver >> 8 & 0xFF, avfver & 0xFF);
   mAVUtilVersion = wxString::Format(L"%d.%d.%d",avuver >> 16 & 0xFF, avuver >> 8 & 0xFF, avuver & 0xFF);

   wxLogMessage(L"   AVCodec version 0x%06x - %s (built against 0x%06x - %s)",
                  avcver, mAVCodecVersion, LIBAVCODEC_VERSION_INT,
                  wxString::FromUTF8(AV_STRINGIFY(LIBAVCODEC_VERSION)));
   wxLogMessage(L"   AVFormat version 0x%06x - %s (built against 0x%06x - %s)",
                  avfver, mAVFormatVersion, LIBAVFORMAT_VERSION_INT,
                  wxString::FromUTF8(AV_STRINGIFY(LIBAVFORMAT_VERSION)));
   wxLogMessage(L"   AVUtil version 0x%06x - %s (built against 0x%06x - %s)",
                  avuver,mAVUtilVersion, LIBAVUTIL_VERSION_INT,
                  wxString::FromUTF8(AV_STRINGIFY(LIBAVUTIL_VERSION)));

   int avcverdiff = (avcver >> 16 & 0xFF) - (int)(LIBAVCODEC_VERSION_MAJOR);
   int avfverdiff = (avfver >> 16 & 0xFF) - (int)(LIBAVFORMAT_VERSION_MAJOR);
   int avuverdiff = (avuver >> 16 & 0xFF) - (int)(LIBAVUTIL_VERSION_MAJOR);
   if (avcverdiff != 0)
      wxLogError(L"AVCodec version mismatch = %d", avcverdiff);
   if (avfverdiff != 0)
      wxLogError(L"AVFormat version mismatch = %d", avfverdiff);
   if (avuverdiff != 0)
      wxLogError(L"AVUtil version mismatch = %d", avuverdiff);
   //make sure that header and library major versions are the same
   if (avcverdiff != 0 || avfverdiff != 0 || avuverdiff != 0)
   {
      wxLogError(L"Version mismatch. FFmpeg libraries are unusable.");
      return false;
   }

   return true;
}

void FFmpegLibs::FreeLibs()
{
   avformat.reset();
   avcodec.reset();
   avutil.reset();
   mLibsLoaded = false;
   return;
}

#endif //USE_FFMPEG
