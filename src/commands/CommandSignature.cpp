/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: wxwidgets

   Dan Horgan

******************************************************************//**

\file CommandSignature.cpp
\brief Definitions for CommandSignature class

*//*******************************************************************/


#include "CommandSignature.h"

CommandSignature::CommandSignature() = default;

CommandSignature::~CommandSignature()
{
}

void CommandSignature::AddParameter(const wxString &name,
      const wxVariant &dft,
      std::unique_ptr<Validator> &&valid)
{
   wxASSERT_MSG(valid->Validate(dft),
         L"Invalid command signature: the default value of '"
         + dft.MakeString()
         + L"' for the '"
         + name
         + L"' parameter doesn't satisfy the provided validator."
         + L" It should be "
         + valid->GetDescription()
         + L".");
   mDefaults.insert(std::pair<wxString, wxVariant>(name, dft));
   mValidators.insert(ValidatorMap::value_type(name, std::move(valid)));
}

ParamValueMap CommandSignature::GetDefaults() const
{
   return mDefaults;
}

Validator &CommandSignature::GetValidator(const wxString &paramName)
{
   wxASSERT(mValidators.find(paramName) != mValidators.end());
   return *mValidators[paramName];
}
