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
   : public wxEvtHandler
   , public ClientData::Base
{
public:
   static ProjectSelectionManager &Get( AudacityProject &project );
   static const ProjectSelectionManager &Get( const AudacityProject &project );

   explicit ProjectSelectionManager( AudacityProject &project );
   ProjectSelectionManager( const ProjectSelectionManager & ) PROHIBITED;
   ProjectSelectionManager &operator=(
      const ProjectSelectionManager & ) PROHIBITED;
   ~ProjectSelectionManager() override;

private:
   void OnSettingsChanged(wxCommandEvent &evt);

public:
   void AS_SetSnapTo(int snap);

   void AS_SetSelectionFormat(const NumericFormatSymbol & format);

   void TT_SetAudioTimeFormat(const NumericFormatSymbol & format);
   void AS_ModifySelection(double &start, double &end, bool done);

   void SSBL_SetFrequencySelectionFormatName(
      const NumericFormatSymbol & formatName);
   void SSBL_SetBandwidthSelectionFormatName(
      const NumericFormatSymbol & formatName);
   void SSBL_ModifySpectralSelection(
      double &bottom, double &top, bool done);

private:
   bool SnapSelection();

   AudacityProject &mProject;
};

#endif
