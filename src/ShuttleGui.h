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

#include <bitset>
#include <initializer_list>
#include <functional>
#include <vector>
#include <wx/slider.h> // to inherit
#include <wx/valgen.h> // for wxGenericValidator
#include <wx/valtext.h> // for wxTextValidator
#include "MemoryX.h"
#include <wx/listbase.h> // for wxLIST_FORMAT_LEFT
#include <wx/weakref.h>

#include "Prefs.h"
#include "widgets/NumericTextCtrl.h"
#include "ShuttlePrefs.h"
#include "WrappedType.h"

#ifndef __AUDACITY_NUM_VALIDATOR_STYLE__
#define __AUDACITY_NUM_VALIDATOR_STYLE__
// Bit masks used for numeric validator styles.
enum class NumValidatorStyle : int
{
    DEFAULT               = 0x0,
    THOUSANDS_SEPARATOR   = 0x1,
    ZERO_AS_BLANK         = 0x2,
    NO_TRAILING_ZEROES    = 0x4,
    ONE_TRAILING_ZERO     = 0x8,
    TWO_TRAILING_ZEROES   = 0x10,
    THREE_TRAILING_ZEROES = 0x20
};

inline NumValidatorStyle operator | (NumValidatorStyle x, NumValidatorStyle y)
{ return NumValidatorStyle( int(x) | int(y) ); }

inline int operator & (NumValidatorStyle x, NumValidatorStyle y)
{ return int(x) & int(y); }

// Sometimes useful for specifying max and min values for validators, when they
// must have the same precision as the validated value
AUDACITY_DLL_API double RoundValue(int precision, double value);
#endif

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

#include "Prefs.h"

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

// This allows validators to generalize their means of storage of a value,
// such as by persistency, or transformation of values
template< typename Target >
class Adaptor {
public:
   using TargetType = Target;

   virtual ~Adaptor() = default;

   // Return value is a success indicator
   virtual bool Get( TargetType &target ) const = 0;
   virtual bool Set( const TargetType &value ) = 0;
};

// This adapts a variable
template< typename Target >
class BasicAdaptor : public Adaptor< Target > {
public:
   ~BasicAdaptor() override = default;

   BasicAdaptor( Target &target ) : mTarget{ target } {}

   bool Get( Target &target ) const override
   {
      target = mTarget;
      return true;
   }

   bool Set( const Target &value ) override
   {
      mTarget = value;
      return true;
   }

private:
   Target &mTarget;
};

// This applies transformations to a variable
template< typename Stored, typename Transformed = Stored >
class Transformer : public Adaptor< Transformed > {
public:
   // Transform the stored value to a value associated with a control
   using Getter = std::function< Transformed(Stored) >;
   // Inverse transformation from control value to stored
   using Setter = std::function< Stored(Transformed) >;

   Transformer( Stored &stored, Getter getter, Setter setter )
      : mStored{ stored }
      , mGetter{ std::move( getter ) }
      , mSetter{ std::move( setter ) }
   {
      wxASSERT( mGetter );
      wxASSERT( mSetter );
   }
   ~Transformer() override = default;

   bool Get( Transformed &target ) const override
   {
      target = mGetter( mStored );
      return true;
   }

   bool Set( const Transformed &value ) override
   {
      mStored = mSetter( value );
      return true;
   }

private:
   Stored &mStored;
   const Getter mGetter;
   const Setter mSetter;
};

// Factory function that deduces template parameters for Transformer,
// except in case Target needs to be distinct from Stored
template<
   typename Stored,
   typename Target = Stored,
   typename Getter = std::function< Target(Stored) >,
   typename Setter = std::function< Stored(Target) >
>
inline std::shared_ptr< Adaptor< Target > >
Transform( Stored &stored,
   // Getter and Setter default to simple type conversions between
   // Target and Stored types
   Getter &&getter = [](Stored value){ return static_cast<Target>( value ); },
   Setter &&setter = [](Target value){ return static_cast<Stored>( value ); } )
{
   return std::make_shared< Transformer< Stored, Target > >( stored,
      std::forward< Getter >( getter ), std::forward< Setter >( setter ) );
}

// Detects whether any of the controls is transiently in a state with invalid
// values, so that other controls (such as an OK button) can be disabled
class ValidationState {
   static constexpr size_t nFlags = 64;
   using Bitset = std::bitset<nFlags>;

public:
   using Slot = Bitset::reference;

   bool Ok() const { return mFlags.all(); }
   Slot ReserveSlot();
   
private:
   size_t mNext = 0;
   Bitset mFlags = ~Bitset{}; // initially all true
};

// Factory function that deduces template parameters for Transformer,
// except in case Target needs to be distinct from Stored.
// Generates a getter and a setter that multiply and divide by a scale factor,
// which is often useful with slider controls.
template<
   typename Stored,
   typename Target = Stored,
   typename Factor,
   typename Origin = int
>
inline std::shared_ptr< Adaptor< Target > >
Scale( Stored &stored, Factor scale, Origin origin = 0 )
{
   return std::make_shared< Transformer< Stored, Target > >( stored,
      [origin, scale]( double output ){ return (output - origin) * scale; },
      [origin, scale]( double input ){ return input / scale + origin; } );
}

// This adapts a setting stored in preferences
template< typename Target >
class SettingAdaptor : public Adaptor< Target > {
public:
   using SettingType = Setting< Target >;

   SettingAdaptor( SettingType &setting ) : mSetting{ setting } {}

   ~SettingAdaptor() override = default;

   SettingType &GetSetting() { return mSetting; }

   bool Get( Target &target ) const override
   {
      return mSetting.Read( &target );
   }

   bool Set( const Target &value ) override
   {
      return mSetting.Write( value ) && gPrefs->Flush();
   }

private:
   SettingType &mSetting;
};

struct ComputedChoices
{
   using TranslatableStrings = ::TranslatableLabels;

   using StringsFunction = Recomputer< Identifiers >;
   using TranslatableStringsFunction = Recomputer< TranslatableStrings >;
   StringsFunction GetChoices;
   mutable Identifiers cache;

   ComputedChoices() = default;

   ComputedChoices( const TranslatableStrings &strings )
      : ComputedChoices{ TranslatableStringsFunction{ strings } }
   {}

   ComputedChoices( const ::TranslatableStrings &strings )
      : ComputedChoices{ TranslatableStringsFunction{
         TranslatableLabels{ strings.begin(), strings.end() } } }
   {}

   ComputedChoices( TranslatableStringsFunction fn )
   {
      GetChoices = [fn]{
         std::optional< Identifiers > result;
         if ( auto strings = fn() )
            result.emplace( transform_container< Identifiers >( *strings,
               std::mem_fn( &TranslatableLabel::Translation ) ) );
         return result;
      };
   }

private:
   explicit ComputedChoices( StringsFunction fn )
      : GetChoices{ std::move( fn ) }
   {}
   friend ComputedChoices Verbatim( StringsFunction fn );
};

inline ComputedChoices Verbatim( ComputedChoices::StringsFunction fn )
{
   return ComputedChoices{ std::move( fn ) };
}

// A mix-in base class for adaptors that may also repopulate a control with
// items (such as choice or listbox)
struct ChoiceAdaptor
{
   explicit ChoiceAdaptor( ComputedChoices translated )
      : mChoices{ std::move( translated ) }
   {}
   virtual ~ChoiceAdaptor();
   ComputedChoices mChoices;
};

class SingleChoiceAdaptor
   : public Adaptor< int >
   , public ChoiceAdaptor
{
public:
   SingleChoiceAdaptor( ComputedChoices translated )
   : ChoiceAdaptor{ std::move( translated ) }
   {
   }

   ~SingleChoiceAdaptor() override;
   bool Get( TargetType &target ) const final;
   bool Set( const TargetType &value ) final;

protected:
   virtual bool Get( int &index, const Identifiers &strings ) const = 0;
   virtual bool Set( int index, const Identifier &str ) = 0;
};

class IntChoiceAdaptor final : public SingleChoiceAdaptor
{
public:
   IntChoiceAdaptor( int &index, ComputedChoices translated )
      : SingleChoiceAdaptor{ std::move( translated ) }
      , mIndex{ index }
   {}

   ~IntChoiceAdaptor() override;

private:
   bool Get( int &index, const Identifiers &strings ) const override;
   bool Set( int index, const Identifier &str ) override;

   int &mIndex;
};

class StringChoiceAdaptor final : public SingleChoiceAdaptor
{
public:
   // Optional third argument gives array of string values to be stored,
   // if not the same as those shown to the user
   StringChoiceAdaptor(
      std::unique_ptr< Adaptor< wxString > > pAdaptor,
      ComputedChoices translated,
      ComputedChoices::StringsFunction internals = {} )
      : SingleChoiceAdaptor{ std::move( translated ) }
      , mpAdaptor{ std::move( pAdaptor ) }
      , mInternals{ std::move( internals ) }
   {}

   ~StringChoiceAdaptor() override;

private:
   bool Get( int &index, const Identifiers &strings ) const override;
   bool Set( int index, const Identifier &str ) override;

   std::unique_ptr< Adaptor< wxString > > mpAdaptor;
   ComputedChoices::StringsFunction mInternals;
   mutable Identifiers mCachedInternals;
};

inline std::shared_ptr< Adaptor<int> >
Choice( int &index, ComputedChoices translated )
{
   return std::make_shared< IntChoiceAdaptor >(
      index, std::move( translated ) );
}

inline std::shared_ptr< Adaptor<int> >
Choice( wxString &string, ComputedChoices translated,
   ComputedChoices::StringsFunction internals = {} )
{
   return std::make_shared< StringChoiceAdaptor >(
      std::make_unique< BasicAdaptor< wxString > >( string ),
      std::move( translated ), std::move( internals ) );
}

inline std::shared_ptr< Adaptor<int> >
Choice( StringSetting &setting, ComputedChoices translated,
   ComputedChoices::StringsFunction internals = {} )
{
   return std::make_shared< StringChoiceAdaptor >(
      std::make_unique< SettingAdaptor< wxString > >( setting ),
      std::move( translated ), std::move( internals ) );
}

class MultipleChoiceAdaptor
   : public BasicAdaptor< std::vector<int> >
   , public ChoiceAdaptor
{
public:
   MultipleChoiceAdaptor( std::vector<int> &ints, ComputedChoices choices )
      : BasicAdaptor< std::vector<int> >{ ints }
      , ChoiceAdaptor{ std::move( choices ) }
   {}
   ~MultipleChoiceAdaptor() override;
};

inline std::shared_ptr< Adaptor< std::vector<int> > >
MultipleChoice( std::vector<int> &ints, ComputedChoices choices )
{
   return std::make_shared< MultipleChoiceAdaptor >(
      ints, std::move( choices ) );
}

class NumberChoiceAdaptor
   : public Adaptor< int >
   , public ChoiceAdaptor
{
public:
   NumberChoiceAdaptor(
      std::unique_ptr< Adaptor< int > > pAdaptor,
      ComputedChoices choices,
         Recomputer< std::vector<int> > values = std::vector<int>{} )
      : ChoiceAdaptor{ std::move( choices ) }
      , mpAdaptor{ std::move( pAdaptor ) }
      , mFindValues{ std::move( values ) }
   {
   }
   ~NumberChoiceAdaptor() override;

   bool Get( TargetType &target ) const override;
   bool Set( const TargetType &value ) override;

private:
   std::unique_ptr< Adaptor< int > > mpAdaptor;
   Recomputer< std::vector<int> > mFindValues;
   mutable std::vector<int> mValues;
};

inline std::shared_ptr< Adaptor< int > >
NumberChoice(
   int &target, ComputedChoices choices,
   Recomputer< std::vector<int> > values = std::vector<int>{} )
{
   return std::make_shared< NumberChoiceAdaptor >(
      std::make_unique< BasicAdaptor<int> >( target ),
      std::move( choices ), std::move( values ) );
}

inline std::shared_ptr< Adaptor< int > >
NumberChoice(
   IntSetting &setting, ComputedChoices choices,
   Recomputer< std::vector<int> > values = std::vector<int>{} )
{
   return std::make_shared< NumberChoiceAdaptor >(
      std::make_unique< SettingAdaptor<int> >( setting ),
      std::move( choices ), std::move( values ) );
}

class ChoiceSettingAdaptor
   : public Adaptor< int >
{
public:
   using SettingType = ChoiceSetting;

   explicit ChoiceSettingAdaptor( ChoiceSetting &setting )
      : mSetting{ setting }
   {}

   ~ChoiceSettingAdaptor() override;

   SettingType &GetSetting() { return mSetting; }

   bool Get( TargetType &target ) const override;
   bool Set( const TargetType &value ) override;

private:
   SettingType &mSetting;
};

class LabelSettingAdaptor
   : public Adaptor< int >
{
public:
   using SettingType = LabelSetting;

   explicit LabelSettingAdaptor( LabelSetting &setting )
      : mSetting{ setting }
   {}

   ~LabelSettingAdaptor() override;

   SettingType &GetSetting() { return mSetting; }

   bool Get( TargetType &target ) const override;
   bool Set( const TargetType &value ) override;

private:
   SettingType &mSetting;
};

template< typename Target > struct AdaptorFor{};

template<> struct AdaptorFor< bool >
   { using type = BasicAdaptor< bool >; };
template<> struct AdaptorFor< int >
   { using type = BasicAdaptor< int >; };
template<> struct AdaptorFor< std::vector<int> >
   { using type = BasicAdaptor< std::vector<int> >; };
template<> struct AdaptorFor< double >
   { using type = BasicAdaptor< double >; };
template<> struct AdaptorFor< wxString >
   { using type = BasicAdaptor< wxString >; };

template<> struct AdaptorFor< BoolSetting >
   { using type = SettingAdaptor< bool >; };
template<> struct AdaptorFor< IntSetting >
   { using type = SettingAdaptor< int >; };
template<> struct AdaptorFor< DoubleSetting >
   { using type = SettingAdaptor< double >; };
template<> struct AdaptorFor< StringSetting >
   { using type = SettingAdaptor< wxString >; };
template<> struct AdaptorFor< ChoiceSetting >
   { using type = ChoiceSettingAdaptor; };
template<> struct AdaptorFor< LabelSetting >
   { using type = LabelSettingAdaptor; };
template< typename Enum > struct AdaptorFor< EnumSetting< Enum > >
   { using type = ChoiceSettingAdaptor; };
template< typename Enum > struct AdaptorFor< EnumLabelSetting< Enum > >
   { using type = LabelSettingAdaptor; };

template< typename Target >
class AdaptingValidatorBase
{
public:
   using TargetType = Target;
   using AdaptorType = Adaptor< TargetType >;

   AdaptingValidatorBase(
      const std::shared_ptr< ValidationState > &pValidationState,
      const std::shared_ptr< AdaptorType > &pAdaptor )
      : mpValidationState{ pValidationState }
      , mSlot{ pValidationState->ReserveSlot() }
      , mpAdaptor{ pAdaptor }
   {}

   AdaptingValidatorBase &operator= ( const AdaptingValidatorBase& ) = delete;

   // This class must be findable with dynamic_cast
   ~AdaptingValidatorBase() = default;

   AdaptorType &GetAdaptor() const { return *mpAdaptor; }

protected:
   Target mTemp{};
   std::shared_ptr< ValidationState > mpValidationState;
   ValidationState::Slot mSlot;
   std::shared_ptr< AdaptorType > mpAdaptor;
};

// This can shuttle to checkboxes
class BoolValidator
   : public wxGenericValidator
   , public AdaptingValidatorBase< bool >
{
public:
   using TargetType = bool;
   using AdaptorType = Adaptor< TargetType >;
   using SettingAdaptorType = SettingAdaptor< TargetType >;

   BoolValidator( const std::shared_ptr< ValidationState > &pValidationState,
      const std::shared_ptr< AdaptorType > &pAdaptor );
   BoolValidator( const BoolValidator & );
   ~BoolValidator() override;

   bool Validate( wxWindow *parent ) override;
   wxObject *Clone() const override;
   bool TransferFromWindow() override;
   bool TransferToWindow() override;
};

// This can shuttle to text boxes, combos, spin controls, sliders, choices,
// single selection list boxes, and radio buttons (a group of radio buttons
// being treated like one choice).
// Also book controls, to change the page.
// For text boxes and combos, validates the characters
class IntValidator
   : public wxValidator
   , public AdaptingValidatorBase< int >
{
public:
   using TargetType = int;
   using AdaptorType = Adaptor< TargetType >;
   using SettingAdaptorType = SettingAdaptor< TargetType >;

   using RadioButtonList = std::vector< wxWindowRef >;

   bool TryBefore(wxEvent& event) override;

   IntValidator( const std::shared_ptr< ValidationState > &pValidationState,
      const std::shared_ptr< AdaptorType > &pAdaptor,
      NumValidatorStyle style = NumValidatorStyle::DEFAULT,
      int min = std::numeric_limits<int>::min(),
      int max = std::numeric_limits<int>::max() );
   IntValidator( const IntValidator& );

   ~IntValidator() override;
   bool Validate( wxWindow *parent ) override;
   wxObject *Clone() const override;
   bool TransferFromWindow() override;
   bool TransferToWindow() override;

private:
   std::unique_ptr< wxValidator > mDelegate;
   std::shared_ptr< RadioButtonList > mRadioButtons;
};

// This can shuttle to multiple selection list boxes
class IntVectorValidator
   : public wxValidator
   , public AdaptingValidatorBase< std::vector<int> >
{
public:
   using TargetType = std::vector<int>;
   using AdaptorType = Adaptor< TargetType >;

   IntVectorValidator( const std::shared_ptr< ValidationState > &pValidationState,
      const std::shared_ptr< AdaptorType > &pAdaptor );
   IntVectorValidator( const IntVectorValidator& );

   ~IntVectorValidator() override;
   bool Validate( wxWindow *parent ) override;
   wxObject *Clone() const override;
   bool TransferFromWindow() override;
   bool TransferToWindow() override;
};

// This can shuttle to text boxes, combos, sliders, and NumericTextCtrl
// For text boxes and combos, validates the characters
class DoubleValidator
   : public wxValidator
   , public AdaptingValidatorBase< double >
{
public:
   using TargetType = double;
   using AdaptorType = Adaptor< TargetType >;
   using SettingAdaptorType = SettingAdaptor< TargetType >;

   DoubleValidator( const std::shared_ptr< ValidationState > &pValidationState,
      const std::shared_ptr< AdaptorType > &pAdaptor,
      NumValidatorStyle style = NumValidatorStyle::DEFAULT,
      int precision = std::numeric_limits<double>::digits10,
      double min = std::numeric_limits<double>::lowest(),
      double max =  std::numeric_limits<double>::max() );
   DoubleValidator( const DoubleValidator& );
   ~DoubleValidator() override;

   bool TryBefore(wxEvent& event) override;

   bool Validate( wxWindow *parent ) override;
   wxObject *Clone() const override;
   bool TransferFromWindow() override;
   bool TransferToWindow() override;
private:
   std::unique_ptr< wxValidator > mDelegate;
   // Used in case of text or combo boxes to avoid precision loss when
   // transferring the value out
   double mExactValue = 0;
};

// This can shuttle to combo boxes and text controls
class StringValidator
   : public wxTextValidator
   , public AdaptingValidatorBase< wxString >
{
public:
   using TargetType = wxString;
   using AdaptorType = Adaptor< TargetType >;
   using SettingAdaptorType = SettingAdaptor< TargetType >;

   struct Options {
      Options() {}

      wxString allowed;
      bool numeric = false;

      Options &&AllowedChars( const wxString &chars ) &&
      { allowed = chars; return std::move( *this ); }

      Options &&Numeric() &&
      { numeric = true; return std::move( *this ); }
   };

   StringValidator( const std::shared_ptr< ValidationState > &pValidationState,
      const std::shared_ptr< AdaptorType > &pAdaptor,
      const Options &options = {} );
   StringValidator( const StringValidator& );
   ~StringValidator() override;

   bool Validate( wxWindow *parent ) override;
   wxObject *Clone() const override;
   bool TransferFromWindow() override;
   bool TransferToWindow() override;

private:
   void ApplyOptions();

   Options mOptions{};
};

struct BaseItem
{
   using ActionType = std::function< void() >;
   using Test = std::function< bool() >;
   using ControlTextFunction = std::function< ControlText() >;

   // This is a "curried" function!
   using ValidatorSetter =
   std::function< std::function< void(wxWindow*) >(
      const std::shared_ptr< ValidationState > & ) >;

   BaseItem() = default;

   explicit BaseItem(StandardButtonID id)
   {
      mStandardButton = id;
   }

   ValidatorSetter mValidatorSetter;

   Test mEnableTest;
   Test mShowTest;

   ControlText mText;
   ControlTextFunction mComputedText;

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

// A metafunction used by TypedItem<Sink>::Target
template< typename > struct ValidatorFor {};

template<> struct ValidatorFor<bool> { using type = BoolValidator; };
template<> struct ValidatorFor<int> { using type = IntValidator; };
template<> struct ValidatorFor<std::vector<int>>
   { using type = IntVectorValidator; };
template<> struct ValidatorFor<double> { using type = DoubleValidator; };
template<> struct ValidatorFor<wxString> { using type = StringValidator; };

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
      mValidatorSetter =
      [f]( const std::shared_ptr<ValidationState> & ){
         return [f](wxWindow *p){ p->SetValidator(f()); };
      };
      return std::move(*this);
   }

   // This overload cooperates with Target to make appropriate
   // Adaptors:
   template<typename V, typename... Args>
   TypedItem&& Validator( Args&&... args ) &&
   {
      mValidatorSetter =
      [args...]( const std::shared_ptr<ValidationState> & pState ){
         return [pState, args...](wxWindow *p){
            p->SetValidator( V( pState, args... ) ); };
      };
      return std::move(*this);
   }

   // This overload cooperates with Target to make appropriate
   // Adaptors:
   template<typename V, typename Referent, typename... Args>
   TypedItem&& Validator(
      std::reference_wrapper<Referent> target, Args&&... args ) &&
   {
      mValidatorSetter =
      [target, args...]( const std::shared_ptr<ValidationState> & pState ){
         return [pState, target, args...](wxWindow *p){
            p->SetValidator(
               V( pState, MakeAdaptor( target.get() ), args... ) ); };
      };
      return std::move(*this);
   }

   // This overload takes a reference directly to the targeted variable and
   // causes the appropriate adaptor and validator to be built
   template< typename Arg, typename... Args >
   TypedItem &&Target( Arg &target, Args&&... args ) &&
   {
      using Adaptor = typename AdaptorFor< Arg >::type;
      using Validator =
         typename ValidatorFor< typename Adaptor::TargetType >::type;
      mValidatorSetter =
      [&target, args...]( const std::shared_ptr<ValidationState> & pState ){
         return [pState, &target, args...](wxWindow *p){
            p->SetValidator( Validator(
               pState, std::make_shared< Adaptor >( target ), args... ) ); };
      };
      return std::move( *this );
   }
   
   // This overload takes a shared_ptr to an adaptor object that
   // already wraps the targeted value and causes the appropriate validator
   // to be built
   template< typename Adaptor, typename... Args >
   TypedItem &&Target(
      const std::shared_ptr< Adaptor > &target, Args&&... args ) &&
   {
      using Validator =
         typename ValidatorFor< typename Adaptor::TargetType >::type;
      mValidatorSetter =
      [target, args...]( const std::shared_ptr<ValidationState> & pState ){
         return [pState, target, args...](wxWindow *p){
            p->SetValidator( Validator( pState, target, args... ) ); };
      };
      return std::move( *this );
   }

   TypedItem &&Text( const ControlText &text ) &&
   {
      mText = text;
      return std::move( *this );
   }

   // Recompute whenever another control's contents are modified
   TypedItem &&VariableText( const ControlTextFunction &fn ) &&
   {
      mComputedText = fn;
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

   std::shared_ptr< DialogDefinition::ValidationState > mpValidationState;
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

   const std::shared_ptr<DialogDefinition::ValidationState>
   &GetValidationState() const
   { return mpState -> mpValidationState; }

//-- Add functions.  These only add a widget or 2.
   void AddPrompt(const TranslatableLabel &Prompt, int wrapWidth = 0);
   void AddUnits(const TranslatableString &Units, int wrapWidth = 0);
   void AddTitle(const TranslatableString &Title, int wrapWidth = 0);
   wxWindow * AddWindow(wxWindow * pWindow);
   void AddSlider(
      const TranslatableLabel &Prompt, int pos, int Max, int Min = 0,
      // Pass -1 to mean unspecified:
      int lineSize = -1, int pageSize = -1 );
   void AddVSlider(const TranslatableLabel &Prompt, int pos, int Max);
   void AddSpinCtrl(const TranslatableLabel &Prompt,
      int Value, int Max, int Min);
   void AddTree();

   // Spoken name of the button defaults to the same as the prompt
   // (after stripping menu codes):
   void AddRadioButton( const TranslatableLabel & Prompt );

   // Only the last button specified as default (if more than one) will be
   // Always ORs the flags with wxALL (which affects borders):
   void AddButton(
      const TranslatableLabel & Label, int PositionFlags = wxALIGN_CENTRE,
      bool setDefault = false );
   // Only the last button specified as default (if more than one) will be
   // Always ORs the flags with wxALL (which affects borders):
   void AddBitmapButton(
      const wxBitmap &Bitmap, int PositionFlags = wxALIGN_CENTRE,
      bool setDefault = false );
   // When PositionFlags is 0, applies wxALL (which affects borders),
   // and either wxALIGN_CENTER (if bCenter) or else wxEXPAND
   void AddVariableText(
      const TranslatableString &Str, bool bCenter = false,
      int PositionFlags = 0, int wrapWidth = 0);
   void AddReadOnlyText(
      const TranslatableLabel &Caption,
      const wxString &Value);
   void AddTextBox(
      const TranslatableLabel &Prompt,
      const wxString &Value, const int nChars);
   void AddNumericTextBox(
      const TranslatableLabel &Prompt,
      const wxString &Value, const int nChars);
   void AddTextWindow(const wxString &Value);
   void AddListBox(const wxArrayStringEx &choices);

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
   void AddListControl(
      std::initializer_list<const ListControlColumn> columns = {},
      long listControlStyles = 0
   );
   void AddListControlReportMode(
      std::initializer_list<const ListControlColumn> columns = {},
      long listControlStyles = 0
   );

   void AddGrid();

   void AddNumericTextCtrl(NumericConverter::Type type,
         const NumericFormatSymbol &formatName = {},
         double value = 0.0,
         double sampleRate = 44100,
         const NumericTextCtrl::Options &options = {},
         const wxPoint &pos = wxDefaultPosition,
         const wxSize &size = wxDefaultSize);

   void AddCheckBox( const TranslatableLabel &Prompt, bool Selected);
   void AddCheckBoxOnRight( const TranslatableLabel &Prompt, bool Selected);

   // These deleted overloads are meant to break compilation of old calls that
   // passed literal "true" and "false" strings
   wxCheckBox * AddCheckBox( const TranslatableLabel &Prompt, const wxChar *) = delete;
   wxCheckBox * AddCheckBox( const TranslatableLabel &Prompt, const char *) = delete;
   wxCheckBox * AddCheckBoxOnRight( const TranslatableLabel &Prompt, const wxChar *) = delete;
   wxCheckBox * AddCheckBoxOnRight( const TranslatableLabel &Prompt, const char *) = delete;

   void AddCombo( const TranslatableLabel &Prompt,
      const wxString &Selected, const wxArrayStringEx & choices );
   void AddChoice( const TranslatableLabel &Prompt,
      const TranslatableStrings &choices, int Selected = -1 );
   void AddChoice( const TranslatableLabel &Prompt,
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
   // Target and action and other attributes can be specified, which will
   // apply to each button, unless re-specified for an individual button.
   void StartRadioButtonGroup();
   // SettingName is a key in Preferences.
   void StartRadioButtonGroup( LabelSetting &Setting );

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
   void TieRadioButton();

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

   static void ApplyText( const DialogDefinition::ControlText &text,
      wxWindow *pWind );

   static void ApplyItem( int step,
      const DialogDefinition::BaseItem &item,
      const std::shared_ptr< DialogDefinition::ValidationState > &pState,
      wxWindow *pWind, wxWindow *pDlg,
      PreferenceVisitor *pVisitor = nullptr );

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
   LabelSetting *mpRadioSetting = nullptr; /// The setting controlled by a group.
   Optional<WrappedType> mRadioValue;  /// The wrapped value associated with the active radio button.
   int mRadioCount = -1;       /// The index of this radio item.  -1 for none.
   wxString mRadioValueString; /// Unwrapped string value.
   DialogDefinition::BaseItem mRadioItem;
   void DoAddRadioButton(
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
   const std::shared_ptr< DialogDefinition::ValidationState > &pValidationState,
   long buttons = eOkButton | eCancelButton,
   DialogDefinition::Items items = {},
   wxWindow *extra = NULL );

// TypedShuttleGui extends ShuttleGuiBase with Audacity specific extensions,
// and takes template parameters that make certain of the functions more
// convenient
template< typename Sink = wxEvtHandler, typename WindowType = wxWindow >
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

   template< typename Sink2, typename Window2 = WindowType >
   TypedShuttleGui< Sink2, Window2 > Rebind()
   {
      return { mpState };
   }

   TypedShuttleGui & Optional( bool & bVar )
   {
      Target( bVar ).AddCheckBox( {} );
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
   TypedShuttleGui &Enable( const DialogDefinition::BaseItem::Test &test )
   {
      GetItem().Enable( test );
      return *this;
   }

   // Specifies a predicate that is retested as needed
   TypedShuttleGui &Show( const DialogDefinition::BaseItem::Test &test )
   {
      GetItem().Show( test );
      return *this;
   }

   TypedShuttleGui &Text( const DialogDefinition::ControlText &text )
   {
      GetItem().Text( text );
      return *this;
   }

   // Recompute whenever another control's contents are modified
   TypedShuttleGui &VariableText(
      const DialogDefinition::BaseItem::ControlTextFunction &fn )
   {
      GetItem().VariableText( fn );
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
         GetItem().Validator( [args...]{ return V( args... ); } );
      return *this;
   }

   template< typename ... Args >
   TypedShuttleGui &Target( Args &&...args )
   {
      GetItem().Target( std::forward<Args>( args )... );
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

   operator WindowType* () const { return static_cast<WindowType*>(mpWind); }
   WindowType* operator -> () const { return *this; }

   TypedShuttleGui<Sink, wxSlider>
   AddSlider(
      const TranslatableLabel &Prompt, int pos, int Max, int Min = 0,
      // Pass -1 to mean unspecified:
      int lineSize = -1, int pageSize = -1 )
   {
      ShuttleGuiBase::AddSlider( Prompt, pos, Max, Min, lineSize, pageSize );
      return Rebind<Sink, wxSlider>();
   }
   
   TypedShuttleGui<Sink, wxSlider>
   AddVSlider(const TranslatableLabel &Prompt, int pos, int Max)
   {
      ShuttleGuiBase::AddSlider( Prompt, pos, Max );
      return Rebind<Sink, wxSlider>();
   }

   TypedShuttleGui<Sink, wxSpinCtrl>
   AddSpinCtrl(const TranslatableLabel &Prompt,
      int Value, int Max, int Min)
   {
      ShuttleGuiBase::AddSpinCtrl(Prompt, Value, Max, Min);
      return Rebind<Sink, wxSpinCtrl>();
   }

   TypedShuttleGui<Sink, wxTreeCtrl>
   AddTree()
   {
      ShuttleGuiBase::AddTree();
      return Rebind<Sink, wxTreeCtrl>();
   }

   // Spoken name of the button defaults to the same as the prompt
   // (after stripping menu codes):
   TypedShuttleGui<Sink, wxRadioButton>
   AddRadioButton( const TranslatableLabel & Prompt )
   {
      ShuttleGuiBase::AddRadioButton( Prompt );
      return Rebind<Sink, wxRadioButton>();
   }

   // Only the last button specified as default (if more than one) will be
   // Always ORs the flags with wxALL (which affects borders):
   TypedShuttleGui<Sink, wxButton>
   AddButton(
      const TranslatableLabel & Text, int PositionFlags = wxALIGN_CENTRE,
      bool setDefault = false )
   {
      ShuttleGuiBase::AddButton( Text, PositionFlags, setDefault );
      return Rebind<Sink, wxButton>();
   }

   // Only the last button specified as default (if more than one) will be
   // Always ORs the flags with wxALL (which affects borders):
   TypedShuttleGui<Sink, wxBitmapButton>
   AddBitmapButton(
      const wxBitmap &Bitmap, int PositionFlags = wxALIGN_CENTRE,
      bool setDefault = false )
   {
      ShuttleGuiBase::AddBitmapButton( Bitmap, PositionFlags, setDefault );
      return Rebind<Sink, wxBitmapButton>();
   }

   // When PositionFlags is 0, applies wxALL (which affects borders),
   // and either wxALIGN_CENTER (if bCenter) or else wxEXPAND
   TypedShuttleGui<Sink, wxStaticText>
   AddVariableText(
      const TranslatableString &Str, bool bCenter = false,
      int PositionFlags = 0, int wrapWidth = 0)
   {
      ShuttleGuiBase::AddVariableText( Str, bCenter, PositionFlags, wrapWidth );
      return Rebind<Sink, wxStaticText>();
   }

   TypedShuttleGui<Sink, ReadOnlyText>
   AddReadOnlyText(
      const TranslatableLabel &Caption,
      const wxString &Value)
   {
      ShuttleGuiBase::AddReadOnlyText( Caption, Value );
      return Rebind<Sink, ReadOnlyText>();
   }

   TypedShuttleGui<Sink, wxTextCtrl>
   AddTextBox(
      const TranslatableLabel &Caption,
      const wxString &Value = {}, const int nChars = 0)
   {
      ShuttleGuiBase::AddTextBox(Caption, Value, nChars);
      return Rebind<Sink, wxTextCtrl>();
   }

   TypedShuttleGui<Sink, wxTextCtrl>
   AddNumericTextBox(
      const TranslatableLabel &Caption,
      const wxString &Value, const int nChars)
   {
      ShuttleGuiBase::AddNumericTextBox( Caption, Value, nChars );
      return Rebind<Sink, wxTextCtrl>();
   }

   TypedShuttleGui<Sink, wxTextCtrl>
   AddTextWindow(const wxString &Value)
   {
      ShuttleGuiBase::AddTextWindow( Value );
      return Rebind<Sink, wxTextCtrl>();
   }

   TypedShuttleGui<Sink, wxListBox>
   AddListBox(const wxArrayStringEx &choices)
   {
      ShuttleGuiBase::AddListBox( choices );
      return Rebind<Sink, wxListBox>();
   }

   TypedShuttleGui<Sink, wxListCtrl>
   AddListControl(
      std::initializer_list<const ListControlColumn> columns = {},
      long listControlStyles = 0
   )
   {
      ShuttleGuiBase::AddListControl( columns, listControlStyles );
      return Rebind<Sink, wxListCtrl>();
   }

   TypedShuttleGui<Sink, wxListCtrl>
   AddListControlReportMode(
      std::initializer_list<const ListControlColumn> columns = {},
      long listControlStyles = 0
   )
   {
      ShuttleGuiBase::AddListControlReportMode( columns, listControlStyles );
      return Rebind<Sink, wxListCtrl>();
   }

   TypedShuttleGui<Sink, NumericTextCtrl>
   AddNumericTextCtrl(NumericConverter::Type type,
         const NumericFormatSymbol &formatName = {},
         double value = 0.0,
         double sampleRate = 44100,
         const NumericTextCtrl::Options &options = {},
         const wxPoint &pos = wxDefaultPosition,
         const wxSize &size = wxDefaultSize)
   {
      ShuttleGuiBase::AddNumericTextCtrl(
         type, formatName, value, sampleRate, options, pos, size );
      return Rebind<Sink, NumericTextCtrl>();
   }

   TypedShuttleGui<Sink, wxGrid>
   AddGrid()
   {
      ShuttleGuiBase::AddGrid();
      return Rebind<Sink, wxGrid>();
   }

   TypedShuttleGui<Sink, wxCheckBox>
   AddCheckBox( const TranslatableLabel &Prompt, bool Selected = false)
   {
      ShuttleGuiBase::AddCheckBox( Prompt, Selected );
      return Rebind<Sink, wxCheckBox>();
   }

   TypedShuttleGui<Sink, wxCheckBox>
   AddCheckBoxOnRight( const TranslatableLabel &Prompt, bool Selected = false)
   {
      ShuttleGuiBase::AddCheckBoxOnRight( Prompt, Selected );
      return Rebind<Sink, wxCheckBox>();
   }

   TypedShuttleGui<Sink, wxComboBox>
   AddCombo( const TranslatableLabel &Prompt,
      const wxString &Selected, const wxArrayStringEx & choices )
   {
      ShuttleGuiBase::AddCombo( Prompt, Selected, choices );
      return Rebind<Sink, wxComboBox>();
   }

   TypedShuttleGui<Sink, wxChoice>
   AddChoice( const TranslatableLabel &Prompt,
      const TranslatableStrings &choices = {}, int Selected = -1 )
   {
      ShuttleGuiBase::AddChoice( Prompt, choices, Selected );
      return Rebind<Sink, wxChoice>();
   }

   TypedShuttleGui<Sink, wxChoice>
   AddChoice( const TranslatableLabel &Prompt,
      const TranslatableStrings &choices, const TranslatableString &selected )
   {
      ShuttleGuiBase::AddChoice( Prompt, choices, selected );
      return Rebind<Sink, wxChoice>();
   }

   // Must be called between a StartRadioButtonGroup / EndRadioButtonGroup pair,
   // and as many times as there are values in the enumeration.
   TypedShuttleGui<Sink, wxRadioButton>
   TieRadioButton()
   {
      ShuttleGuiBase::TieRadioButton();
      return Rebind<Sink, wxRadioButton>();
   }

   // Chain a post-construction step
   template<typename F>
   TypedShuttleGui &Initialize( const F &f )
   {
      WindowType *p = *this;
      f( p );
      return *this;
   }

   // Alternative using pointer-to-member functions
   template<typename R, typename C, typename... Params, typename... Args>
   TypedShuttleGui &Initialize(
      R (C::*pmf)(Params...),
      Args... args
   )
   { return Initialize( [=](WindowType *p){ return (p->*pmf)(args...); } ); }

   // A common post-step is to record a pointer to the constructed window
   // Be careful that the reference to pointer does not dangle!
   template<typename Ptr>
   TypedShuttleGui &Assign ( Ptr &p )
   {
      WindowType *pWindow = *this;
      p = pWindow;
      return *this;
   }

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
