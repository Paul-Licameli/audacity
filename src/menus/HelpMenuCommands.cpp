#include "../Audacity.h"
#include "HelpMenuCommands.h"

#include "../Project.h"
#include "../commands/CommandManager.h"
#include "../widgets/HelpSystem.h"

#define FN(X) FNT(HelpMenuCommands, this, & HelpMenuCommands :: X)

HelpMenuCommands::HelpMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void HelpMenuCommands::Create(CommandManager *c)
{
   c->SetDefaultFlags(AlwaysEnabledFlag, AlwaysEnabledFlag);

   c->AddItem(wxT("QuickHelp"), _("&Quick Help"), FN(OnQuickHelp));
   c->AddItem(wxT("Manual"), _("&Manual"), FN(OnManual));
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
