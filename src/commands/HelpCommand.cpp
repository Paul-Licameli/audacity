/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: wxwidgets

   Dan Horgan

******************************************************************//**

\file HelpCommand.cpp
\brief Definitions for HelpCommand and HelpCommandType classes

*//*******************************************************************/

#include "../Audacity.h"
#include "HelpCommand.h"
//#include "../Project.h"
//#include "../Track.h"
#include "../ShuttleGui.h"
#include "CommandContext.h"
#include "../effects/EffectManager.h"

bool HelpCommand::DefineParams( ShuttleParams & S ){
   S.Define( mCommandName, wxT("Command"),  "Help" );
   return true;
}

void HelpCommand::PopulateOrExchange(ShuttleGui & S)
{
   S.AddSpace(0, 5);

   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      S.TieTextBox(_("Command:"),mCommandName);
   }
   S.EndMultiColumn();
}

bool HelpCommand::Apply(const CommandContext & context){
   EffectManager & em = EffectManager::Get();
   PluginID ID = em.GetEffectByIdentifier( mCommandName );
   if( ID.IsEmpty() )
      context.Status( "Command not found" );
   else
      context.Status( em.GetCommandDefinition( ID ));
   return true;
}

