/*!*********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file EffectHost.cpp
 
 Paul Licameli split from Effect.cpp
 
 **********************************************************************/

#include "EffectHost.h"
#include "../PluginManager.h"

EffectHost::EffectHost(EffectClientInterface &client)
: mClient{ client }
{
}

EffectHost::~EffectHost() = default;

// EffectDefinitionInterface implementation

EffectType EffectHost::GetType()
{
   return mClient.GetType();
}

PluginPath EffectHost::GetPath()
{
   return mClient.GetPath();
}

ComponentInterfaceSymbol EffectHost::GetSymbol()
{
   return mClient.GetSymbol();
}

VendorSymbol EffectHost::GetVendor()
{
   return mClient.GetVendor();
}

wxString EffectHost::GetVersion()
{
   return mClient.GetVersion();
}

TranslatableString EffectHost::GetDescription()
{
   return mClient.GetDescription();
}

EffectFamilySymbol EffectHost::GetFamily()
{
   return mClient.GetFamily();
}

bool EffectHost::IsInteractive()
{
   return mClient.IsInteractive();
}

bool EffectHost::IsDefault()
{
   return mClient.IsDefault();
}

bool EffectHost::SupportsRealtime()
{
   return mClient.SupportsRealtime();
}

bool EffectHost::SupportsAutomation()
{
   return mClient.SupportsAutomation();
}

// EffectClientInterface implementation

bool EffectHost::SetHost(EffectHostInterface *host)
{
   return mClient.SetHost(host);
}

unsigned EffectHost::GetAudioInCount()
{
   return mClient.GetAudioInCount();
}

unsigned EffectHost::GetAudioOutCount()
{
   return mClient.GetAudioOutCount();
}

int EffectHost::GetMidiInCount()
{
   return mClient.GetMidiInCount();
}

int EffectHost::GetMidiOutCount()
{
   return mClient.GetMidiOutCount();
}

void EffectHost::SetSampleRate(double rate)
{
   mClient.SetSampleRate(rate);
   Effect::SetSampleRate(rate);
}

size_t EffectHost::SetBlockSize(size_t maxBlockSize)
{
   return mClient.SetBlockSize(maxBlockSize);
}

size_t EffectHost::GetBlockSize() const
{
   return mClient.GetBlockSize();
}

sampleCount EffectHost::GetLatency()
{
   return mClient.GetLatency();
}

size_t EffectHost::GetTailSize()
{
   return mClient.GetTailSize();
}

bool EffectHost::IsReady()
{
   return mClient.IsReady();
}

bool EffectHost::ProcessInitialize(sampleCount totalLen, ChannelNames chanMap)
{
   return mClient.ProcessInitialize(totalLen, chanMap);
}

bool EffectHost::ProcessFinalize()
{
   return mClient.ProcessFinalize();
}

size_t EffectHost::ProcessBlock(float **inBlock, float **outBlock, size_t blockLen)
{
   return mClient.ProcessBlock(inBlock, outBlock, blockLen);
}

bool EffectHost::RealtimeInitialize()
{
   mBlockSize = mClient.SetBlockSize(512);
   return mClient.RealtimeInitialize();
}

bool EffectHost::RealtimeAddProcessor(unsigned numChannels, float sampleRate)
{
   return mClient.RealtimeAddProcessor(numChannels, sampleRate);
}

bool EffectHost::RealtimeFinalize()
{
   return mClient.RealtimeFinalize();
}

bool EffectHost::RealtimeSuspend()
{
   return mClient.RealtimeSuspend();
}

bool EffectHost::RealtimeResume()
{
   return mClient.RealtimeResume();
}

bool EffectHost::RealtimeProcessStart()
{
   return mClient.RealtimeProcessStart();
}

size_t EffectHost::RealtimeProcess(int group,
                                    float **inbuf,
                                    float **outbuf,
                                    size_t numSamples)
{
   return mClient.RealtimeProcess(group, inbuf, outbuf, numSamples);
}

bool EffectHost::RealtimeProcessEnd()
{
   return mClient.RealtimeProcessEnd();
}

auto EffectHost::DoShowClientInterface(wxWindow &parent,
   const EffectDialogFactory &factory, bool forceModal) -> ShowInterfaceResult
{
   return mClient.ShowInterface(parent, factory, forceModal)
      ? ShowInterfaceResult::Success
      : ShowInterfaceResult::Failure;
}

bool EffectHost::GetAutomationParameters(CommandParameters & parms)
{
   return mClient.GetAutomationParameters(parms);
}

bool EffectHost::SetAutomationParameters(CommandParameters & parms)
{
   return mClient.SetAutomationParameters(parms);
}

bool EffectHost::LoadUserPreset(const RegistryPath & name)
{
   return mClient.LoadUserPreset(name);
}

bool EffectHost::SaveUserPreset(const RegistryPath & name)
{
   return mClient.SaveUserPreset(name);
}

RegistryPaths EffectHost::GetFactoryPresets()
{
   return mClient.GetFactoryPresets();
}

bool EffectHost::LoadFactoryPreset(int id)
{
   return mClient.LoadFactoryPreset(id);
}

bool EffectHost::LoadFactoryDefaults()
{
   return mClient.LoadFactoryDefaults();
}

// Effect implementation

PluginID EffectHost::GetID()
{
   return PluginManager::GetID(&mClient);
}

bool EffectHost::Startup()
{
   // Set host so client startup can use our services
   if (!mClient.SetHost(this))
      // Bail if the client startup fails
      return false;

   return Effect::Startup();
}
