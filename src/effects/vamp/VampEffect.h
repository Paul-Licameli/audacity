/**********************************************************************

  Audacity: A Digital Audio Editor

  VampEffect.h

  Chris Cannam

  Vamp is an audio analysis and feature extraction plugin API.
  http://www.vamp-plugins.org/

**********************************************************************/



#if defined(USE_VAMP)

#include <vamp-hostsdk/PluginLoader.h>

#include "../Effect.h"

class LabelTrack;

#define VAMPEFFECTS_VERSION L"1.0.0.0"
/* i18n-hint: Vamp is the proper name of a software protocol for sound analysis.
   It is not an abbreviation for anything.  See http://vamp-plugins.org */
#define VAMPEFFECTS_FAMILY XO("Vamp")

class VampEffect final : public Effect
{
public:
   VampEffect(std::unique_ptr<Vamp::Plugin> &&plugin,
              const PluginPath & path,
              int output,
              bool hasParameters);
   virtual ~VampEffect();

   // ComponentInterface implementation

   PluginPath GetPath() override;
   ComponentInterfaceSymbol GetSymbol() override;
   VendorSymbol GetVendor() override;
   wxString GetVersion() override;
   TranslatableString GetDescription() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;
   EffectFamilySymbol GetFamily() override;
   bool IsInteractive() override;
   bool IsDefault() override;

   // EffectClientInterface implementation

   unsigned GetAudioInCount() override;
   bool GetAutomationParameters(CommandParameters & parms) override;
   bool SetAutomationParameters(CommandParameters & parms) override;

   // Effect implementation

   bool Init() override;
   bool Process() override;
   void End() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;

private:
   // VampEffect implementation

   void AddFeatures(LabelTrack *track, Vamp::Plugin::FeatureSet & features);

   void UpdateFromPlugin();

   void OnSlider( int p );
   void OnTextCtrl( int p );

private:
   std::unique_ptr<Vamp::Plugin> mPlugin;
   PluginPath mPath;
   int mOutput;
   bool mHasParameters;

   Vamp::HostExt::PluginLoader::PluginKey mKey;
   wxString mName;
   double mRate;

   bool mInSlider;
   bool mInText;

   Vamp::Plugin::ParameterList mParameters;

   Doubles mValues;

   std::vector<int> mSliders;
   ArrayOf<bool> mToggles;
   std::vector<int> mChosen;
   int mChosenProgram = 0;
};

#endif
