/**********************************************************************

Audacity: A Digital Audio Editor

ProjectSelectionManager.cpp

Paul Licameli split from ProjectManager.cpp

**********************************************************************/

#ifndef __AUDACITY_PROJECT_SELECTION_MANAGER__
#define __AUDACITY_PROJECT_SELECTION_MANAGER__

#include "audacity/Types.h"
#include "ClientData.h" // to inherit
#include "ComponentInterfaceSymbol.h"

class AudacityProject;

class AUDACITY_DLL_API ProjectSelectionManager final
   : public ClientData::Base
{
public:
   static ProjectSelectionManager &Get( AudacityProject &project );
   static const ProjectSelectionManager &Get( const AudacityProject &project );

   explicit ProjectSelectionManager( AudacityProject &project );
   ProjectSelectionManager( const ProjectSelectionManager & ) PROHIBITED;
   ProjectSelectionManager &operator=(
      const ProjectSelectionManager & ) PROHIBITED;
   ~ProjectSelectionManager() override;

   double AS_GetRate();
   void AS_SetRate(double rate);
   int AS_GetSnapTo();
   void AS_SetSnapTo(int snap);
   const NumericFormatSymbol & AS_GetSelectionFormat();
   void AS_SetSelectionFormat(const NumericFormatSymbol & format);

   const NumericFormatSymbol & TT_GetAudioTimeFormat();
   void TT_SetAudioTimeFormat(const NumericFormatSymbol & format);
   void AS_ModifySelection(double &start, double &end, bool done);


   double SSBL_GetRate() const;
   const NumericFormatSymbol & SSBL_GetFrequencySelectionFormatName();
   void SSBL_SetFrequencySelectionFormatName(
      const NumericFormatSymbol & formatName);
   const NumericFormatSymbol & SSBL_GetBandwidthSelectionFormatName();
   void SSBL_SetBandwidthSelectionFormatName(
      const NumericFormatSymbol & formatName);
   void SSBL_ModifySpectralSelection(
      double &bottom, double &top, bool done);

private:
   bool SnapSelection();

   AudacityProject &mProject;
};

#endif
