/**********************************************************************

  Audacity: A Digital Audio Editor

  ShuttleGui.h

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************/

#ifndef SHUTTLE_GUI
#define SHUTTLE_GUI


#include "Identifier.h"

#include <initializer_list>
#include <functional>
#include <vector>
#include <wx/slider.h> // to inherit
#include "MemoryX.h"
#include <wx/listbase.h> // for wxLIST_FORMAT_LEFT
#include <wx/weakref.h>

#include "Prefs.h"
#include "widgets/NumericTextCtrl.h"
#include "ShuttlePrefs.h"
#include "WrappedType.h"

class ChoiceSetting;
class LabelSetting;

class wxArrayStringEx;

const int nMaxNestedSizers = 20;

enum teShuttleMode
{
   eIsCreating,
   eIsGettingFromDialog,
   eIsSettingToDialog,

   // Next two are only ever seen in constructor.
   // After that they revert to one of the modes above.
   // They are used to achieve 'two step' operation,
   // where we transfer between two shuttles in one go.
   eIsCreatingFromPrefs,
   eIsSavingToPrefs
};

class wxListCtrl;
class wxCheckBox;
class wxChoice;
class wxComboBox;
class wxScrolledWindow;
class wxStaticText;
class wxTreeCtrl;
class wxTextCtrl;
class wxSlider;
class wxNotebook;
class wxSimplebook;
typedef wxWindow wxNotebookPage;  // so far, any window can be a page
class wxButton;
class wxBitmapButton;
class wxRadioButton;
class wxBitmap;
class wxPanel;
class wxSizer;
class wxSizerItem;
class wxStaticBox;
class wxSpinCtrl;
class wxListBox;
class wxGrid;
class Shuttle;
class ReadOnlyText;

class WrappedType;

#ifdef __WXMAC__

#include <wx/statbox.h> // to inherit

class wxStaticBoxWrapper
   : public wxStaticBox // inherit to get access to m_container
{
public:
   template< typename... Args >
   wxStaticBoxWrapper( Args &&...args )
      : wxStaticBox( std::forward<Args>(args)... )
   {
      m_container.EnableSelfFocus();
   }
};

/// Fix a defect in TAB key navigation to sliders, known to happen in wxWidgets
/// 3.1.1 and maybe in earlier versions
class wxSliderWrapper : public wxSlider
{
public:
   using wxSlider::wxSlider;
   void SetFocus() override;
};
#else
using wxStaticBoxWrapper = wxStaticBox;
using wxSliderWrapper = wxSlider;
#endif

// CreateStdButtonSizer defs...should probably move to widgets subdir
enum StandardButtonID : unsigned
{
   eButtonUndefined = 0,
   eOkButton      = 0x0001,
   eCancelButton  = 0x0002,
   eYesButton     = 0x0004,
   eNoButton      = 0x0008,
   eHelpButton    = 0x0010,
   ePreviewButton = 0x0020,
   eDebugButton   = 0x0040,
   eSettingsButton= 0x0080,
   ePreviewDryButton  = 0x0100,
   eApplyButton   = 0x0200,
   eCloseButton   = 0x0400,
};

namespace DialogDefinition {
  
struct ControlText{

   ControlText() = default;

   // The usual, implicit constructor
   ControlText( const TranslatableString &name )
      : mName{ name }
   {}

   // More elaborate cases
   ControlText( const TranslatableString &name,
      const TranslatableString &suffix,
      const TranslatableString &toolTip = {},
      const TranslatableString &label = {} )
      : mName{ name }
      , mSuffix{ suffix }
      , mToolTip{ toolTip }
      , mLabel{ mLabel }
   {}

   // Menu codes in the translation will be stripped
   TranslatableString mName;

   // Append a space, then the translation of the given string, to control name
   // (not the title or label:  this affects the screen reader behavior)
   TranslatableString mSuffix;

   TranslatableString mToolTip;

   // If not empty, then this is the visible label, distinct from the audible
   // mName which is for the screen reader
   TranslatableString mLabel;
};

// Convenience for a frequent case specifying label only
inline ControlText Label( const TranslatableString &label )
{
   return { {}, {}, {}, label };
}

struct BaseItem
{
   using ActionType = std::function< void() >;
   using Test = std::function< bool() >;

   BaseItem() = default;

   explicit BaseItem(StandardButtonID id)
   {
      mStandardButton = id;
   }

   std::function< void(wxWindow*) > mValidatorSetter;
   Test mEnableTest;
   Test mShowTest;
   ControlText mText;

   std::vector<std::pair<wxEventType, wxObjectEventFunction>> mRootConnections;


   long miStyle{};

   // Applies to windows, not to subsizers
   int mWindowPositionFlags{ 0 };

   StandardButtonID mStandardButton{ eButtonUndefined };

   wxSize mWindowSize = wxDefaultSize;

   wxSize mMinSize{ -1, -1 };
   bool mHasMinSize{ false };
   bool mUseBestSize{ false };

   bool mFocused { false };
   bool mDefault { false };
   bool mDisabled { false };

   mutable wxEventTypeTag<wxCommandEvent> mEventType{ 0 };
   ActionType mAction{};
};

template< typename Sink = wxEvtHandler >
struct TypedItem : BaseItem {
   TypedItem() = default;
   TypedItem ( StandardButtonID id )
   {
      mStandardButton = id;
   }

   // Alternative chain-call but you can also just use the constructor
   // This has effect only for items passed to AddStandardButtons
   TypedItem && StandardButton( StandardButtonID id ) &&
   {
      mStandardButton = id;
      return std::move( *this );
   }

   // Factory is a class that returns a value of some subclass of wxValidator
   // We must wrap it in another lambda to allow the return type of f to
   // vary, and avoid the "slicing" problem.
   // (That is, std::function<wxValidator()> would not work.)
   template<typename Factory>
   TypedItem&& Validator( const Factory &f ) &&
   {
      mValidatorSetter = [f](wxWindow *p){ p->SetValidator(f()); };
      return std::move(*this);
   }

   // This allows further abbreviation of the previous:
   template<typename V, typename... Args>
   TypedItem&& Validator( Args&&... args ) &&
   { return std::move(*this).Validator( [args...]{ return V( args... ); } ); }

   TypedItem&& Text( const ControlText &text ) &&
   {
      mText = text;
      return std::move( *this );
   }

   TypedItem&& Style( long style ) &&
   {
      miStyle = style;
      return std::move( *this );
   }

   // Only the last item specified as focused (if more than one) will be
   TypedItem&& Focus( bool focused = true ) &&
   {
      mFocused = focused;
      return std::move( *this );
   }

   // For buttons only
   // Only the last item specified as default (if more than one) will be
   TypedItem&& Default( bool isdefault = true ) &&
   {
      mDefault = isdefault;
      return std::move( *this );
   }

   // Just sets the state of the item once
   TypedItem&& Disable( bool disabled = true ) &&
   {
      mDisabled = disabled;
      return std::move( *this );
   }

   // Specifies a predicate that is retested as needed
   TypedItem&& Enable( const Test &test ) &&
   {
      mEnableTest = test;
      return std::move( *this );
   }

   // Specifies a predicate that is retested as needed
   TypedItem&& Show( const Test &test ) &&
   {
      mShowTest = test;
      return std::move( *this );
   }

   // Dispatch events from the control to the dialog
   // The template type deduction ensures consistency between the argument type
   // and the event type.  It does not (yet) ensure correctness of the type of
   // the handler object.
   template< typename Tag, typename Argument, typename Handler >
   auto ConnectRoot(
      wxEventTypeTag<Tag> eventType,
      void (Handler::*func)(Argument&)
   ) &&
        -> typename std::enable_if<
            std::is_base_of<Argument, Tag>::value,
            TypedItem&&
        >::type
   {
      mRootConnections.push_back({
         eventType,
         (void(wxEvtHandler::*)(wxEvent&)) (
            static_cast<void(wxEvtHandler::*)(Argument&)>( func )
         )
      });
      return std::move( *this );
   }

   TypedItem&& MinSize() && // set best size as min size
   {
      mUseBestSize = true;
      return std::move ( *this );
   }

   TypedItem&& MinSize( wxSize sz ) &&
   {
      mMinSize = sz; mHasMinSize = true;
      return std::move ( *this );
   }

   TypedItem&& Position( int flags ) &&
   {
      mWindowPositionFlags = flags;
      return std::move( *this );
   }

   TypedItem&& Size( wxSize size ) &&
   {
      mWindowSize = size;
      return std::move( *this );
   }

   // Supply an event handler for a control; this is an alternative to using
   // the event table macros, and removes the need to specify an id for the
   // control.
   // The event sink defaults to the dialog itself, or to what was last given
   // to ShuttleGui::SetDefaultSink
   // If the control has a validator, the function body can assume the value was
   // validated and transferred from the control successfully
   // After the body, call the dialog's TransferDataToWindow()
   // The event type and id to bind are inferred from the control
   TypedItem &&Action(
      // A pointer to nullary member function of class Sink can be given to
      // this argument:
      const ActionType &action  ) &&
   {
      return std::move( *this ).Action( action,
         wxEventTypeTag<wxCommandEvent>(0)  );
   }

   // An overload for cases that there is more than one useful event type
   // But it must specify some type whose event class is wxCommandEvent
   TypedItem &&Action(
      // A pointer to nullary member function of class Sink can be given to
      // this argument:
      const ActionType &action,
      wxEventTypeTag<wxCommandEvent> type ) &&
   {
      mEventType = type;
      mAction = action;
      return std::move( *this );
   }
};

using Item = TypedItem<>;

using Items = std::initializer_list< BaseItem >;

}

class PreferenceVisitor {
public:
   virtual ~PreferenceVisitor();
   
   virtual void Visit(
      const wxString &Prompt,
      const Setting<bool> &Setting) = 0;
   
   virtual void Visit(
      const wxString &Prompt,
      const Setting<int> &Setting) = 0;
   
   virtual void Visit(
      const wxString &Prompt,
      const Setting<double> &Setting) = 0;
   
   virtual void Visit(
      const wxString &Prompt,
      const Setting<wxString> &Setting) = 0;
   
   virtual void Visit(
      const wxString &Prompt,
      const ChoiceSetting &Setting) = 0;
};

struct ShuttleGuiState final
{
   ShuttleGuiState(
      wxWindow *pDlg, teShuttleMode ShuttleMode, bool vertical, wxSize minSize,
      const std::shared_ptr< PreferenceVisitor > &pVisitor );

   void PushSizer();
   void PopSizer();

   wxWindow *const mpDlg;
   const teShuttleMode mShuttleMode;

   std::unique_ptr<Shuttle> mpShuttle; /*! Controls source/destination of shuttled data.  You can
   leave this NULL if you are shuttling to variables */

   std::shared_ptr< PreferenceVisitor > mpVisitor;

   wxSizer * mpSizer = nullptr;

   int miBorder = 5;

   wxSizer * pSizerStack[ nMaxNestedSizers ];
   int mSizerDepth = -1;

   int miIdNext = 3000;

   wxWindow * mpParent = mpDlg;

   using RadioButtonList = std::vector< wxWindowRef >;
   std::shared_ptr< RadioButtonList > mRadioButtons;
};

class AUDACITY_DLL_API ShuttleGuiBase /* not final */
{
public:
   ShuttleGuiBase(
      wxWindow * pParent,
      teShuttleMode ShuttleMode,
      bool vertical, // Choose layout direction of topmost level sizer
      wxSize minSize,
      const std::shared_ptr< PreferenceVisitor > &pVisitor
   );
   ShuttleGuiBase( const std::shared_ptr< ShuttleGuiState > &pState );
   ShuttleGuiBase( ShuttleGuiBase&& );
   virtual ~ShuttleGuiBase();
   void ResetId();

//-- Add functions.  These only add a widget or 2.
   void HandleOptionality(const TranslatableLabel &Prompt);
   void AddPrompt(const TranslatableLabel &Prompt, int wrapWidth = 0);
   void AddUnits(const TranslatableString &Units, int wrapWidth = 0);
   void AddTitle(const TranslatableString &Title, int wrapWidth = 0);
   wxWindow * AddWindow(wxWindow * pWindow);
   wxSlider * AddSlider(
      const TranslatableLabel &Prompt, int pos, int Max, int Min = 0,
      // Pass -1 to mean unspecified:
      int lineSize = -1, int pageSize = -1 );
   wxSlider * AddVSlider(const TranslatableLabel &Prompt, int pos, int Max);
   wxSpinCtrl * AddSpinCtrl(const TranslatableLabel &Prompt,
      int Value, int Max, int Min);
   wxTreeCtrl * AddTree();

   // Spoken name of the button defaults to the same as the prompt
   // (after stripping menu codes):
   wxRadioButton * AddRadioButton( const TranslatableLabel & Prompt );

   // Only the last button specified as default (if more than one) will be
   // Always ORs the flags with wxALL (which affects borders):
   wxButton * AddButton(
      const TranslatableLabel & Label, int PositionFlags = wxALIGN_CENTRE,
      bool setDefault = false );
   // Only the last button specified as default (if more than one) will be
   // Always ORs the flags with wxALL (which affects borders):
   wxBitmapButton * AddBitmapButton(
      const wxBitmap &Bitmap, int PositionFlags = wxALIGN_CENTRE,
      bool setDefault = false );
   // When PositionFlags is 0, applies wxALL (which affects borders),
   // and either wxALIGN_CENTER (if bCenter) or else wxEXPAND
   wxStaticText * AddVariableText(
      const TranslatableString &Str, bool bCenter = false,
      int PositionFlags = 0, int wrapWidth = 0);
   ReadOnlyText * AddReadOnlyText(
      const TranslatableLabel &Caption,
      const wxString &Value);
   wxTextCtrl * AddTextBox(
      const TranslatableLabel &Prompt,
      const wxString &Value, const int nChars);
   wxTextCtrl * AddNumericTextBox(
      const TranslatableLabel &Prompt,
      const wxString &Value, const int nChars);
   wxTextCtrl * AddTextWindow(const wxString &Value);
   wxListBox * AddListBox(const wxArrayStringEx &choices);

   struct ListControlColumn{
      ListControlColumn(
         const TranslatableString &h,
         int f = wxLIST_FORMAT_LEFT, int w = wxLIST_AUTOSIZE)
         : heading(h), format(f), width(w)
      {}

      TranslatableString heading;
      int format;
      int width;
   };
   wxListCtrl * AddListControl(
      std::initializer_list<const ListControlColumn> columns = {},
      long listControlStyles = 0
   );
   wxListCtrl * AddListControlReportMode(
      std::initializer_list<const ListControlColumn> columns = {},
      long listControlStyles = 0
   );

   wxGrid * AddGrid();

   NumericTextCtrl * AddNumericTextCtrl(NumericConverter::Type type,
         const NumericFormatSymbol &formatName = {},
         double value = 0.0,
         double sampleRate = 44100,
         const NumericTextCtrl::Options &options = {},
         const wxPoint &pos = wxDefaultPosition,
         const wxSize &size = wxDefaultSize);

   wxCheckBox * AddCheckBox( const TranslatableLabel &Prompt, bool Selected);
   wxCheckBox * AddCheckBoxOnRight( const TranslatableLabel &Prompt, bool Selected);

   // These deleted overloads are meant to break compilation of old calls that
   // passed literal "true" and "false" strings
   wxCheckBox * AddCheckBox( const TranslatableLabel &Prompt, const wxChar *) = delete;
   wxCheckBox * AddCheckBox( const TranslatableLabel &Prompt, const char *) = delete;
   wxCheckBox * AddCheckBoxOnRight( const TranslatableLabel &Prompt, const wxChar *) = delete;
   wxCheckBox * AddCheckBoxOnRight( const TranslatableLabel &Prompt, const char *) = delete;

   wxComboBox * AddCombo( const TranslatableLabel &Prompt,
      const wxString &Selected, const wxArrayStringEx & choices );
   wxChoice   * AddChoice( const TranslatableLabel &Prompt,
      const TranslatableStrings &choices, int Selected = -1 );
   wxChoice   * AddChoice( const TranslatableLabel &Prompt,
      const TranslatableStrings &choices, const TranslatableString &selected );
   void AddIcon( wxBitmap * pBmp);
   void AddFixedText(
      const TranslatableString & Str, bool bCenter = false, int wrapWidth = 0 );
   void AddConstTextBox(
      const TranslatableString &Caption, const TranslatableString & Value );

//-- Start and end functions.  These are used for sizer, or other window containers
//   and create the appropriate widget.
   void StartHorizontalLay(int PositionFlags=wxALIGN_CENTRE, int iProp=1);
   void EndHorizontalLay();
   void StartVerticalLay(int iProp=1);
   void StartVerticalLay(int PositionFlags, int iProp);

   void EndVerticalLay();
   wxScrolledWindow * StartScroller(int iStyle=0);
   void EndScroller();
   wxPanel * StartPanel(int iStyle=0);
   void EndPanel();
   void StartMultiColumn(int nCols, int PositionFlags=wxALIGN_LEFT);
   void EndMultiColumn();

   void StartTwoColumn() {StartMultiColumn(2);};
   void EndTwoColumn() {EndMultiColumn();};
   void StartThreeColumn(){StartMultiColumn(3);};
   void EndThreeColumn(){EndMultiColumn();};

   wxStaticBox * StartStatic( const TranslatableString & Str, int iProp=0 );
   void EndStatic();

   wxNotebook * StartNotebook();
   void EndNotebook();

   wxSimplebook * StartSimplebook();
   void EndSimplebook();

   // Use within any kind of book control:
   // IDs of notebook pages cannot be chosen by the caller
   wxNotebookPage * StartNotebookPage( const TranslatableString & Name );

   void EndNotebookPage();

   wxPanel * StartInvisiblePanel();
   void EndInvisiblePanel();

   // Introduce a sequence of calls to AddRadioButton.
   void StartRadioButtonGroup();
   // SettingName is a key in Preferences.
   void StartRadioButtonGroup( const LabelSetting &Setting );

   // End a sequence of calls to AddRadioButton.
   void EndRadioButtonGroup();

   bool DoStep( int iStep );
   int TranslateToIndex( const wxString &Value, const Identifiers &Choices );
   Identifier TranslateFromIndex( const int nIn, const Identifiers &Choices );

//-- Tie functions both add controls and also read/write to them.

   wxTextCtrl * TieTextBox(
      const TranslatableLabel &Prompt, wxString & Value, const int nChars=0);
   wxTextCtrl * TieTextBox(
      const TranslatableLabel &Prompt, int &Selected, const int nChars=0);
   wxTextCtrl * TieTextBox(
      const TranslatableLabel &Prompt, double &Value, const int nChars=0);

   wxTextCtrl * TieNumericTextBox( const TranslatableLabel &Prompt, int &Value, const int nChars=0);
   wxTextCtrl * TieNumericTextBox( const TranslatableLabel &Prompt, double &Value, const int nChars=0);

   wxCheckBox * TieCheckBox( const TranslatableLabel &Prompt, bool & Var );
   wxCheckBox * TieCheckBoxOnRight( const TranslatableLabel & Prompt, bool & Var );

   wxChoice * TieChoice(
      const TranslatableLabel &Prompt,
      TranslatableString &Selected, const TranslatableStrings &choices );
   wxChoice * TieChoice(
      const TranslatableLabel &Prompt, int &Selected, const TranslatableStrings &choices );

   wxSlider * TieSlider(
      const TranslatableLabel &Prompt,
      int &pos, const int max, const int min = 0);
   wxSlider * TieSlider(
      const TranslatableLabel &Prompt,
      double &pos, const double max, const double min = 0.0);
   wxSlider * TieSlider(
      const TranslatableLabel &Prompt,
      float &pos, const float fMin, const float fMax);
   wxSlider * TieVSlider(
      const TranslatableLabel &Prompt,
      float &pos, const float fMin, const float fMax);

   // Must be called between a StartRadioButtonGroup / EndRadioButtonGroup pair,
   // and as many times as there are values in the enumeration.
   wxRadioButton * TieRadioButton();

   wxSpinCtrl * TieSpinCtrl( const TranslatableLabel &Prompt,
      int &Value, const int max, const int min = 0 );


//-- Variants of the standard Tie functions which do two step exchange in one go
// Note that unlike the other Tie functions, ALL the arguments are const.
// That's because the data is being exchanged between the dialog and mpShuttle
// so it doesn't need an argument that is writeable.
   wxCheckBox * TieCheckBox(
      const TranslatableLabel &Prompt,
      const BoolSetting &Setting);
   wxCheckBox * TieCheckBoxOnRight(
      const TranslatableLabel &Prompt,
      const BoolSetting &Setting);

   wxChoice *TieChoice(
      const TranslatableLabel &Prompt,
      const ChoiceSetting &choiceSetting );

   // This overload presents what is really a numerical setting as a choice among
   // commonly used values, but the choice is not necessarily exhaustive.
   // This behaves just like the previous for building dialogs, but the
   // behavior is different when the call is intercepted for purposes of
   // emitting scripting information about Preferences.
  wxChoice * TieNumberAsChoice(
      const TranslatableLabel &Prompt,
      const IntSetting &Setting,
      const TranslatableStrings & Choices,
      const std::vector<int> * pInternalChoices = nullptr,
      int iNoMatchSelector = 0 );

   wxTextCtrl * TieTextBox(
      const TranslatableLabel &Prompt,
      const StringSetting &Setting,
      const int nChars);
   wxTextCtrl * TieIntegerTextBox(
      const TranslatableLabel & Prompt,
      const IntSetting &Setting,
      const int nChars);
   wxTextCtrl * TieNumericTextBox(
      const TranslatableLabel & Prompt,
      const DoubleSetting &Setting,
      const int nChars);
   wxSlider * TieSlider(
      const TranslatableLabel & Prompt,
      const IntSetting &Setting,
      const int max,
      const int min = 0);
   wxSpinCtrl * TieSpinCtrl(
      const TranslatableLabel &Prompt,
      const IntSetting &Setting,
      const int max,
      const int min);
//-- End of variants.
   void SetBorder( int Border ) {mpState -> miBorder = Border;};
   void SetSizerProportion( int iProp ) {miSizerProp = iProp;};
   void SetStretchyCol( int i );
   void SetStretchyRow( int i );

//--Some Additions since June 2007 that don't fit in elsewhere...
   wxWindow * GetParent()
   {
      // This assertion justifies the use of safenew in many places where GetParent()
      // is used to construct a window
      wxASSERT(mpState -> mpParent != NULL);
      return mpState -> mpParent;
   }
   ShuttleGuiBase & Prop( int iProp );
   void UseUpId();

   wxSizer * GetSizer() {return mpState -> mpSizer;}

   static void CheckEventType(
      const DialogDefinition::BaseItem &item,
      std::initializer_list<wxEventType> types );

   static void ApplyItem( int step, const DialogDefinition::BaseItem &item,
      wxWindow *pWind, wxWindow *pDlg );

   // If none of the items is default,
   // then the first of these buttons, if any, that is included will be default:
   // Apply, Yes, OK
   // The buttons shown are the union of those given simply as a bit flag,
   // or else specified as an Item with its StandardButton specified.
   // The positioning of the items is determined in the function, independently
   // of the sequence of the items
   void AddStandardButtons(
      long buttons = eOkButton | eCancelButton,
      DialogDefinition::Items items = {},
      wxWindow *extra = nullptr,
      DialogDefinition::Item extraItem = {});

   wxSizerItem * AddSpace( int width, int height, int prop = 0 );
   wxSizerItem * AddSpace( int size ) { return AddSpace( size, size ); };

   // Calculate width of a choice control adequate for the items, maybe after
   // the dialog is created but the items change.
   static void SetMinSize( wxWindow *window, const TranslatableStrings & items );
   static void SetMinSize( wxWindow *window, const wxArrayStringEx & items );
  // static void SetMinSize( wxWindow *window, const std::vector<int> & items );

protected:
   void SetProportions( int Default );

   void UpdateSizersCore( bool bPrepend, int Flags, bool prompt = false );
   void UpdateSizers();
   void UpdateSizersC();
   void UpdateSizersAtStart();

   long GetStyle( long Style );

private:
   void DoInsertListColumns(
      wxListCtrl *pListCtrl,
      long listControlStyles,
      std::initializer_list<const ListControlColumn> columns );

protected:
   std::shared_ptr< ShuttleGuiState > mpState;

   int miNoMatchSelector = 0; //! Used in choices to determine which item to use on no match.

   int miSizerProp = 0;
   int miProp = 0;

   // See UseUpId() for explanation of these two.
   int miId = -1;
   int miIdSetByUser = -1;

   // Proportion set by user rather than default.
   int miPropSetByUser = -1;

   bool * mpbOptionalFlag = nullptr;

   std::unique_ptr<wxSizer> mpSubSizer;
   wxWindow * mpWind = nullptr;

private:
   void DoDataShuttle( const wxString &Name, WrappedType & WrappedRef );
   wxChoice *DoTieChoice(
      const TranslatableLabel &Prompt,
      const ChoiceSetting &choiceSetting );
   wxCheckBox * DoTieCheckBoxOnRight( const TranslatableLabel & Prompt, WrappedType & WrappedRef );
   wxTextCtrl * DoTieTextBox(
      const TranslatableLabel &Prompt,
      WrappedType &  WrappedRef, const int nChars);
   wxTextCtrl * DoTieNumericTextBox(
      const TranslatableLabel &Prompt, WrappedType &  WrappedRef, const int nChars);
   wxCheckBox * DoTieCheckBox(
      const TranslatableLabel &Prompt, WrappedType & WrappedRef );
   wxSlider * DoTieSlider(
      const TranslatableLabel &Prompt,
      WrappedType & WrappedRef, const int max, const int min = 0 );
   wxSpinCtrl * DoTieSpinCtrl( const TranslatableLabel &Prompt,
      WrappedType & WrappedRef, const int max, const int min = 0 );

   TranslatableLabels mRadioLabels;
   Identifiers mRadioValues;
   wxString mRadioSettingName; /// The setting controlled by a group.
   Optional<WrappedType> mRadioValue;  /// The wrapped value associated with the active radio button.
   int mRadioCount = -1;       /// The index of this radio item.  -1 for none.
   wxString mRadioValueString; /// Unwrapped string value.
   wxRadioButton *DoAddRadioButton(
      const TranslatableLabel &Prompt, int style );

protected:
   DialogDefinition::BaseItem mItem;
};

// A rarely used helper function that sets a pointer
// ONLY if the value it is to be set to is non NULL.
extern void SetIfCreated( wxChoice *&Var, wxChoice * Val );
extern void SetIfCreated( wxTextCtrl *&Var, wxTextCtrl * Val );
extern void SetIfCreated( wxStaticText *&Var, wxStaticText * Val );

class GuiWaveTrack;
class AttachableScrollBar;
class ViewInfo;

#include <wx/defs.h>  // to get wxSB_HORIZONTAL

enum
{
   // ePreviewID     = wxID_LOWEST - 1,
   // But there is a wxID_PREVIEW
   ePreviewID     = wxID_PREVIEW,

   eDebugID       = wxID_LOWEST - 2,
   eSettingsID    = wxID_LOWEST - 3,
   ePreviewDryID  = wxID_LOWEST - 4,
   eCloseID       = wxID_CANCEL
};

AUDACITY_DLL_API std::unique_ptr<wxSizer> CreateStdButtonSizer(
   wxWindow *pDlg, wxWindow *parent,
   long buttons = eOkButton | eCancelButton,
   DialogDefinition::Items items = {},
   wxWindow *extra = NULL );

// TypedShuttleGui extends ShuttleGuiBase with Audacity specific extensions,
// and takes template parameters that make certain of the functions more
// convenient
template< typename Sink = wxEvtHandler >
class AUDACITY_DLL_API TypedShuttleGui /* not final */ : public ShuttleGuiBase
{
public:
   using ItemType = DialogDefinition::TypedItem< Sink >;
   ItemType Item( StandardButtonID id = eButtonUndefined )
   { return ItemType{ id }; }

   TypedShuttleGui( const std::shared_ptr< ShuttleGuiState > &pState )
      : ShuttleGuiBase{ pState }
   {}

   TypedShuttleGui(
      wxWindow * pParent, teShuttleMode ShuttleMode,
      bool vertical = true, // Choose layout direction of topmost level sizer
      wxSize minSize = { 250, 100 },
      const std::shared_ptr< PreferenceVisitor > &pVisitor = nullptr )
      : ShuttleGuiBase( pParent, EffectiveMode( ShuttleMode ),
         vertical, minSize, pVisitor )
   {
      if (!(ShuttleMode == eIsCreatingFromPrefs || ShuttleMode == eIsSavingToPrefs))
         return;
      
      mpState -> mpShuttle = std::make_unique<ShuttlePrefs>();
      // In this case the client is the GUI, so if creating we do want to
      // store in the client.
      mpState -> mpShuttle->mbStoreInClient = (ShuttleMode == eIsCreatingFromPrefs );
   };

   TypedShuttleGui( TypedShuttleGui&& ) = default;

   ~TypedShuttleGui() = default;

   template< typename Sink2 >
   TypedShuttleGui< Sink2 > Rebind()
   {
      return { mpState };
   }

   TypedShuttleGui & Optional( bool & bVar )
   {
      mpbOptionalFlag = &bVar;
      return *this;
   }

   TypedShuttleGui & Id(int id )
   {
      miIdSetByUser = id;
      return *this;
   }

   // Only the last item specified as focused (if more than one) will be
   TypedShuttleGui & Focus( bool focused = true )
   {
      GetItem().Focus( focused );
      return *this;
   }

   // For buttons only
   // Only the last item specified as default (if more than one) will be
   TypedShuttleGui & Default( bool isdefault = true )
   {
      GetItem().Default( isdefault );
      return *this;
   }

   // Just sets the state of the item once
   TypedShuttleGui &Disable( bool disabled = true )
   {
      GetItem().Disable( disabled );
      return *this;
   }

   // Specifies a predicate that is retested as needed
   TypedShuttleGui &Enable( const DialogDefinition::Item::Test &test )
   {
      GetItem().Enable( test );
      return *this;
   }

   // Specifies a predicate that is retested as needed
   TypedShuttleGui &Show( const DialogDefinition::Item::Test &test )
   {
      GetItem().Show( test );
      return *this;
   }

   TypedShuttleGui & Text( const DialogDefinition::ControlText &text )
   {
      GetItem().Text( text );
      return *this;
   }

   template<typename Factory>
   TypedShuttleGui& Validator( const Factory &f )
   {
      if ( GetMode() == eIsCreating )
         GetItem().Validator( f );
      return *this;
   }

   // This allows further abbreviation of the previous:
   template<typename V, typename...Args>
   TypedShuttleGui& Validator( Args&& ...args )
   {
      if ( GetMode() == eIsCreating )
         GetItem().template Validator<V>( std::forward<Args>(args)... );
      return *this;
   }

   // Dispatch events from the control to the dialog
   // The template type deduction ensures consistency between the argument type
   // and the event type.  It does not (yet) ensure correctness of the type of
   // the handler object.
   template< typename Tag, typename Argument, typename Handler >
   auto ConnectRoot(
      wxEventTypeTag<Tag> eventType,
      void (Handler::*func)(Argument&)
   )
        -> typename std::enable_if<
            std::is_base_of<Argument, Tag>::value,
            TypedShuttleGui&
        >::type
   {
      GetItem().ConnectRoot( eventType, func );
      return *this;
   }

   TypedShuttleGui & Position( int flags )
   {
      GetItem().Position( flags );
      return *this;
   }

   TypedShuttleGui & Size( wxSize size )
   {
      GetItem().Size( size );
      return *this;
   }

   // Supply an event handler for a control; this is an alternative to using
   // the event table macros, and removes the need to specify an id for the
   // control.
   // The event sink defaults to the dialog itself, or to what was last given
   // to TypedShuttleGui::SetDefaultSink
   // If the control has a validator, the function body can assume the value was
   // validated and transferred from the control successfully
   // After the body, call the dialog's TransferDataToWindow()
   // The event type and id to bind are inferred from the control
   TypedShuttleGui &Action(
      // A pointer to nullary member function of class Sink can be given to
      // this argument:
      const DialogDefinition::BaseItem::ActionType &action )
   {
      GetItem().Action( action );
      return *this;
   }

   // An overload for cases that there is more than one useful event type
   // But it must specify some type whose event class is wxCommandEvent
   TypedShuttleGui &Action(
      // A pointer to nullary member function of class Sink can be given to
      // this argument:
      const DialogDefinition::BaseItem::ActionType &action,
      wxEventTypeTag<wxCommandEvent> type )
   {
      GetItem().Action( action, type );
      return *this;
   }

   // Prop() sets the proportion value, defined as in wxSizer::Add().
   TypedShuttleGui & Prop( int iProp ){ ShuttleGuiBase::Prop(iProp); return *this;}; // Has to be here too, to return a TypedShuttleGui and not a ShuttleGuiBase.

   TypedShuttleGui & Style( long iStyle )
   {
      GetItem().Style( iStyle );
      return *this;
   }

   TypedShuttleGui &MinSize() // set best size as min size
      { GetItem().MinSize(); return *this; }
   TypedShuttleGui &MinSize( wxSize sz )
      { GetItem().MinSize( sz ); return *this; }

   teShuttleMode GetMode() { return mpState -> mShuttleMode; };

private:
   ItemType &&GetItem()
   {
      return std::move( static_cast<ItemType&>(mItem) );
   }

   inline teShuttleMode EffectiveMode( teShuttleMode inMode )
   {
      switch ( inMode ) {
         case eIsCreatingFromPrefs:
            return eIsCreating;
         case eIsSavingToPrefs:
            return eIsGettingFromDialog;
         default:
            return inMode;
      }
   }
};

class ComponentInterfaceSymbol;

class ShuttleGui : public TypedShuttleGui<>
{
public:
   using TypedShuttleGui<>::TypedShuttleGui;
};

//! Convenience function often useful when adding choice controls
AUDACITY_DLL_API TranslatableStrings Msgids(
   const EnumValueSymbol strings[], size_t nStrings);

//! Convenience function often useful when adding choice controls
AUDACITY_DLL_API TranslatableStrings Msgids( const std::vector<EnumValueSymbol> &strings );

#endif
