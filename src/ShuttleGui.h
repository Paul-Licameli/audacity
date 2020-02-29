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

#include <vector>
#include <wx/slider.h> // to inherit
#include "MemoryX.h"
#include <wx/listbase.h> // for wxLIST_FORMAT_LEFT
#include <wx/weakref.h>

#include "Prefs.h"
#include "widgets/NumericTextCtrl.h"
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

struct Item {
   Item() = default;

   // Factory is a class that returns a value of some subclass of wxValidator
   // We must wrap it in another lambda to allow the return type of f to
   // vary, and avoid the "slicing" problem.
   // (That is, std::function<wxValidator()> would not work.)
   template<typename Factory>
   Item&& Validator( const Factory &f ) &&
   {
      mValidatorSetter = [f](wxWindow *p){ p->SetValidator(f()); };
      return std::move(*this);
   }

   // This allows further abbreviation of the previous:
   template<typename V, typename... Args>
   Item&& Validator( Args&&... args ) &&
   { return std::move(*this).Validator( [args...]{ return V( args... ); } ); }

   Item&& Text( const ControlText &text ) &&
   {
      mText = text;
      return std::move( *this );
   }

   Item&& Style( long style ) &&
   {
      miStyle = style;
      return std::move( *this );
   }

   // Only the last item specified as focused (if more than one) will be
   Item&& Focus( bool focused = true ) &&
   {
      mFocused = focused;
      return std::move( *this );
   }

   Item&& Disable( bool disabled = true ) &&
   {
      mDisabled = disabled;
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
            Item&&
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

   Item&& MinSize() && // set best size as min size
   {
      mUseBestSize = true;
      return std::move ( *this );
   }

   Item&& MinSize( wxSize sz ) &&
   {
      mMinSize = sz; mHasMinSize = true;
      return std::move ( *this );
   }

   Item&& Position( int flags ) &&
   {
      mWindowPositionFlags = flags;
      return std::move( *this );
   }

   Item&& Size( wxSize size ) &&
   {
      mWindowSize = size;
      return std::move( *this );
   }

   std::function< void(wxWindow*) > mValidatorSetter;
   ControlText mText;

   std::vector<std::pair<wxEventType, wxObjectEventFunction>> mRootConnections;

   long miStyle{};

   // Applies to windows, not to subsizers
   int mWindowPositionFlags{ 0 };

   wxSize mWindowSize = wxDefaultSize;

   wxSize mMinSize{ -1, -1 };
   bool mHasMinSize{ false };
   bool mUseBestSize{ false };

   bool mFocused { false };
   bool mDisabled { false };

};

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
   void SetBorder( int Border ) {miBorder = Border;};
   void SetSizerProportion( int iProp ) {miSizerProp = iProp;};
   void SetStretchyCol( int i );
   void SetStretchyRow( int i );

//--Some Additions since June 2007 that don't fit in elsewhere...
   wxWindow * GetParent()
   {
      // This assertion justifies the use of safenew in many places where GetParent()
      // is used to construct a window
      wxASSERT(mpParent != NULL);
      return mpParent;
   }
   ShuttleGuiBase & Prop( int iProp );
   void UseUpId();

   wxSizer * GetSizer() {return mpSizer;}

   static void ApplyItem( int step, const DialogDefinition::Item &item,
      wxWindow *pWind, wxWindow *pDlg );

protected:
   void SetProportions( int Default );
   void PushSizer();
   void PopSizer();

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
   wxWindow *const mpDlg;
   wxSizer * pSizerStack[ nMaxNestedSizers ];

   std::unique_ptr<Shuttle> mpShuttle; /*! Controls source/destination of shuttled data.  You can
   leave this NULL if you are shuttling to variables */
   int miNoMatchSelector = 0; //! Used in choices to determine which item to use on no match.

   teShuttleMode mShuttleMode;

   int miSizerProp = 0;
   int mSizerDepth;
   int miBorder;
   int miProp = 0;

   // See UseUpId() for explanation of these three.
   int miId = -1;
   int miIdNext;
   int miIdSetByUser = -1;
   // Proportion set by user rather than default.
   int miPropSetByUser = -1;

   bool * mpbOptionalFlag = nullptr;

   std::unique_ptr<wxSizer> mpSubSizer;
   wxSizer * mpSizer;
   wxWindow * mpParent;
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
   DialogDefinition::Item mItem;

   using RadioButtonList = std::vector< wxWindowRef >;
   std::shared_ptr< RadioButtonList > mRadioButtons;

   std::shared_ptr< PreferenceVisitor > mpVisitor;
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

// CreateStdButtonSizer defs...should probably move to widgets subdir
enum
{
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

AUDACITY_DLL_API std::unique_ptr<wxSizer> CreateStdButtonSizer( wxWindow *parent,
                               long buttons = eOkButton | eCancelButton,
                               wxWindow *extra = NULL );

// ShuttleGui extends ShuttleGuiBase with Audacity specific extensions.
class AUDACITY_DLL_API ShuttleGui /* not final */ : public ShuttleGuiBase
{
public:
   ShuttleGui(
      wxWindow * pParent, teShuttleMode ShuttleMode,
      bool vertical = true, // Choose layout direction of topmost level sizer
      wxSize minSize = { 250, 100 },
      const std::shared_ptr< PreferenceVisitor > &pVisitor = nullptr );

   ~ShuttleGui(void);
public:
   ShuttleGui & Optional( bool & bVar );
   ShuttleGui & Id(int id );

   // Only the last item specified as focused (if more than one) will be
   ShuttleGui & Focus( bool focused = true )
   {
      std::move( mItem ).Focus( focused );
      return *this;
   }

   ShuttleGui &Disable( bool disabled = true )
   {
      std::move( mItem ).Disable( disabled );
      return *this;
   }

   ShuttleGui & Text( const DialogDefinition::ControlText &text )
   {
      std::move( mItem ).Text( text );
      return *this;
   }

   template<typename Factory>
   ShuttleGui& Validator( const Factory &f )
   {
      if ( GetMode() == eIsCreating )
         std::move( mItem ).Validator( f );
      return *this;
   }

   // This allows further abbreviation of the previous:
   template<typename V, typename...Args>
   ShuttleGui& Validator( Args&& ...args )
   {
      if ( GetMode() == eIsCreating )
         std::move( mItem ).Validator<V>( std::forward<Args>(args)... );
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
            ShuttleGui&
        >::type
   {
      std::move( mItem ).ConnectRoot( eventType, func );
      return *this;
   }

   ShuttleGui & Position( int flags )
   {
      std::move( mItem ).Position( flags );
      return *this;
   }

   ShuttleGui & Size( wxSize size )
   {
      std::move( mItem ).Size( size );
      return *this;
   }

   // Prop() sets the proportion value, defined as in wxSizer::Add().
   ShuttleGui & Prop( int iProp ){ ShuttleGuiBase::Prop(iProp); return *this;}; // Has to be here too, to return a ShuttleGui and not a ShuttleGuiBase.

   ShuttleGui & Style( long iStyle )
   {
      std::move( mItem ).Style( iStyle );
      return *this;
   }

   ShuttleGui &MinSize() // set best size as min size
      { std::move( mItem ).MinSize(); return *this; }
   ShuttleGui &MinSize( wxSize sz )
      { std::move( mItem ).MinSize( sz ); return *this; }

   // The first of these buttons, if any, that is included will be default:
   // Apply, Yes, OK
   void AddStandardButtons(
      long buttons = eOkButton | eCancelButton, wxWindow *extra = NULL );

   wxSizerItem * AddSpace( int width, int height, int prop = 0 );
   wxSizerItem * AddSpace( int size ) { return AddSpace( size, size ); };

   // Calculate width of a choice control adequate for the items, maybe after
   // the dialog is created but the items change.
   static void SetMinSize( wxWindow *window, const TranslatableStrings & items );
   static void SetMinSize( wxWindow *window, const wxArrayStringEx & items );
  // static void SetMinSize( wxWindow *window, const std::vector<int> & items );

   teShuttleMode GetMode() { return  mShuttleMode; };
};

class ComponentInterfaceSymbol;

//! Convenience function often useful when adding choice controls
AUDACITY_DLL_API TranslatableStrings Msgids(
   const EnumValueSymbol strings[], size_t nStrings);

//! Convenience function often useful when adding choice controls
AUDACITY_DLL_API TranslatableStrings Msgids( const std::vector<EnumValueSymbol> &strings );

#endif
