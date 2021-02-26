/**********************************************************************

  Audacity: A Digital Audio Editor

  ReloadPreferences.cpp

  Paul Licameli split from PrefsDialog.cpp

**********************************************************************/

#include "ReloadPreferences.h"
#include <wx/frame.h>
#include "Menus.h"
#include "Project.h"
#include "ProjectCommandManager.h"
#include "ProjectWindows.h"
#include "PrefsDialog.h"

void DoReloadPreferences( AudacityProject &project )
{
   PreferenceInitializer::ReinitializeAll();

   {
      GlobalPrefsDialog dialog(
         &GetProjectFrame( project ) /* parent */, &project );
      wxCommandEvent Evt;
      //dialog.Show();
      dialog.OnOK(Evt);
   }

   // LL:  Moved from PrefsDialog since wxWidgets on OSX can't deal with
   //      rebuilding the menus while the PrefsDialog is still in the modal
   //      state.
   for (auto p : AllProjects{}) {
      auto &cm = ProjectCommandManager::Get(*p);
      MenuManager::Get(*p).RebuildMenuBar(*p, cm);
// TODO: The comment below suggests this workaround is obsolete.
#if defined(__WXGTK__)
      // Workaround for:
      //
      //   http://bugzilla.audacityteam.org/show_bug.cgi?id=458
      //
      // This workaround should be removed when Audacity updates to wxWidgets
      // 3.x which has a fix.
      auto &window = GetProjectFrame( *p );
      wxRect r = window.GetRect();
      window.SetSize(wxSize(1,1));
      window.SetSize(r.GetSize());
#endif
   }
}
