/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file TranslatableString.h
 
 Paul Licameli split from Types.h
 
 **********************************************************************/

#ifndef __AUDACITY_TRANSLATABLE_STRING__
#define __AUDACITY_TRANSLATABLE_STRING__

#include <stddef.h> // for size_t
#include <functional>
#include <wx/string.h>

class Identifier;

#include <vector>

//! Holds a msgid for the translation catalog; may also bind format arguments
class STRINGS_API FormattedStringBase {
public:
   enum class Request {
      Context,     // return a disambiguating context string
      Format,      // Given the msgid, format the string for end users
      DebugFormat, // Given the msgid, format the string for developers
   };

   //! A multi-purpose function, depending on the enum argument
   /*! the string
      argument is unused in some cases
      If there is no function, defaults are empty context string, no plurals,
      and no substitutions */
   using Formatter = std::function< wxString(const wxString &, Request) >;

   //! Returns true if context is NullContextFormatter
   bool IsVerbatim() const;

   //! MSGID is the English lookup key in the catalog, not necessarily for user's eyes if locale is some other.
   /*! The MSGID might not be all the information FormattedString holds.
      This is a deliberately ugly-looking function name.  Use with caution. */
   Identifier MSGID() const;

private:
   static const wxChar *const NullContextName;

protected:
   static const Formatter NullContextFormatter;

   FormattedStringBase( Formatter formatter = {} )
      : mFormatter{ std::move( formatter ) } {}

   static wxString DoGetContext( const Formatter &formatter );
   static wxString DoSubstitute(
      const Formatter &formatter,
      const wxString &format, const wxString &context, bool debug );
   wxString DoFormat( bool debug ) const
   {  return DoSubstitute(
      mFormatter, mMsgid, DoGetContext(mFormatter), debug ); }
   static wxString DoChooseFormat(
      const Formatter &formatter,
      const wxString &singular, const wxString &plural, unsigned nn, bool debug );

   void Join( FormattedStringBase arg, const wxString &separator = {} );

   wxString mMsgid;
   Formatter mFormatter;
};

// Forward declaration, see below
template< typename String > String Verbatim( wxString );

//! A template that generates subclasses of FormattedStringBase, which do not implicitly inter-convert.
/*!
Implicit conversions to and from wxString are also intentionally disabled.

Different string-valued accessors for the msgid itself, and for the user-visible translation with substitution of
captured format arguments.

Also an accessor for format substitution into the English msgid, for debug-only outputs.

The msgid should be used only in unusual cases and the translation more often
*/
template< typename Derived > class FormattedString
   : public FormattedStringBase {
public:
   FormattedString() {}

   /*! Supply {} for the second argument to cause lookup of the msgid with
      empty context string (default context) rather than the null context (which is for verbatim strings) */
   explicit FormattedString( wxString str, Formatter formatter )
      : FormattedStringBase{ std::move(formatter) }
   {
      mMsgid.swap( str );
   }

   // copy and move
   FormattedString( const FormattedString & ) = default;
   FormattedString &operator=( const FormattedString & ) = default;
   FormattedString( FormattedString && str )
      : FormattedStringBase{ std::move( str.mFormatter ) }
   {
      mMsgid.swap( str.mMsgid );
   }
   FormattedString &operator=( FormattedString &&str )
   {
      mFormatter = std::move( str.mFormatter );
      mMsgid.swap( str.mMsgid );
      return *this;
   }

   //! conversion from other kinds of FormattedString must be explicit
   template< typename Derived2 >
   explicit FormattedString( const FormattedString< Derived2 > &other )
      : FormattedStringBase{ other }
   {
      mMsgid = other.MSGID().GET();
   }
   template< typename Derived2 >
   explicit FormattedString( FormattedString< Derived2 > &&other )
      : FormattedStringBase{ std::move( other ) }
   {
      mMsgid = other.MSGID().GET();
   }

   bool empty() const { return mMsgid.empty(); }

   wxString Translation() const { return DoFormat( false ); }

   //! Format as an English string for debugging logs and developers' eyes, not for end users
   wxString Debug() const { return DoFormat( true ); }

   //! Warning: comparison of msgids only, which is not all of the information!
   /*! This operator makes it easier to define a std::unordered_map on FormattedStrings */
   friend bool operator == (
      const FormattedString &x, const FormattedString &y)
   { return x.mMsgid == y.mMsgid; }

   friend bool operator != (
      const FormattedString &x, const FormattedString &y)
   { return !(x == y); }

   //! Returns true if context is NullContextFormatter
   bool IsVerbatim() const;

   //! Capture variadic format arguments (by copy) when there is no plural.
   /*! The substitution is computed later in a call to Translate() after msgid is
      looked up in the translation catalog.
      Any format arguments that are also of type FormattedString<Tag> will be
      translated too at substitution time, for non-debug formatting */
   template< typename... Args >
   Derived &Format( Args &&...args ) &
   {
      auto prevFormatter = mFormatter;
      this->mFormatter = [prevFormatter, args...]
      (const wxString &str, Request request) -> wxString {
         switch ( request ) {
            case Request::Context:
               return FormattedStringBase::DoGetContext( prevFormatter );
            case Request::Format:
            case Request::DebugFormat:
            default: {
               bool debug = request == Request::DebugFormat;
               return wxString::Format(
                  FormattedStringBase::DoSubstitute(
                     prevFormatter,
                     str, FormattedStringBase::DoGetContext( prevFormatter ),
                     debug ),
                  TranslateArgument( args, debug )...
               );
            }
         }
      };
      return static_cast<Derived&>(*this);
   }
   template< typename... Args >
   Derived &&Format( Args &&...args ) &&
   {
      return std::move( Format( std::forward<Args>(args)... ) );
   }

   //! Choose a non-default and non-null disambiguating context for lookups
   /*! This is meant to be the first of chain-call modifications of the
      FormattedString object; it will destroy any previously captured
      information */
   Derived &Context( const wxString &context ) &
   {
      this->mFormatter = [context]
      (const wxString &str, Request request) -> wxString {
         switch ( request ) {
            case Request::Context:
               return context;
            case Request::DebugFormat:
               return DoSubstitute( {}, str, context, true );
            case Request::Format:
            default:
               return DoSubstitute( {}, str, context, false );
         }
      };
      return static_cast<Derived&>(*this);
   }
   Derived &&Context( const wxString &context ) &&
   {
      return std::move( Context( context ) );
   }

   //! Append another formatted string
   /*! lookup of msgids for
      this and for the argument are both delayed until Translate() is invoked
      on this, and then the formatter concatenates the translations */
   Derived &Join(
      FormattedString arg, const wxString &separator = {} ) &
   {
      FormattedStringBase::Join( std::move( arg ), separator );
      return static_cast<Derived&>(*this);
   }
   Derived &&Join(
      FormattedString arg, const wxString &separator = {} ) &&
   {
      return std::move( Join( std::move(arg), separator ) );
   }

   Derived &operator +=( FormattedString arg )
   {
      Join( std::move( arg ) );
      return static_cast<Derived&>(*this);
   }
private:
   //! Construct a FormattedString that does no translation but passes str verbatim.
   /*! This constructor should not be invoked directly, but instead by the function Verbatim */
   explicit FormattedString( wxString str )
      : FormattedStringBase{ NullContextFormatter }
   {
      mMsgid.swap( str );
   }
   template< typename String > friend String Verbatim( wxString );

   friend std::hash< FormattedString >;

   template< typename T > static const T &TranslateArgument( const T &arg, bool )
   { return arg; }
   //! This allows you to wrap arguments of Format in std::cref
   /*! (So that they are captured (as if) by reference rather than by value) */
   template< typename T > static auto TranslateArgument(
      const std::reference_wrapper<T> &arg, bool debug )
         -> decltype(
            FormattedString::TranslateArgument( arg.get(), debug ) )
   { return FormattedString::TranslateArgument( arg.get(), debug ); }
   static wxString TranslateArgument( const Derived &arg, bool debug )
   { return arg.DoFormat( debug ); }

   //! Implements the XP macro
   /*! That macro specifies a second msgid, a list
      of format arguments, and which of those format arguments selects among
      messages; the translated strings to select among, depending on language,
      might actually be more or fewer than two.  See Internat.h. */
   template< size_t N > struct PluralTemp{
      Derived &ts;
      const wxString &pluralStr;
      template< typename... Args >
         Derived &&operator()( Args&&... args )
      {
         // Pick from the pack the argument that specifies number
         auto selector =
            std::template get< N >( std::forward_as_tuple( args... ) );
         // We need an unsigned value.  Guard against negative values.
         auto nn = static_cast<unsigned>(
            std::max<decltype(selector)>( 0, selector )
         );
         // Name these before we capture them by value in the lambda:
         auto &prevFormatter = this->ts.mFormatter;
         auto &plural = this->pluralStr;

         this->ts.mFormatter = [prevFormatter, plural, nn, args...]
         (const wxString &str, Request request) -> wxString {
            switch ( request ) {
               case Request::Context:
                  return FormattedStringBase::DoGetContext( prevFormatter );
               case Request::Format:
               case Request::DebugFormat:
               default:
               {
                  bool debug = request == Request::DebugFormat;
                  return wxString::Format(
                     FormattedStringBase::DoChooseFormat(
                        prevFormatter, str, plural, nn, debug ),
                     FormattedString::TranslateArgument( args, debug )...
                  );
               }
            }
         };
         return std::move(ts);
      }
   };

public:
   template< size_t N >
   PluralTemp< N > Plural( const wxString &pluralStr ) &&
   {
     return PluralTemp< N >{ static_cast<Derived&>(*this), pluralStr };
   }
};

template< typename Derived > inline Derived operator +(
   const FormattedString< Derived > &x, const FormattedString< Derived > &y  )
{
   Derived temp{ static_cast<const Derived&>( x ) };
   return std::move(temp += std::move(y));
}

//! For using std::unordered_map on FormattedString<Derived>
/*! Note:  hashing on msgids only, which is not all of the information */
namespace std
{
   template<typename Derived> struct hash< FormattedString<Derived> > {
      size_t operator () (const FormattedString<Derived> &str) const // noexcept
      {
         const wxString &stdstr = str.mMsgid.ToStdWstring(); // no allocations, a cheap fetch
         using Hasher = hash< wxString >;
         return Hasher{}( stdstr );
      }
   };
}

//! Generate TranslatableString from the class template
struct TranslatableString : FormattedString< TranslatableString > {
public:
   /*! Translated strings may still contain menu hot-key codes (indicated by &)
      that wxWidgets interprets, and also trailing ellipses, that should be
      removed for other uses. */
   enum StripOptions : unsigned {
      // Values to be combined with bitwise OR
      MenuCodes = 0x1,
      Ellipses = 0x2,
   };

   using FormattedString< TranslatableString >::FormattedString;

   TranslatableString &Strip( unsigned options = MenuCodes ) &
   { DoStrip(options); return *this; }
   TranslatableString &&Strip( unsigned options = MenuCodes ) &&
   { return std::move( Strip( options ) ); }

   //! non-mutating, constructs another FormattedString object
   TranslatableString Stripped( unsigned options = MenuCodes ) const
   { return TranslatableString{ *this }.Strip( options ); }

   wxString StrippedTranslation() const { return Stripped().Translation(); }

private:
   void DoStrip( unsigned options = MenuCodes );
};

//! Allow TranslatableString to work with shift output operators
template< typename Sink >
inline Sink &operator <<( Sink &sink, const TranslatableString &str )
{
   return sink << str.Translation();
}

namespace std
{
   template<> struct hash< TranslatableString >
      : hash< FormattedString< TranslatableString > > {};
}

using TranslatableStrings = std::vector<TranslatableString>;

//! A special string value that will have no screen reader pronunciation
extern const TranslatableString InaudibleString;

//! Require calls to the one-argument constructor to go through this distinct global function name.
/*! This makes it easier to locate and
   review the uses of this function, separately from the uses of the type. */
template< typename String = TranslatableString >
inline String Verbatim( wxString str )
{ return String( std::move( str ) ); }

#endif
