/**********************************************************************

  Audacity: A Digital Audio Editor

  LabelTrack.h

  Dominic Mazzoni
  James Crook
  Jun Wan

**********************************************************************/

#ifndef _LABELTRACK_
#define _LABELTRACK_

#include "ClientData.h"
#include "SelectedRegion.h"
#include "Track.h"


class wxTextFile;

class AudacityProject;
class DirManager;
class TimeWarper;

class LabelTrackView;

class LabelStruct : public ClientData::Site< LabelStruct >
{
public:
   using Caches = Site< LabelStruct >;

   LabelStruct() = default;
   // Copies region
   LabelStruct(const SelectedRegion& region, const wxString &aTitle);
   // Copies region but then overwrites other times
   LabelStruct(const SelectedRegion& region, double t0, double t1,
               const wxString &aTitle);
   const SelectedRegion &getSelectedRegion() const { return selectedRegion; }
   double getDuration() const { return selectedRegion.duration(); }
   double getT0() const { return selectedRegion.t0(); }
   double getT1() const { return selectedRegion.t1(); }

   struct BadFormatException {};
   static LabelStruct Import(wxTextFile &file, int &index);

   void Export(wxTextFile &file) const;

   /// Relationships between selection region and labels
   enum TimeRelations
   {
       BEFORE_LABEL,
       AFTER_LABEL,
       SURROUNDS_LABEL,
       WITHIN_LABEL,
       BEGINS_IN_LABEL,
       ENDS_IN_LABEL
   };

   /// Returns relationship between a region described and this label; if
   /// parent is set, it will consider point labels at the very beginning
   /// and end of parent to be within a region that borders them (this makes
   /// it possible to DELETE capture all labels with a Select All).
   TimeRelations RegionRelation(double reg_t0, double reg_t1,
                                const LabelTrack *parent = NULL) const;

public:
   SelectedRegion selectedRegion;
   wxString title; /// Text of the label.
};

using LabelArray = std::vector<LabelStruct>;

class AUDACITY_DLL_API LabelTrack final : public Track
{
 public:
   LabelTrack(const std::shared_ptr<DirManager> &projDirManager);
   LabelTrack(const LabelTrack &orig);

   virtual ~ LabelTrack();

   wxString GetDefaultName() const override;

   const LabelArray &GetLabels() const { return mLabels; }
   void SetLabel( size_t iLabel, const LabelStruct &newLabel );

   void SetOffset(double dOffset) override;

   double GetOffset() const override;
   double GetStartTime() const override;
   double GetEndTime() const override;

   using Holder = std::shared_ptr<LabelTrack>;
   
private:
   Track::Holder Clone() const override;

public:
   bool HandleXMLTag(const wxChar *tag, const wxChar **attrs) override;
   XMLTagHandler *HandleXMLChild(const wxChar *tag) override;
   void WriteXML(XMLWriter &xmlFile) const override;

#if LEGACY_PROJECT_FILE_SUPPORT
   bool Load(wxTextFile * in, DirManager * dirManager) override;
   bool Save(wxTextFile * out, bool overwrite) override;
#endif

   Track::Holder Cut  (double t0, double t1) override;
   Track::Holder Copy (double t0, double t1, bool forClipboard = true) const override;
   void Clear(double t0, double t1) override;
   void Paste(double t, const Track * src) override;
   bool Repeat(double t0, double t1, int n);

   void Silence(double t0, double t1) override;
   void InsertSilence(double t, double len) override;

   void Import(wxTextFile & f);
   void Export(wxTextFile & f) const;

   int GetNumLabels() const;
   const LabelStruct *GetLabel(int index) const;

   void OnLabelAdded( const wxString &title, int pos );
   //This returns the index of the label we just added.
   int AddLabel(const SelectedRegion &region, const wxString &title);

   //This deletes the label at given index.
   void DeleteLabel(int index);

   // This pastes labels without shifting existing ones
   bool PasteOver(double t, const Track *src);

   // PRL:  These functions were not used because they were not overrides!  Was that right?
   //Track::Holder SplitCut(double b, double e) /* not override */;
   //bool SplitDelete(double b, double e) /* not override */;

   void ShiftLabelsOnInsert(double length, double pt);
   void ChangeLabelsOnReverse(double b, double e);
   void ScaleLabels(double b, double e, double change);
   double AdjustTimeStampOnScale(double t, double b, double e, double change);
   void WarpLabels(const TimeWarper &warper);

   // Returns tab-separated text of all labels completely within given region
   wxString GetTextOfLabels(double t0, double t1) const;

   int FindNextLabel(const SelectedRegion& currentSelection);
   int FindPrevLabel(const SelectedRegion& currentSelection);

 public:
   void SortLabels();

 private:
   TrackKind GetKind() const override { return TrackKind::Label; }

   LabelArray mLabels;

   // Set in copied label tracks
   double mClipLen;

   int miLastLabel;                 // used by FindNextLabel and FindPrevLabel

private:
   std::shared_ptr<TrackView> DoGetView() override;
   std::shared_ptr<TrackControls> DoGetControls() override;
};

struct LabelTrackEvent : TrackListEvent
{
   explicit
   LabelTrackEvent(
      wxEventType commandType, const std::shared_ptr<LabelTrack> &pTrack,
      const wxString &title,
      int formerPosition,
      int presentPosition
   )
   : TrackListEvent{ commandType, pTrack }
   , mTitle{ title }
   , mFormerPosition{ formerPosition }
   , mPresentPosition{ presentPosition }
   {}

   LabelTrackEvent( const LabelTrackEvent& ) = default;
   wxEvent *Clone() const override {
      // wxWidgets will own the event object
      return safenew LabelTrackEvent(*this); }

   wxString mTitle;

   // invalid for addition event
   int mFormerPosition{ -1 };

   // invalid for deletion event
   int mPresentPosition{ -1 };
};

// Posted when a label is added.
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_LABELTRACK_ADDITION, LabelTrackEvent);

// Posted when a label is deleted.
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_LABELTRACK_DELETION, LabelTrackEvent);

// Posted when a label is repositioned in the sequence of labels.
wxDECLARE_EXPORTED_EVENT(AUDACITY_DLL_API,
                         EVT_LABELTRACK_PERMUTED, LabelTrackEvent);

struct LabelStructDisplay final : ClientData::Base
{
// Working storage for on-screen
   int width{}; /// width of the text in pixels.
   int x{};     /// Pixel position of left hand glyph
   int x1{};    /// Pixel position of right hand glyph
   int xText{}; /// Pixel position of left hand side of text box
   int y{};     /// Pixel position of label.
   
   /// flag to tell if the label times were updated
   bool updated{};

   static LabelStructDisplay &Get( const LabelStruct & );
};

#endif
