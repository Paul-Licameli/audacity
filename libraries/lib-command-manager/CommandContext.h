/**********************************************************************

  Audacity: A Digital Audio Editor

  CommandContext.h

  Created by Paul Licameli on 4/22/16.

**********************************************************************/

#ifndef __AUDACITY_COMMAND_CONTEXT__
#define __AUDACITY_COMMAND_CONTEXT__

#include <memory>
#include "CommandID.h"

class AudacityProject;
class wxEvent;
class CommandOutputTargets;

class COMMAND_MANAGER_API CommandContext {
public:
   CommandContext(
      AudacityProject &p
      , std::unique_ptr<CommandOutputTargets> target
      , const wxEvent *e = nullptr
      , int ii = 0
      , const CommandParameter &param = CommandParameter{}
   );

   ~CommandContext();

   virtual void Status( const wxString &message, bool bFlush = false ) const;
   virtual void Error(  const wxString &message ) const;
   virtual void Progress( double d ) const;

   // Output formatting...
   void StartArray() const;
   void EndArray() const;
   void StartStruct() const;
   void EndStruct() const;
   void StartField(const wxString &name) const;
   void EndField() const;
   void AddItem(const wxString &value , const wxString &name = {} ) const;
   void AddBool(const bool value      , const wxString &name = {} ) const;
   void AddItem(const double value    , const wxString &name = {} ) const;

   AudacityProject &project;
   std::unique_ptr<CommandOutputTargets> pOutput;
   const wxEvent *pEvt;
   int index;
   CommandParameter parameter;
};
#endif
