/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadVamp.h

  Chris Cannam

**********************************************************************/



#if defined(USE_VAMP)

#include <memory>

#include "audacity/ModuleInterface.h"
#include "audacity/EffectInterface.h"
#include "audacity/PluginInterface.h"

#include <vamp-hostsdk/PluginLoader.h>

///////////////////////////////////////////////////////////////////////////////
//
// VampEffectsModule
//
///////////////////////////////////////////////////////////////////////////////

class VampEffectsModule final : public ModuleInterface
{
public:
   VampEffectsModule();
   virtual ~VampEffectsModule();

   // ComponentInterface implementation

   PluginPath GetPath() override;
   ComponentInterfaceSymbol GetSymbol() override;
   VendorSymbol GetVendor() override;
   wxString GetVersion() override;
   TranslatableString GetDescription() override;

   // ModuleInterface implementation

   bool Initialize() override;
   void Terminate() override;
   EffectFamilySymbol GetOptionalFamilySymbol() override;

   const FileExtensions &GetFileExtensions() override;
   FilePath InstallPath() override { return {}; }

   bool AutoRegisterPlugins(PluginManagerInterface & pm) override;
   PluginPaths FindPluginPaths(PluginManagerInterface & pm) override;
   unsigned DiscoverPluginsAtPath(
      const PluginPath & path, TranslatableString &errMsg,
      PluginManagerInterface *pPluginManager )
         override;

   bool IsPluginValid(const PluginPath & path, bool bFast) override;

   std::shared_ptr< ComponentInterface > CreateInstance(
      const PluginPath & path ) override;

private:
   // VampEffectModule implementation

   std::unique_ptr<Vamp::Plugin> FindPlugin(const PluginPath & wpath,
                            int & output,
                            bool & hasParameters);
};

#endif
