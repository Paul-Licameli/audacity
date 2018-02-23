/**********************************************************************

  Audacity: A Digital Audio Editor

  BatchCommands.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_BATCH_COMMANDS_DIALOG__
#define __AUDACITY_BATCH_COMMANDS_DIALOG__

#include <wx/defs.h>
#include <wx/string.h>

#include "export/Export.h"

class Effect;
class CommandContext;

class BatchCommands final {
 public:
   // constructors and destructors
   BatchCommands();
 public:
   bool ApplyChain(const wxString & filename = wxT(""));
   bool ApplyCommand( const wxString & command, const wxString & params, CommandContext const * pContext=NULL );
   bool ApplyCommandInBatchMode(const wxString & command, const wxString &params);
   bool ApplySpecialCommand(int iCommand, const wxString & command,const wxString & params);
   bool ApplyEffectCommand(const PluginID & ID, const wxString & command, const wxString & params, const CommandContext & Context);
   bool ReportAndSkip( const wxString & command, const wxString & params );
   void AbortBatch();

   // Utility functions for the special commands.
   wxString BuildCleanFileName(const wxString &fileName, const wxString &extension);
   bool WriteMp3File( const wxString & Name, int bitrate );
   double GetEndTime();
   bool IsMono();

   // These commands do not depend on the command list.
   static wxArrayString GetNames();

   // A pair of user-visible name, and internal string identifier
   using CommandName = std::pair<wxString, wxString>;
   using CommandNameVector = std::vector<CommandName>;
   // Result is sorted by user-visible name
   static CommandNameVector GetAllCommands();

   static wxString GetCurrentParamsFor(const wxString & command);
   static wxString PromptForParamsFor(const wxString & command, const wxString & params, wxWindow *parent);
   static wxString PromptForPresetFor(const wxString & command, const wxString & params, wxWindow *parent);

   // These commands do depend on the command list.
   void ResetChain();

   bool ReadChain(const wxString & chain);
   bool WriteChain(const wxString & chain);
   bool AddChain(const wxString & chain);
   bool DeleteChain(const wxString & name);
   bool RenameChain(const wxString & oldchain, const wxString & newchain);

   void AddToChain(const wxString & command, int before = -1);
   void AddToChain(const wxString & command, const wxString & params, int before = -1);
   void DeleteFromChain(int index);
   wxString GetCommand(int index);
   wxString GetParams(int index);
   int GetCount();
   wxString GetMessage(){ return mMessage;};
   void AddToMessage(const wxString & msgIn ){ mMessage += msgIn;};

   void SetWavToMp3Chain();

   bool IsFixed(const wxString & name);

   void RestoreChain(const wxString & name);

   void Split(const wxString & str, wxString & command, wxString & param);
   wxString Join(const wxString & command, const wxString & param);

   wxArrayString mCommandChain;
   wxArrayString mParamsChain;
   bool mAbort;
   wxString mMessage;

   Exporter mExporter;
   wxString mFileName;
};

#endif
