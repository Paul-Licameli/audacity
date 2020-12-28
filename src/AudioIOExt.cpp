/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 AudioIOExt.cpp
 
 Paul Licameli
 
 **********************************************************************/

#include "AudioIOExt.h"

auto AudioIOExt::GetFactories() -> Factories &
{
   static Factories factories;
   return factories;
}

AudioIOExt::RegisteredFactory::RegisteredFactory(Factory factory)
{
   GetFactories().push_back(std::move(factory));
}

AudioIOExt::RegisteredFactory::~RegisteredFactory()
{
   GetFactories().pop_back();
}

AudioIOExt::~AudioIOExt() = default;
