/**********************************************************************

  Audacity: A Digital Audio Editor

  @file MP3Prefs.cpp
  @brief adds controls for MP3 import/export to Library preferences

  Paul Licameli split from LibraryPrefs.cpp

**********************************************************************/

#include "ExportMP3.h"
#include "Internat.h"
#include "ShuttleGui.h"
#include "LibraryPrefs.h"
#include "HelpSystem.h"
#include "ReadOnlyText.h"
#include <wx/button.h>
#include <wx/stattext.h>

namespace {

struct State {
   wxWindow *parent = nullptr;
   ReadOnlyText *MP3Version = nullptr;
};

/// Sets the a text area on the dialog to have the name
/// of the MP3 Library version.
void SetMP3VersionText(State &state, bool prompt = false)
{
   auto MP3Version = state.MP3Version;
   MP3Version->SetValue(GetMP3Version(state.parent, prompt));
}

void AddControls( ShuttleGui &S )
{
   auto pState = std::make_shared<State>();
   pState->parent = S.GetParent();

   S.StartStatic(XO("LAME MP3 Export Library"));
   {
      S.StartTwoColumn();
      {
         auto mP3Version = S
            .Position(wxALIGN_CENTRE_VERTICAL)
            .AddReadOnlyText(XO("MP3 Library Version:"), "");
      }
      S.EndTwoColumn();
   }
   S.EndStatic();

   SetMP3VersionText(*pState);
}

LibraryPrefs::RegisteredControls reg{ wxT("MP3"), AddControls };

}
