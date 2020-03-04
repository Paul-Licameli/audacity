/**********************************************************************

  Audacity: A Digital Audio Editor

  TracksBehaviorsPrefs.cpp

  Steve Daulton


*******************************************************************//**

\class TracksBehaviorsPrefs
\brief A PrefsPanel for Tracks Behaviors settings.

*//*******************************************************************/


#include "TracksBehaviorsPrefs.h"

#include "../Prefs.h"
#include "../ShuttleGui.h"

TracksBehaviorsPrefs::TracksBehaviorsPrefs(wxWindow * parent, wxWindowID winid)
/* i18n-hint: i.e. the behaviors of tracks */
:  PrefsPanel(parent, winid, XO("Tracks Behaviors"))
{
}

TracksBehaviorsPrefs::~TracksBehaviorsPrefs()
{
}

ComponentInterfaceSymbol TracksBehaviorsPrefs::GetSymbol()
{
   return TRACKS_BEHAVIORS_PREFS_PLUGIN_SYMBOL;
}

TranslatableString TracksBehaviorsPrefs::GetDescription()
{
   return XO("Preferences for TracksBehaviors");
}

wxString TracksBehaviorsPrefs::HelpPageName()
{
   return "Tracks_Behaviors_Preferences";
}

ChoiceSetting TracksBehaviorsSolo{
   L"/GUI/Solo",
   {
      ByColumns,
      { XO("Simple"),  XO("Multi-track"), XO("None") },
      { L"Simple", L"Multi",      L"None" }
   },
   0, // "Simple"
};

void TracksBehaviorsPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S
      .StartStatic(XO("Behaviors"));
   {
      S
         .TieCheckBox(XXO("&Select all audio, if selection required"),
            TracksBehaviorsSelectAllOnNone);

      /* i18n-hint: Cut-lines are lines that can expand to show the cut audio.*/
      S
         .TieCheckBox(XXO("Enable cut &lines"),
            TracksBehaviorsCutLines);

      S
         .TieCheckBox(XXO("Enable &dragging selection edges"),
            TracksBehaviorsAdjustSelectionEdges);

      S
         .TieCheckBox(XXO("Editing a clip can &move other clips"),
            TracksBehaviorsClipsCanMove);

      S
         .TieCheckBox(XXO("\"Move track focus\" c&ycles repeatedly through tracks"),
            TracksBehaviorsCircularNavigation);

      S
         .TieCheckBox(XXO("&Type to create a label"),
            TracksBehaviorsTypeToCreateLabel);

      S
         .TieCheckBox(XXO("Use dialog for the &name of a new label"),
            TracksBehaviorsDialogForNameNewLabel);

#ifdef EXPERIMENTAL_SCROLLING_LIMITS
      S
         .TieCheckBox(XXO("Enable scrolling left of &zero"),
            TracksBehaviorsScrollBeyondZero);
#endif

      S
         .TieCheckBox(XXO("Advanced &vertical zooming"),
            TracksBehaviorsAdvancedVerticalZooming);

      S.AddSpace(10);

      S.StartMultiColumn(2);
      {
         S
            .TieChoice( XXO("Solo &Button:"), TracksBehaviorsSolo);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
   S.EndScroller();
}

bool TracksBehaviorsPrefs::Commit()
{
   wxPanel::TransferDataFromWindow();
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   return true;
}

namespace{
PrefsPanel::Registration sAttachment{ "TracksBehaviors",
   [](wxWindow *parent, wxWindowID winid, AudacityProject *)
   {
      wxASSERT(parent); // to justify safenew
      return safenew TracksBehaviorsPrefs(parent, winid);
   },
   false,
   // Place it at a lower tree level
   { "Tracks" }
};
}

// Bug 825 is essentially that SyncLock requires EditClipsCanMove.
// SyncLock needs rethinking, but meanwhile this function
// fixes the issues of Bug 825 by allowing clips to move when in
// SyncLock.
bool GetEditClipsCanMove()
{
   bool mIsSyncLocked = TracksBehaviorsSyncLockTracks.Read();
   if( mIsSyncLocked )
      return true;
   return TracksBehaviorsClipsCanMove.Read();
}

BoolSetting TracksBehaviorsAdjustSelectionEdges{
   L"/GUI/AdjustSelectionEdges",    true  };
BoolSetting TracksBehaviorsCircularNavigation{
   L"/GUI/CircularTrackNavigation", false };
BoolSetting TracksBehaviorsClipsCanMove{
   L"/GUI/EditClipCanMove",         true  };
BoolSetting TracksBehaviorsCutLines{
   L"/GUI/EnableCutLines",          false };
BoolSetting TracksBehaviorsDialogForNameNewLabel{
   L"/GUI/DialogForNameNewLabel",   false };
BoolSetting TracksBehaviorsSelectAllOnNone{
   L"/GUI/SelectAllOnNone",         false };
BoolSetting TracksBehaviorsSyncLockTracks{
   L"/GUI/SyncLockTracks",          false  };
BoolSetting TracksBehaviorsTypeToCreateLabel{
   L"/GUI/TypeToCreateLabel",       false };
BoolSetting TracksBehaviorsScrollBeyondZero{
   L"/GUI/ScrollBeyondZero",        false };
BoolSetting TracksBehaviorsAdvancedVerticalZooming{
   L"/GUI/VerticalZooming",         false };

