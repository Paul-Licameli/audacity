/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadNyquist.cpp

  Dominic Mazzoni

**********************************************************************/



#include "LoadNyquist.h"

#include <wx/log.h>

#include "Nyquist.h"

#include "../../FileNames.h"
#include "../../PluginManager.h"
#include "../../ModuleManager.h"
#include "../../wxFileNameWrapper.h"

// ============================================================================
// List of effects that ship with Audacity.  These will be autoregistered.
// ============================================================================
const static wxChar *kShippedEffects[] =
{
   L"adjustable-fade.ny",
   L"beat.ny",
   L"clipfix.ny",
   L"crossfadeclips.ny",
   L"crossfadetracks.ny",
   L"delay.ny",
   L"equalabel.ny",
   L"highpass.ny",
   L"label-sounds.ny",
   L"limiter.ny",
   L"lowpass.ny",
   L"noisegate.ny",
   L"notch.ny",
   L"nyquist-plug-in-installer.ny",
   L"pluck.ny",
   L"rhythmtrack.ny",
   L"rissetdrum.ny",
   L"sample-data-export.ny",
   L"sample-data-import.ny",
   L"spectral-delete.ny",
   L"SpectralEditMulti.ny",
   L"SpectralEditParametricEQ.ny",
   L"SpectralEditShelves.ny",
   L"StudioFadeOut.ny",
   L"tremolo.ny",
   L"vocalrediso.ny",
   L"vocoder.ny",
};

// ============================================================================
// Module registration entry point
//
// This is the symbol that Audacity looks for when the module is built as a
// dynamic library.
//
// When the module is builtin to Audacity, we use the same function, but it is
// declared static so as not to clash with other builtin modules.
// ============================================================================
DECLARE_MODULE_ENTRY(AudacityModule)
{
   // Create and register the importer
   // Trust the module manager not to leak this
   return safenew NyquistEffectsModule();
}

// ============================================================================
// Register this as a builtin module
// ============================================================================
DECLARE_BUILTIN_MODULE(NyquistsEffectBuiltin);

///////////////////////////////////////////////////////////////////////////////
//
// NyquistEffectsModule
//
///////////////////////////////////////////////////////////////////////////////

NyquistEffectsModule::NyquistEffectsModule()
{
}

NyquistEffectsModule::~NyquistEffectsModule()
{
}

// ============================================================================
// ComponentInterface implementation
// ============================================================================

PluginPath NyquistEffectsModule::GetPath()
{
   return {};
}

ComponentInterfaceSymbol NyquistEffectsModule::GetSymbol()
{
   return XO("Nyquist Effects");
}

VendorSymbol NyquistEffectsModule::GetVendor()
{
   return XO("The Audacity Team");
}

wxString NyquistEffectsModule::GetVersion()
{
   // This "may" be different if this were to be maintained as a separate DLL
   return NYQUISTEFFECTS_VERSION;
}

TranslatableString NyquistEffectsModule::GetDescription()
{
   return XO("Provides Nyquist Effects support to Audacity");
}

// ============================================================================
// ModuleInterface implementation
// ============================================================================

bool NyquistEffectsModule::Initialize()
{
   const auto &audacityPathList = FileNames::AudacityPathList();

   for (const auto &path : audacityPathList)
   {
      wxFileNameWrapper name{ path, L"" };
      name.AppendDir(L"nyquist");
      name.SetFullName(L"nyquist.lsp");
      if (name.FileExists())
      {
         // set_xlisp_path doesn't handle fn_Str() in Unicode build. May or may not actually work.
         nyx_set_xlisp_path(name.GetPath().ToUTF8());
         return true;
      }
   }

   wxLogWarning(L"Critical Nyquist files could not be found. Nyquist effects will not work.");

   return false;
}

void NyquistEffectsModule::Terminate()
{
   nyx_set_xlisp_path(NULL);

   return;
}

EffectFamilySymbol NyquistEffectsModule::GetOptionalFamilySymbol()
{
#if USE_NYQUIST
   return NYQUISTEFFECTS_FAMILY;
#else
   return {};
#endif
}

const FileExtensions &NyquistEffectsModule::GetFileExtensions()
{
   static FileExtensions result{{ _T("ny") }};
   return result;
}

FilePath NyquistEffectsModule::InstallPath()
{
   return FileNames::PlugInDir();
}

bool NyquistEffectsModule::AutoRegisterPlugins(PluginManagerInterface & pm)
{
   // Autoregister effects that we "think" are ones that have been shipped with
   // Audacity.  A little simplistic, but it should suffice for now.
   auto pathList = NyquistEffect::GetNyquistSearchPath();
   FilePaths files;
   TranslatableString ignoredErrMsg;

   auto name = NYQUIST_PROMPT_NAME;
   if (!pm.IsPluginRegistered(NYQUIST_PROMPT_ID, &name))
   {
      // No checking of error ?
      DiscoverPluginsAtPath(NYQUIST_PROMPT_ID, ignoredErrMsg,
         PluginManagerInterface::DefaultRegistrationCallback);
   }

   for (size_t i = 0; i < WXSIZEOF(kShippedEffects); i++)
   {
      files.clear();
      pm.FindFilesInPathList(kShippedEffects[i], pathList, files);
      for (size_t j = 0, cnt = files.size(); j < cnt; j++)
      {
         /*
           TODO: Currently the names of Nyquist plug-ins cannot have
          context specific translations or internal names different from
          the visible English names.
   
          This makes it unnecessary to pass a second argument to
          IsPluginRegistered for correction of the registry (as is needed
          in the case of built-in effects).

          If it does become necessary in the future, we will need to open the
          .ny files to access their $name lines so that this argument could
          be supplied.
          */
         if (!pm.IsPluginRegistered(files[j].GET()))
         {
            // No checking of error ?
            DiscoverPluginsAtPath(files[j].GET(), ignoredErrMsg,
               PluginManagerInterface::DefaultRegistrationCallback);
         }
      }
   }

   // We still want to be called during the normal registration process
   return false;
}

PluginPaths NyquistEffectsModule::FindPluginPaths(PluginManagerInterface & pm)
{
   auto pathList = NyquistEffect::GetNyquistSearchPath();
   FilePaths files;

   // Add the Nyquist prompt
   files.push_back(NYQUIST_PROMPT_ID);

   // Load .ny plug-ins
   pm.FindFilesInPathList(L"*.ny", pathList, files);
   // LLL:  Works for all platform with NEW plugin support (dups are removed)
   pm.FindFilesInPathList(L"*.NY", pathList, files); // Ed's fix for bug 179

   return transform_container<PluginPaths>(
      files, std::mem_fn( &FilePath::GET ) );
}

unsigned NyquistEffectsModule::DiscoverPluginsAtPath(
   const PluginPath & path, TranslatableString &errMsg,
   const RegistrationCallback &callback)
{
   errMsg = {};
   NyquistEffect effect(path);
   if (effect.IsOk())
   {
      if (callback)
         callback(this, &effect);
      return 1;
   }

   errMsg = effect.InitializationError();
   return 0;
}

bool NyquistEffectsModule::IsPluginValid(const PluginPath & path, bool bFast)
{
   // Ignores bFast parameter, since checking file exists is fast enough for
   // the small number of Nyquist plug-ins that we have.
   static_cast<void>(bFast);
   if(path == NYQUIST_PROMPT_ID)
      return true;

   return wxFileName::FileExists(path);
}

ComponentInterface *NyquistEffectsModule::CreateInstance(const PluginPath & path)
{
   // Acquires a resource for the application.
   auto effect = std::make_unique<NyquistEffect>(path);
   if (effect->IsOk())
   {
      // Safety of this depends on complementary calls to DeleteInstance on the module manager side.
      return effect.release();
   }

   return NULL;
}

void NyquistEffectsModule::DeleteInstance(ComponentInterface *instance)
{
   std::unique_ptr < NyquistEffect > {
      dynamic_cast<NyquistEffect *>(instance)
   };
}

// ============================================================================
// NyquistEffectsModule implementation
// ============================================================================
