/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMultiple.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EXPORT_MULTIPLE__
#define __AUDACITY_EXPORT_MULTIPLE__

#include "Export.h"
#include "../wxFileNameWrapper.h" // member variable

#include "../ShuttleGui.h" // remove this?

class wxButton;
class wxCheckBox;
class wxChoice;
class wxListEvent;
class wxSimplebook;
class wxStaticText;
class wxTextCtrl;

class AudacityProject;
class LabelTrack;
class SelectionState;
class ShuttleGui;
class Track;

class AUDACITY_DLL_API ExportMultipleDialog final : public wxDialogWrapper
{
public:

   ExportMultipleDialog(AudacityProject *parent);
   virtual ~ExportMultipleDialog();

   int ShowModal();

private:

   // Export
   void CanExport();
   void CountTracksAndLabels();
   bool DirOk();
   /** \brief Export multiple labeled regions of the project to separate files
    *
    * Uses a single label track in the project to split up the audio into a
    * series of sections, each of which is exported to a separate file.
    * @param byName Controls whether files are named after the text in the
    * labels that define them (true), or just numbered (false).
    * @param prefix The string used to prefix the file number if files are being
    * numbered rather than named */
   ProgressResult ExportMultipleByLabel(bool byName, const wxString &prefix, bool addNumber);

   /** \brief Export each track in the project to a separate file
    *
    * @param byName Controls whether files are named after the track names
    * (true), or just numbered (false).
    * @param prefix The string used to prefix the file number if files are being
    * numbered rather than named */
   ProgressResult ExportMultipleByTrack(bool byName, const wxString &prefix, bool addNumber);

   /** Export one file of an export multiple set
    *
    * Called once for each file in the list to do a (non-interactive) export
    * @param channels Number of channels to export
    * @param name The file name (and path) to export to
    * @param selectedOnly Should we  export the selected tracks only?
    * @param t0 Start time for export
    * @param t1 End time for export
    * @param tags Metadata to include in the file (if possible).
    */
   ProgressResult DoExport(std::unique_ptr<ProgressDialog> &pDialog,
                 unsigned channels,
                 const wxFileName &name,
                 bool selectedOnly,
                 double t0,
                 double t1,
                 const Tags &tags);
   /** \brief Takes an arbitrary text string and converts it to a form that can
    * be used as a file name, if necessary prompting the user to edit the file
    * name produced */
   wxString MakeFileName(const wxString &input);
   // Dialog
   void PopulateOrExchange(ShuttleGui& S);

   void OnOptions();
   void OnCreate();
   void OnChoose();
   void OnCancel();
   void OnHelp();
   void OnExport();

private:
   Exporter mExporter;
   std::vector<ExportPlugin*> mPlugins;   /**< Array of references to available exporter
                                   plug-ins */
   AudacityProject *mProject;
   TrackList *mTracks;           /**< The list of tracks in the project that is
                                   being exported */
   const LabelTrack *mLabels;
   int mNumLabels;
   int mNumWaveTracks;

   int mFilterIndex;          /**< The index in the drop-down list of export
                                formats (mFormat) of the selected export format.
                                This list includes all possible
                                plug-in - subformat combinations. */
   int mPluginIndex;          /**< The index in mPlugins of the selected export
                              plug-in */
   int mSubFormatIndex;       /**< The selected subformat number within the
                                selected export plug-in set by mPluginIndex */

   // List of file actually exported
   FilePaths mExported;

   wxButton      *mOptions;

   wxTextCtrl    *mDir;    /**< The directory all the exported files will end
                             up in */
   wxButton      *mCreate;
   wxButton      *mChoose;

   wxStaticText  *mLabelLabel;   /**< description text for mLabel */

   wxCheckBox    *mFirst;  /**< Check box to export audio before first label */
   wxTextCtrl    *mFirstFileName;   /**< Name to use for exporting audio before
                                      the first label in the file */

   wxStaticText  *mTrackLabel;

   wxTextCtrl    *mPrefix;

   wxButton      *mCancel;

   SelectionState &mSelectionState;

   std::shared_ptr< DialogDefinition::Adaptor< int > > mpTarget;

   int mPreferByLabels;
};

class SuccessDialog final : public wxDialogWrapper
{
public:
   SuccessDialog(wxWindow *parent, wxWindowID id, const TranslatableString &title) :
      wxDialogWrapper(parent, id, title, wxDefaultPosition,
         wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {};
   void OnKeyDown(wxListEvent& event); // dismisses dialog when <enter> is pressed with list control having focus
   void OnItemActivated(wxListEvent& event); // dismisses dialog when <enter> is pressed with list item having focus
private:
   DECLARE_EVENT_TABLE()
};

class MouseEvtHandler final : public wxEvtHandler
{
public:
   void OnMouse(wxMouseEvent& event);
private:
   DECLARE_EVENT_TABLE()
};


#endif
