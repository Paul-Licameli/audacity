#include "../CommonCommandFlags.h"
#include "../Menus.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../commands/CommandContext.h"
#include "../commands/CommandManager.h"
#include "../prefs/GUIPrefs.h"
#include "../toolbars/MixerToolBar.h"
#include "../toolbars/DeviceToolBar.h"

#include <wx/frame.h>

// helper functions and classes
namespace {
}

/// Namespace for helper functions for Extra menu
namespace ExtraActions {

// exported helper functions
// none

// Menu handler functions

struct Handler : CommandHandlerObject {

void OnOutputGain(const CommandContext &context)
{
   auto &project = context.project;
   auto tb = &MixerToolBar::Get( project );

   if (tb) {
      tb->ShowOutputGainDialog();
   }
}

void OnOutputGainInc(const CommandContext &context)
{
   auto &project = context.project;
   auto tb = &MixerToolBar::Get( project );

   if (tb) {
      tb->AdjustOutputGain(1);
   }
}

void OnOutputGainDec(const CommandContext &context)
{
   auto &project = context.project;
   auto tb = &MixerToolBar::Get( project );

   if (tb) {
      tb->AdjustOutputGain(-1);
   }
}

void OnInputGain(const CommandContext &context)
{
   auto &project = context.project;
   auto tb = &MixerToolBar::Get( project );

   if (tb) {
      tb->ShowInputGainDialog();
   }
}

void OnInputGainInc(const CommandContext &context)
{
   auto &project = context.project;
   auto tb = &MixerToolBar::Get( project );

   if (tb) {
      tb->AdjustInputGain(1);
   }
}

void OnInputGainDec(const CommandContext &context)
{
   auto &project = context.project;
   auto tb = &MixerToolBar::Get( project );

   if (tb) {
      tb->AdjustInputGain(-1);
   }
}

void OnInputDevice(const CommandContext &context)
{
   auto &project = context.project;
   auto &tb = DeviceToolBar::Get( project );
   tb.ShowInputDialog();
}

void OnOutputDevice(const CommandContext &context)
{
   auto &project = context.project;
   auto &tb = DeviceToolBar::Get( project );
   tb.ShowOutputDialog();
}

void OnInputChannels(const CommandContext &context)
{
   auto &project = context.project;
   auto &tb = DeviceToolBar::Get( project );
   tb.ShowChannelsDialog();
}

void OnAudioHost(const CommandContext &context)
{
   auto &project = context.project;
   auto &tb = DeviceToolBar::Get( project );
   tb.ShowHostDialog();
}

void OnFullScreen(const CommandContext &context)
{
   auto &project = context.project;
   auto &window = GetProjectFrame( project );

   bool bChecked = !window.wxTopLevelWindow::IsFullScreen();
   window.wxTopLevelWindow::ShowFullScreen(bChecked);

   MenuManager::Get(project).ModifyToolbarMenus(project);
}

}; // struct Handler

} // namespace

static CommandHandlerObject &findCommandHandler(AudacityProject &) {
   // Handler is not stateful.  Doesn't need a factory registered with
   // AudacityProject.
   static ExtraActions::Handler instance;
   return instance;
};

// Menu definitions

#define FN(X) (& ExtraActions::Handler :: X)

namespace {
using namespace MenuTable;

BaseItemSharedPtr ExtraMixerMenu();
BaseItemSharedPtr ExtraDeviceMenu();

BaseItemSharedPtr ExtraMenu()
{
   // Table of menu factories.
   // TODO:  devise a registration system instead.
   static BaseItemSharedPtr extraItems{ Items( wxEmptyString,
      Section( "Part1",
           ExtraMixerMenu()
         , ExtraDeviceMenu()
      ),

      Section( "Part2" )
   ) };

   static const auto pred =
      []{ return GUIShowExtraMenus.Read(); };
   static BaseItemSharedPtr menu{
      ConditionalItems( L"Optional",
         pred, Menu( L"Extra", XXO("Ext&ra"), extraItems ) )
   };
   return menu;
}

AttachedItem sAttachment1{
   L"",
   Shared( ExtraMenu() )
};

// Under /MenuBar/Optional/Extra/Part1
BaseItemSharedPtr ExtraMixerMenu()
{
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"Mixer", XXO("Mi&xer"),
      Command( L"OutputGain", XXO("Ad&just Playback Volume..."),
         FN(OnOutputGain), AlwaysEnabledFlag ),
      Command( L"OutputGainInc", XXO("&Increase Playback Volume"),
         FN(OnOutputGainInc), AlwaysEnabledFlag ),
      Command( L"OutputGainDec", XXO("&Decrease Playback Volume"),
         FN(OnOutputGainDec), AlwaysEnabledFlag ),
      Command( L"InputGain", XXO("Adj&ust Recording Volume..."),
         FN(OnInputGain), AlwaysEnabledFlag ),
      Command( L"InputGainInc", XXO("I&ncrease Recording Volume"),
         FN(OnInputGainInc), AlwaysEnabledFlag ),
      Command( L"InputGainDec", XXO("D&ecrease Recording Volume"),
         FN(OnInputGainDec), AlwaysEnabledFlag )
   ) ) };
   return menu;
}

// Under /MenuBar/Optional/Extra/Part1
BaseItemSharedPtr ExtraDeviceMenu()
{
   static BaseItemSharedPtr menu{
   ( FinderScope{ findCommandHandler },
   Menu( L"Device", XXO("De&vice"),
      Command( L"InputDevice", XXO("Change &Recording Device..."),
         FN(OnInputDevice),
         AudioIONotBusyFlag(), L"Shift+I" ),
      Command( L"OutputDevice", XXO("Change &Playback Device..."),
         FN(OnOutputDevice),
         AudioIONotBusyFlag(), L"Shift+O" ),
      Command( L"AudioHost", XXO("Change Audio &Host..."), FN(OnAudioHost),
         AudioIONotBusyFlag(), L"Shift+H" ),
      Command( L"InputChannels", XXO("Change Recording Cha&nnels..."),
         FN(OnInputChannels),
         AudioIONotBusyFlag(), L"Shift+N" )
   ) ) };
   return menu;
}

// Under /MenuBar/Optional/Extra/Part2
BaseItemSharedPtr ExtraMiscItems()
{
   using Options = CommandManager::Options;

   // Not a menu.
   static BaseItemSharedPtr items{
   Items( L"Misc",
      // Delayed evaluation
      []( AudacityProject &project ) {

   static const auto key =
#ifdef __WXMAC__
      L"Ctrl+/"
#else
      L"F11"
#endif
   ;

         return (
         FinderScope{ findCommandHandler },
         // Accel key is not bindable.
         Command( L"FullScreenOnOff", XXO("&Full Screen (on/off)"),
            FN(OnFullScreen),
            AlwaysEnabledFlag,
            Options{ key }.CheckTest( []( const AudacityProject &project ) {
               return GetProjectFrame( project )
                  .wxTopLevelWindow::IsFullScreen(); } ) )
        );
      }
   ) };
   return items;
}

AttachedItem sAttachment2{
   Placement{ L"Optional/Extra/Part2", { OrderingHint::End } },
   Shared( ExtraMiscItems() )
};

}

#undef FN
