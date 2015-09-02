#ifndef __AUDACITY_EDIT_MENU_COMMANDS__
#define __AUDACITY_EDIT_MENU_COMMANDS__

#include "../Experimental.h"

class AudacityProject;
class CommandManager;
class CursorAndFocusCommands;
class Regions;
class Track;
class WaveTrack;

class EditMenuCommands
{
   EditMenuCommands(const EditMenuCommands&);
   EditMenuCommands& operator= (const EditMenuCommands&);
public:
   EditMenuCommands(AudacityProject *project);
   void Create(CommandManager *c);
   void CreateNonMenuCommands(CommandManager *c);

   void OnUndo();
   void OnRedo();
   void OnCut();
   void OnDelete();
   void OnCopy();

   void OnPaste();
private:
   void OnPasteOver();
   bool HandlePasteText(); // Handle text paste (into active label), if any. Return true if pasted.
   bool HandlePasteNothingSelected(); // Return true if nothing selected, regardless of paste result.

   void OnDuplicate();
   void OnSplitCut();
   void OnSplitDelete();
public:
   void OnSilence();
   void OnTrim();
private:
   void OnPasteNewLabel();
   void OnSplit();
   void OnSplitNew();
   void OnJoin();
   void OnDisjoin();

   void OnCutLabels();
   void OnDeleteLabels();
   void OnSplitCutLabels();
   void OnSplitDeleteLabels();
   void OnSilenceLabels();
   void OnCopyLabels();
   void OnSplitLabels();
   void OnJoinLabels();
   void OnDisjoinLabels();

   typedef bool (WaveTrack::* EditFunction)(double, double);
   void EditByLabel(EditFunction action, bool bSyncLockedTracks);

   typedef bool (WaveTrack::* EditDestFunction)(double, double, Track**);
   void EditClipboardByLabel(EditDestFunction action);

   void GetRegionsByLabel(Regions &regions);
   void ClearClipboard();


public:
   void OnSelectAll();
   void SelectAllIfNone();
   void OnSelectNone();

   void OnLockPlayRegion();
   void OnUnlockPlayRegion();
   void OnPreferences();

private:

   AudacityProject *const mProject;
   CursorAndFocusCommands *const mCursorAndFocusCommands;
};
#endif
