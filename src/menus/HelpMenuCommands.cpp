#include "HelpMenuCommands.h"

#include "../Project.h"
#include "../Screenshot.h"
#include "../commands/CommandManager.h"
#include "../widgets/HelpSystem.h"

#define FN(X) new ObjectCommandFunctor<HelpMenuCommands>(this, &HelpMenuCommands:: X )

HelpMenuCommands::HelpMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void HelpMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddItem(wxT("QuickHelp"), _("&Quick Help"), FN(OnQuickHelp));
   c->AddItem(wxT("Manual"), _("&Manual"), FN(OnManual));

   c->AddSeparator();

   c->AddItem(wxT("Screenshot"), _("&Screenshot Tools..."), FN(OnScreenshot));
}

void HelpMenuCommands::OnQuickHelp()
{
   HelpSystem::ShowHelpDialog(
      mProject,
      wxT("Quick_Help"));
}

void HelpMenuCommands::OnManual()
{
   HelpSystem::ShowHelpDialog(
      mProject,
      wxT("Main_Page"));
}

void HelpMenuCommands::OnScreenshot()
{
   ::OpenScreenshotTools();
}
