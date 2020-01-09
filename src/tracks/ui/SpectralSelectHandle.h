/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 SpectralSelectHandle.h
 
 Paul Licameli split from SelectHandle.h
 
 **********************************************************************/

#ifndef __AUDACITY__SPECTRAL_SELECT_HANDLE__
#define __AUDACITY__SPECTRAL_SELECT_HANDLE__

#include "SelectHandle.h" // to inherit

class SpectralSelectHandle : public SelectHandle
{
public:
   using SelectHandle::SelectHandle;
   ~SpectralSelectHandle() override;
};

#endif
