/**********************************************************************

  Audacity: A Digital Audio Editor

  LoadCommands.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_LOAD_COMMANDS__
#define __AUDACITY_LOAD_COMMANDS__

#include "ModuleInterface.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <memory>

class AudacityCommand;

///////////////////////////////////////////////////////////////////////////////
//
// BuiltinCommandsModule
//
///////////////////////////////////////////////////////////////////////////////

class SCRIPTING_API BuiltinCommandsModule final : public ModuleInterface
{
public:
   BuiltinCommandsModule();
   virtual ~BuiltinCommandsModule();

   struct SCRIPTING_API Init{ Init(); };

   using Factory = std::function< std::unique_ptr<AudacityCommand> () >;

   // Typically you make a static object of this type in the .cpp file that
   // also implements the Command subclass.
   template< typename Subclass >
   struct Registration final {
      Registration() {
         DoRegistration(
            Subclass::Symbol, []{ return std::make_unique< Subclass >(); } );
      }
      ~Registration() {
         UndoRegistration( Subclass::Symbol );
      }
   };

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
      const RegistrationCallback &callback)
         override;

   bool IsPluginValid(const PluginPath & path, bool bFast) override;

   std::unique_ptr<ComponentInterface>
      CreateInstance(const PluginPath & path) override;

private:
   // BuiltinEffectModule implementation

   std::unique_ptr<AudacityCommand> Instantiate(const PluginPath & path);

private:
   struct Entry;

   static void DoRegistration(
      const ComponentInterfaceSymbol &name, const Factory &factory );

   static void UndoRegistration( const ComponentInterfaceSymbol &name );

   using CommandHash = std::unordered_map< wxString, const Entry* > ;
   CommandHash mCommands;
};

// Guarantees registry exists before attempts to use it
static BuiltinCommandsModule::Init sInitLoadCommands;

#endif
