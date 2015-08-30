#include "../Audacity.h"
#include "HelpMenuCommands.h"

#include "../Benchmark.h"
#include "../Project.h"
#include "../Screenshot.h"
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

   c->AddSeparator();

   c->AddItem(wxT("Screenshot"), _("&Screenshot Tools..."), FN(OnScreenshot));
#if IS_ALPHA
   // TODO: What should we do here?  Make benchmark a plug-in?
   // Easy enough to do.  We'd call it mod-self-test.
   c->AddItem(wxT("Benchmark"), _("&Run Benchmark..."), FN(OnBenchmark));
#endif
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

void HelpMenuCommands::OnBenchmark()
{
   ::RunBenchmark(mProject);
}
