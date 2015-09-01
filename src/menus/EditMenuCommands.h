#ifndef __AUDACITY_EDIT_MENU_COMMANDS__
#define __AUDACITY_EDIT_MENU_COMMANDS__

class AudacityProject;
class CommandManager;

class EditMenuCommands
{
   EditMenuCommands(const EditMenuCommands&);
   EditMenuCommands& operator= (const EditMenuCommands&);
public:
   EditMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);

   void OnUndo();
   void OnRedo();
   void OnCut();
private:

   AudacityProject *const mProject;
};
#endif
