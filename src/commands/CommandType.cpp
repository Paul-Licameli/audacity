/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: wxwidgets

   Dan Horgan

******************************************************************//**

\file CommandType.cpp
\brief Contains definitions for CommandType class

\class OldStyleCommandType
\brief Base class for containing data common to all commands of a given type.
Also acts as a factory.

*//*******************************************************************/


#include "CommandType.h"

OldStyleCommandType::OldStyleCommandType()
   : mSymbol{}, mSignature{}
{ }

OldStyleCommandType::~OldStyleCommandType()
{
}

ComponentInterfaceSymbol OldStyleCommandType::GetSymbol()
{
   if (mSymbol.empty())
      mSymbol = BuildName();
   return mSymbol;
}

CommandSignature &OldStyleCommandType::GetSignature()
{
   if (!mSignature)
   {
      mSignature.emplace();
      BuildSignature(*mSignature);
   }
   return *mSignature;
}

wxString OldStyleCommandType::Describe()
{
   // PRL: Is this intended for end-user visibility or just debugging?  It did not
   // use _(""), so I assume it is meant to use internal strings
   auto desc = GetSymbol().Internal() + L"\nParameters:";
   GetSignature();
   ParamValueMap::iterator iter;
   ParamValueMap defaults = mSignature->GetDefaults();
   for (iter = defaults.begin(); iter != defaults.end(); ++iter)
   {
      desc += L"\n" + iter->first + L": "
         + mSignature->GetValidator(iter->first).GetDescription()
         + L" (default: "
         + iter->second.MakeString() + L")";
   }
   return desc;
}
