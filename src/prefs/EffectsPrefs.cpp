/**********************************************************************

  Audacity: A Digital Audio Editor

  EffectsPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class EffectsPrefs
\brief A PrefsPanel for general GUI preferences.

*//*******************************************************************/


#include "EffectsPrefs.h"

#include <wx/choice.h>
#include <wx/defs.h>

#include "Languages.h"
#include "../PluginManager.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

EffectsPrefs::EffectsPrefs(wxWindow * parent, wxWindowID winid)
:  PrefsPanel(parent, winid, XO("Effects"))
{
}

EffectsPrefs::~EffectsPrefs()
{
}

ComponentInterfaceSymbol EffectsPrefs::GetSymbol()
{
   return EFFECTS_PREFS_PLUGIN_SYMBOL;
}

TranslatableString EffectsPrefs::GetDescription()
{
   return XO("Preferences for Effects");
}

wxString EffectsPrefs::HelpPageName()
{
   return "Effects_Preferences";
}

ChoiceSetting EffectsGroupBy{
   L"/Effects/GroupBy",
   {
      ByColumns,
      {
         XO("Sorted by Effect Name") ,
         XO("Sorted by Publisher and Effect Name") ,
         XO("Sorted by Type and Effect Name") ,
         XO("Grouped by Publisher") ,
         XO("Grouped by Type") ,
      },
      {
         L"sortby:name" ,
         L"sortby:publisher:name" ,
         L"sortby:type:name" ,
         L"groupby:publisher" ,
         L"groupby:type" ,
      }
   },
   0 // "sortby:name"
};

namespace {

// Rather than hard-code an exhaustive list of effect families in this file,
// pretend we don't know, but discover them instead by querying the module and
// effect managers.

// But then we would like to have prompts with accelerator characters that are
// distinct.  We collect some prompts in the following map.

// It is not required that each module be found here, nor that each module
// mentioned here be found.
const std::map< wxString, TranslatableLabel > SuggestedPrompts{

/* i18n-hint: Audio Unit is the name of an Apple audio software protocol */
   { L"AudioUnit", XXO("Audio Unit") },

/* i18n-hint: abbreviates "Linux Audio Developer's Simple Plugin API"
   (Application programming interface)
 */
   { L"LADSPA",    XXO("&LADSPA") },

/* i18n-hint: abbreviates
   "Linux Audio Developer's Simple Plugin API (LADSPA) version 2" */
   { L"LV2",       XXO("LV&2") },

/* i18n-hint: "Nyquist" is an embedded interpreted programming language in
 Audacity, named in honor of the Swedish-American Harry Nyquist (or Nyqvist).
 In the translations of this and other strings, you may transliterate the
 name into another alphabet.  */
   { L"Nyquist",   XXO("N&yquist") },

/* i18n-hint: Vamp is the proper name of a software protocol for sound analysis.
   It is not an abbreviation for anything.  See http://vamp-plugins.org */
   { L"Vamp",      XXO("&Vamp") },

/* i18n-hint: Abbreviates Virtual Studio Technology, an audio software protocol
   developed by Steinberg GmbH */
   { L"VST",       XXO("V&ST") },

};

// Collect needed prompts and settings paths, at most once, on demand
struct Entry {
   TranslatableLabel prompt;
   BoolSetting *pSetting;
};
static const std::vector< Entry > &GetModuleData()
{
   struct ModuleData : public std::vector< Entry > {
      ModuleData() {
         auto &pm = PluginManager::Get();
         for (auto plug = pm.GetFirstPlugin(PluginTypeModule);
              plug;
              plug = pm.GetNextPlugin(PluginTypeModule)) {
            auto internal = plug->GetEffectFamily();
            if ( internal.empty() )
               continue;

            auto pSetting = pm.GetPluginEnabledSetting( *plug );
            if ( !pSetting )
               continue;

            TranslatableLabel prompt;
            auto iter = SuggestedPrompts.find( internal );
            if ( iter == SuggestedPrompts.end() )
               // For the built-in modules this Msgid includes " Effects",
               // but those strings were never shown to the user,
               // and the prompts in the table above do not include it.
               // If there should be new modules, it is not important for them
               // to follow the " Effects" convention, but instead they can
               // have shorter msgids.
               prompt = TranslatableLabel{ plug->GetSymbol().Msgid() };
            else
               prompt = iter->second;

            push_back( { prompt, pSetting } );
         }
         // Guarantee some determinate ordering
         std::sort( begin(), end(),
            []( const Entry &a, const Entry &b ){
               return a.pSetting->GetPath() < b.pSetting->GetPath();
            }
         );
      }
   };
   static ModuleData theData;
   return theData;
}

}

void EffectsPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(XO("Enable Effects"));
   {
      for ( const auto &entry : GetModuleData() )
      {
         S
            .Target( *entry.pSetting )
            .AddCheckBox( entry.prompt );
      }
   }
   S.EndStatic();

   S.StartStatic(XO("Effect Options"));
   {
      S.StartMultiColumn(2);
      {
         S
            .MinSize()
            .Target( EffectsGroupBy )
            .AddChoice( XXO("S&ort or Group:") );

         S
            .Target( EffectsMaxPerGroup )
            .AddTextBox(XXO("&Maximum effects per group (0 to disable):"),
               {}, 5);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();

#if 1//ndef EXPERIMENTAL_EFFECT_MANAGEMENT
   S.StartStatic(XO("Plugin Options"));
   {
      S
         .Target( PluginsCheckForUpdates )
         .AddCheckBox( XXO("Check for updated plugins when Audacity starts") );

      S
         .Target( PluginsRescan )
         .AddCheckBox( XXO("Rescan plugins next time Audacity is started") );
   }
   S.EndStatic();
#endif

#ifdef EXPERIMENTAL_EQ_SSE_THREADED
   S.StartStatic(XO("Instruction Set"));
   {
      S
         .Target( SSEGUI )
         .AddCheckBox( XXO("&Use SSE/SSE2/.../AVX") );
   }
   S.EndStatic();
#endif
   S.EndScroller();
}

bool EffectsPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "Effects",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew EffectsPrefs(parent, winid);
   }
};
}

BoolSetting PluginsCheckForUpdates{ L"/Plugins/CheckForUpdates", true };
BoolSetting PluginsRescan{ L"/Plugins/Rescan", false };

BoolSetting SSEGUI{ L"/SSE/GUI", true };

IntSetting EffectsMaxPerGroup{
   L"/Effects/MaxPerGroup",
#if defined(__WXGTK__)
   15
#else
   0
#endif
};

