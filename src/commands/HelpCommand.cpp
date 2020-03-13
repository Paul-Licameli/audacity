/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2018 Audacity Team
   License: wxwidgets

   Dan Horgan
   James Crook

******************************************************************//**

\file HelpCommand.cpp
\brief Definitions for HelpCommand and HelpCommandType classes

*//*******************************************************************/


#include "HelpCommand.h"

#include "../Shuttle.h"
#include "LoadCommands.h"
#include "../ShuttleGui.h"
#include "CommandTargets.h"
#include "CommandContext.h"
#include "../effects/EffectManager.h"

const ComponentInterfaceSymbol HelpCommand::Symbol
{ XO("Help") };

const ComponentInterfaceSymbol CommentCommand::Symbol
{ XO("Comment") };

namespace{ BuiltinCommandsModule::Registration< HelpCommand > reg; }
namespace{ BuiltinCommandsModule::Registration< CommentCommand > reg2; }

enum {
   kJson,
   kLisp,
   kBrief,
   nFormats
};

static const EnumValueSymbol kFormats[nFormats] =
{
   // These are acceptable dual purpose internal/visible names
   
   /* i18n-hint JavaScript Object Notation */
   { XO("JSON") },
   /* i18n-hint name of a computer programming language */
   { XO("LISP") },
   { XO("Brief") }
};


bool HelpCommand::DefineParams( ShuttleParams & S ){
   S.Define( mCommandName, L"Command",  "Help" );
   S.DefineEnum( mFormat, L"Format", 0, kFormats, nFormats );
   return true;
}

void HelpCommand::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      S
         .Target( mCommandName )
         .AddTextBox(XXO("Command:"));

      S
         .Target( mFormat )
         .AddChoice( XXO("Format:"),
            Msgids( kFormats, nFormats ));
   }
   S.EndMultiColumn();
}

bool HelpCommand::Apply(const CommandContext &context)
{
   if( mFormat == kJson )
      return ApplyInner( context );

   if( mFormat == kLisp )
   {
      CommandContext LispyContext( 
         context.project,
         std::make_unique<LispifiedCommandOutputTargets>( *context.pOutput.get() )
         );
      return ApplyInner( LispyContext );
   }

   if( mFormat == kBrief )
   {
      CommandContext BriefContext( 
         context.project,
         std::make_unique<BriefCommandOutputTargets>( *context.pOutput.get() )
         );
      return ApplyInner( BriefContext );
   }

   return false;
}

bool HelpCommand::ApplyInner(const CommandContext & context){
   EffectManager & em = EffectManager::Get();
   PluginID ID = em.GetEffectByIdentifier( mCommandName );
   if( ID.empty() )
      context.Status( "Command not found" );
   else
      em.GetCommandDefinition( ID, context, 1);
   return true;
}

bool CommentCommand::DefineParams( ShuttleParams & S ){
   S.Define( mComment, L"_",  "" );
   return true;
}

void CommentCommand::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      S.TieTextBox(XXO("_"),mComment,80);
   }
   S.EndMultiColumn();
}

