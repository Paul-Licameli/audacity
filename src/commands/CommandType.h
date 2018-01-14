/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: wxwidgets

   Dan Horgan

******************************************************************//**

\file CommandType.h
\brief Contains declarations for CommandType class

*//*******************************************************************/

#ifndef __COMMANDTYPE__
#define __COMMANDTYPE__

#include "CommandMisc.h"
#include "CommandSignature.h"
#include "../MemoryX.h"
#include "../commands/AudacityCommand.h"

class OldStyleCommand;

/**************************************************************//**

\class OldStyleCommand 
\brief OldStyleCommand is the key class that allows us to carry
a converted (not textual) command from a non-GUI place to the GUI
thread.  It contains the command AND the context that will be used 
for its output.

\class OldStyleCommandPointer
\brief OldStyleCommandPointer is a shared_ptr to a OldStyleCommandPointer.

*******************************************************************/

using OldStyleCommandPointer = std::shared_ptr<OldStyleCommand>;
class CommandOutputTarget;
class CommandSignature;
class wxString;

class CommandType : public AudacityCommand
{
private:
   wxString mName;
   Maybe<CommandSignature> mSignature;

public:
   CommandType();
   virtual ~CommandType();
   wxString GetName() override;
   CommandSignature &GetSignature();
   wxString Describe();

   // Subclasses should override the following:
   // =========================================

   // Return the name of the command type
   virtual wxString BuildName() = 0;

   /// Postcondition: signature is a 'signature' map containing parameter
   // names, validators and default values.
   virtual void BuildSignature(CommandSignature &signature) = 0;

   // Create a command instance with the specified output target
   virtual OldStyleCommandPointer Create(std::unique_ptr<CommandOutputTarget> &&target) = 0;
};

#endif /* End of include guard: __COMMANDTYPE__ */
