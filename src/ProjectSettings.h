/**********************************************************************

Audacity: A Digital Audio Editor

ProjectSettings.h

Paul Licameli split from AudacityProject.h

**********************************************************************/

#ifndef __AUDACITY_PROJECT_SETTINGS__
#define __AUDACITY_PROJECT_SETTINGS__

#include <atomic>

#include "Project.h"
#include "Prefs.h" // to inherit
#include "audacity/Types.h"

class AudacityProject;

// Sent to the project when certain settings change
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
   EVT_PROJECT_SETTINGS_CHANGE, wxCommandEvent);

enum
{
   SNAP_OFF,
   SNAP_NEAREST,
   SNAP_PRIOR
};

namespace ToolCodes {
enum {
   selectTool,
   envelopeTool,
   drawTool,
   zoomTool,
   slideTool,
   multiTool,
   numTools,
   
   firstTool = selectTool,
   lastTool = multiTool,
};
}

///\brief Holds various per-project settings values,
/// and sends events to the project when certain values change
class AUDACITY_DLL_API ProjectSettings final
   : public AttachedProjectObject
   , private PrefsListener
{
public:
   static ChoiceSetting TracksBehaviorsSolo;

   static ProjectSettings &Get( AudacityProject &project );
   static const ProjectSettings &Get( const AudacityProject &project );
   
   // Values retrievable from GetInt() of the event for settings change
   enum EventCode : int {
      ChangedSnapTo,
      ChangedSelectionFormat,
      ChangedAudioTimeFormat,
      ChangedFrequencyFormat,
      ChangedBandwidthFormat,
   };

   explicit ProjectSettings( AudacityProject &project );
   ProjectSettings( const ProjectSettings & ) PROHIBITED;
   ProjectSettings &operator=( const ProjectSettings & ) PROHIBITED;


   bool GetTracksFitVerticallyZoomed() const { return mTracksFitVerticallyZoomed; } //lda
   void SetTracksFitVerticallyZoomed(bool flag) { mTracksFitVerticallyZoomed = flag; } //lda

   // Snap To

   void SetSnapTo(int snap);
   int GetSnapTo() const;

   // Current tool

   void SetTool(int tool) { mCurrentTool = tool; }
   int GetTool() const { return mCurrentTool; }

   // Speed play
   double GetPlaySpeed() const {
      return mPlaySpeed.load( std::memory_order_relaxed ); }
   void SetPlaySpeed( double value ) {
      mPlaySpeed.store( value, std::memory_order_relaxed ); }

   // Selection Format
   void SetSelectionFormat(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetSelectionFormat() const;

   // AudioTime format
   void SetAudioTimeFormat(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetAudioTimeFormat() const;

   // Spectral Selection Formats
   void SetFrequencySelectionFormatName(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetFrequencySelectionFormatName() const;

   void SetBandwidthSelectionFormatName(const NumericFormatSymbol & format);
   const NumericFormatSymbol & GetBandwidthSelectionFormatName() const;

   bool IsSoloSimple() const { return mSoloPref == wxT("Simple"); }
   bool IsSoloNone() const { return mSoloPref == wxT("None"); }

   bool EmptyCanBeDirty() const { return mEmptyCanBeDirty; }

   bool GetShowSplashScreen() const { return mShowSplashScreen; }

private:
   void UpdatePrefs() override;

   AudacityProject &mProject;

   NumericFormatSymbol mSelectionFormat;
   NumericFormatSymbol mFrequencySelectionFormatName;
   NumericFormatSymbol mBandwidthSelectionFormatName;
   NumericFormatSymbol mAudioTimeFormat;

   wxString mSoloPref;

   // This is atomic because scrubber may read it in a separate thread from
   // the main
   std::atomic<double> mPlaySpeed{};

   int mSnapTo;

   int mCurrentTool;
   
   bool mTracksFitVerticallyZoomed{ false };  //lda
   bool mEmptyCanBeDirty;
   bool mShowSplashScreen;
};

#endif
