#include "ViewMenuCommands.h"

#include "../Project.h"
#include "../commands/CommandManager.h"

#define FN(X) new ObjectCommandFunctor<ViewMenuCommands>(this, &ViewMenuCommands:: X )

ViewMenuCommands::ViewMenuCommands(AudacityProject *project)
   : mProject(project)
{
}

void ViewMenuCommands::Create(CommandManager *c)
{
}
