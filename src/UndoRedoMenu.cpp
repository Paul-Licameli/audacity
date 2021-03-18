/**********************************************************************

  Audacity: A Digital Audio Editor

  @file UndoRedoMenu.cpp

  Paul Licameli split from Menus.cpp

**********************************************************************/

#include "UndoRedoMenu.h"
#include "Internat.h"
#include "ProjectCommandManager.h"
#include "Menus.h"
#include "Project.h"
#include "ProjectHistory.h"
#include "UndoManager.h"

void ModifyUndoMenuItems(AudacityProject &project)
{
   TranslatableString desc;
   auto &undoManager = UndoManager::Get( project );
   auto &commandManager = ProjectCommandManager::Get( project );
   int cur = undoManager.GetCurrentState();

   if (undoManager.UndoAvailable()) {
      undoManager.GetShortDescription(cur, &desc);
      commandManager.Modify(wxT("Undo"),
         XXO("&Undo %s")
            .Format( desc ));
      commandManager.Enable(wxT("Undo"),
         ProjectHistory::Get( project ).UndoAvailable());
   }
   else {
      commandManager.Modify(wxT("Undo"),
                            XXO("&Undo"));
   }

   if (undoManager.RedoAvailable()) {
      undoManager.GetShortDescription(cur+1, &desc);
      commandManager.Modify(wxT("Redo"),
         XXO("&Redo %s")
            .Format( desc ));
      commandManager.Enable(wxT("Redo"),
         ProjectHistory::Get( project ).RedoAvailable());
   }
   else {
      commandManager.Modify(wxT("Redo"),
                            XXO("&Redo"));
      commandManager.Enable(wxT("Redo"), false);
   }
}

static AudacityProject::AttachedObjects::RegisteredFactory sKey {
   []( AudacityProject &project ) {
      auto handler = [&project](UndoRedoEvent &evt){
         evt.Skip();
         ModifyUndoMenuItems( project );
         MenuManager::Get(project)
            .UpdateMenus(true, ProjectCommandManager::Get(project));
      };
      project.Bind( EVT_UNDO_OR_REDO, handler);
      project.Bind( EVT_UNDO_RESET, handler);
      project.Bind( EVT_UNDO_PUSHED, handler);
      project.Bind( EVT_UNDO_RENAMED, handler);
      // Don't need to construct anything
      return nullptr;
   }
};
