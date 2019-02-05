/**********************************************************************

  Audacity: A Digital Audio Editor

  Shuttle.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_SHUTTLE__
#define __AUDACITY_SHUTTLE__

#include "../include/audacity/ComponentInterface.h"
#include "../include/audacity/EffectAutomationParameters.h" // for CommandParameters

class ComponentInterfaceSymbol;

template<typename Type> struct EffectParameter {
   const wxChar *const key;
   const Type def;
   const Type min;
   const Type max;
   const Type scale;
   mutable Type cache;
   inline operator const Type& () const { return cache; }
};

template<typename Type>
inline const EffectParameter<Type> Parameter(
   const wxChar *key,
   const Type &def, const Type &min, const Type &max, const Type &scale )
{ return { key, def, min, max, scale, {} }; }

struct EnumEffectParameter : EffectParameter<int>
{
   EnumEffectParameter(
      const wxChar *key, int def, int min, int max, int scale,
      const EnumValueSymbol *symbols_, size_t nSymbols_ )
      : EffectParameter<int>{ key, def, min, max, scale, {} }
      , symbols{ symbols_ }
      , nSymbols{ nSymbols_ }
   {}

   const EnumValueSymbol *const symbols;
   const size_t nSymbols;
};

using EnumParameter = EnumEffectParameter;

class Shuttle /* not final */ {
 public:
   // constructors and destructors
   Shuttle();
   virtual ~Shuttle() {}

 public:
   bool mbStoreInClient;
   wxString mValueString;
   // Even though virtual, mostly the transfer functions won't change
   // for special kinds of archive.
   virtual bool TransferBool( const wxString & Name, bool & bValue, const bool & bDefault );
   virtual bool TransferFloat( const wxString & Name, float & fValue, const float &fDefault );
   virtual bool TransferDouble( const wxString & Name, double & dValue, const double &dDefault );
   virtual bool TransferInt( const wxString & Name, int & iValue, const int &iDefault );
   virtual bool TransferInt( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferLongLong( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferString( const wxString & Name, wxString & strValue, const wxString &strDefault );
   virtual bool TransferEnum( const wxString & Name, int & iValue,
      const int nChoices, const wxString * pFirstStr);
   virtual bool ExchangeWithMaster(const wxString & Name);
};

class CommandParameters;
/**************************************************************************//**
\brief Shuttle that deals with parameters.  This is a base class with lots of
virtual functions that do nothing by default.
Unrelated to class Shuttle.
********************************************************************************/
class AUDACITY_DLL_API ShuttleParams /* not final */
{
public:
   wxString mParams;
   bool *pOptionalFlag;
   CommandParameters * mpEap;
   ShuttleParams() { mpEap = NULL; pOptionalFlag = NULL; }
   virtual ~ShuttleParams() {}
   bool ShouldSet();
   virtual ShuttleParams & Optional( bool & WXUNUSED(var) ){ pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalY( bool & var ){ return Optional( var );};
   virtual ShuttleParams & OptionalN( bool & var ){ return Optional( var );};
   virtual void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin=false, const bool vmax=false, const bool vscl=false );
   virtual void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin=0, const int vmax=100000, const int vscl=1 );
   virtual void Define( int & var,      const wxChar * key, const int vdefault, const int vmin=0, const int vmax=100000, const int vscl=1 );
   virtual void Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl=1.0f );
   virtual void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl=1.0f );
   virtual void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl=1.0f );
   virtual void Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin = {}, const wxString vmax = {}, const wxString vscl = {} );
   virtual void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings );

   template< typename Var, typename Type >
   void SHUTTLE_PARAM( Var &var, const EffectParameter< Type > &name )
   { Define( var, name.key, name.def, name.min, name.max, name.scale ); }

   void SHUTTLE_PARAM( int &var, const EnumEffectParameter &name )
   { DefineEnum( var, name.key, name.def, name.symbols, name.nSymbols ); }
};

/**************************************************************************//**
\brief An object that stores callback functions, generated from the constructor
arguments.
For each variable passed to the constructor:
   Reset resets it to a default
   Visit visits it with a ShuttleParams object
   Get serializes it to a string
   Set deserializes it from a string and returns a success flag
The constructor arguments are alternating references to variables and
EffectParameter objects (and optionally a first argument which is a function
to be called at the end of Reset or Set, and returning a value for Set).
********************************************************************************/
struct CapturedParameters {
public:
   // Types of the callbacks
   using ResetFunction = std::function< void() >;
   using VisitFunction = std::function< void( ShuttleParams & S ) >;
   using GetFunction = std::function< void( CommandParameters & parms ) >;
   // Returns true if successful:
   using SetFunction = std::function< bool ( CommandParameters & parms ) >;

   // Another helper type
   using PostSetFunction = std::function< bool() >;

   // Constructors
   CapturedParameters() = default;

   template< typename ...Args >
   // Arguments are references to variables, alternating with references to
   // EffectParameter objects detailing the callback operations on them.
   CapturedParameters( Args &...args )
      : CapturedParameters( PostSetFunction{}, args... ) {}

   template< typename Fn, typename ...Args,
      // SFINAE: require that first arg be callable
      typename = decltype( std::declval<Fn>()() )
   >
   // Like the previous, but with an extra first argument,
   // which is called at the end of Reset or Set.  Its return value is
   // ignored in Reset() and passed as the result of Set.
   CapturedParameters( const Fn &PostSet, Args &...args )
   {
      PostSetFunction fn = PostSet;
      Reset = [&args..., fn]{ DoReset( fn, args... ); };
      Visit = [&args...]( ShuttleParams &S ){ DoVisit( S, args... ); };
      Get = [&args...]( CommandParameters &parms ){ DoGet( parms, args... ); };
      Set = [&args..., fn]( CommandParameters &parms ){
         return DoSet( parms, fn, args... ); };
   }

   // True iff this was default-constructed
   bool empty() const { return !Reset; }

   // Callable as if they were const member functions
   ResetFunction Reset;
   VisitFunction Visit;
   GetFunction Get;
   SetFunction Set;

private:
   // Base cases stop recursions
   static void DoReset( const PostSetFunction &fn ){ if (fn) fn(); }
   static void DoVisit(ShuttleParams &){}
   static void DoGet(CommandParameters &) {}
   static bool DoSet( CommandParameters &, const PostSetFunction &fn )
      { return !fn || fn(); }

   // Recursive cases
   template< typename Var, typename Type, typename ...Args >
   static void DoReset( const PostSetFunction &fn,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      var = param.def;
      DoReset( fn, others... );
   }

   template< typename Var, typename Type, typename ...Args >
   static void DoVisit( ShuttleParams & S,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      S.Define( var, param.key, param.def, param.min, param.max, param.scale );
      DoVisit( S, others... );
   }
   template< typename ...Args >
   static void DoVisit( ShuttleParams & S,
      int &var, EnumEffectParameter &param, Args &...others )
   {
      S.DefineEnum( var, param.key, param.def, param.symbols, param.nSymbols );
      DoVisit( S, others... );
   }

   template< typename Var, typename Type, typename ...Args >
   static void DoGet( CommandParameters & parms,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      parms.Write( param.key, static_cast<Type>(var) );
      DoGet( parms, others... );
   }
   template< typename ...Args >
   static void DoGet( CommandParameters & parms,
      int &var, EnumEffectParameter &param, Args &...others )
   {
      parms.Write( param.key, param.symbols[ var ].Internal() );
      DoGet( parms, others... );
   }

   template< typename Var, typename Type, typename ...Args >
   static bool DoSet( CommandParameters & parms, const std::function<bool()> &fn,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      if (!parms.ReadAndVerify(param.key, &param.cache, param.def,
         param.min, param.max))
         return false;
      var = param;
      return DoSet( parms, fn, others... );
   }
   template< typename ...Args >
   static bool DoSet( CommandParameters & parms, const std::function<bool()> &fn,
      int &var, EnumEffectParameter &param, Args &...others )
   {
      if (!parms.ReadAndVerify(param.key, &param.cache, param.def,
         param.symbols, param.nSymbols))
         return false;
      var = param;
      return DoSet( parms, fn, others... );
   }
};

/**************************************************************************//**
\brief Shuttle that gets parameter values into a string.
********************************************************************************/
class AUDACITY_DLL_API ShuttleGetAutomation final : public ShuttleParams
{
public:
   ShuttleParams & Optional( bool & var ) override;
   void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl ) override;
   void Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ) override;
   void Define( wxString &var,  const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ) override;
   void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings ) override;
};

/**************************************************************************//**
\brief Shuttle that sets parameters to a value (from a string)
********************************************************************************/
class AUDACITY_DLL_API ShuttleSetAutomation final : public ShuttleParams
{
public:
   ShuttleSetAutomation(){ bWrite = false; bOK = false;};
   bool bOK;
   bool bWrite;
   ShuttleParams & Optional( bool & var ) override;
   bool CouldGet(const wxString &key);
   void SetForValidating( CommandParameters * pEap){ mpEap=pEap; bOK=true;bWrite=false;};
   void SetForWriting(CommandParameters * pEap){ mpEap=pEap;bOK=true;bWrite=true;};
   void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl ) override;
   void Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( float & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ) override;
   void Define( wxString &var,  const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ) override;
   void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings ) override;
};


/**************************************************************************//**
\brief Shuttle that sets parameters to their default values.
********************************************************************************/
class ShuttleDefaults final : public ShuttleParams
{
public:
   wxString Result;
   virtual ShuttleParams & Optional( bool & var )override{  var = true; pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalY( bool & var )override{ var = true; pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalN( bool & var )override{ var = false;pOptionalFlag = NULL;return *this;};

   void Define( bool & var,          const wxChar * WXUNUSED(key),  const bool     vdefault, 
      const bool     WXUNUSED(vmin), const bool     WXUNUSED(vmax), const bool     WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( int & var,           const wxChar * WXUNUSED(key),  const int      vdefault, 
      const int      WXUNUSED(vmin), const int      WXUNUSED(vmax), const int      WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( size_t & var,        const wxChar * WXUNUSED(key),  const int      vdefault, 
      const int      WXUNUSED(vmin), const int      WXUNUSED(vmax), const int      WXUNUSED(vscl) ) 
      override{ var = vdefault;};
   void Define( float & var,         const wxChar * WXUNUSED(key),  const float    vdefault, 
      const float    WXUNUSED(vmin), const float    WXUNUSED(vmax), const float    WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( double & var,        const wxChar * WXUNUSED(key),  const float    vdefault, 
      const float    WXUNUSED(vmin), const float    WXUNUSED(vmax), const float    WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( double & var,        const wxChar * WXUNUSED(key),  const double   vdefault, 
      const double   WXUNUSED(vmin), const double   WXUNUSED(vmax), const double   WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void Define( wxString &var,       const wxChar * WXUNUSED(key),  const wxString vdefault, 
      const wxString WXUNUSED(vmin), const wxString WXUNUSED(vmax), const wxString WXUNUSED(vscl) ) 
      override { var = vdefault;};
   void DefineEnum( int &var,        const wxChar * WXUNUSED(key),  const int vdefault,
      const EnumValueSymbol WXUNUSED(strings) [], size_t WXUNUSED( nStrings ) )
      override { var = vdefault;};
};






#endif
