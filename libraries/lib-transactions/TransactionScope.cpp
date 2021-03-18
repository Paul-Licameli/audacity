/**********************************************************************

  Audacity: A Digital Audio Editor

  @file TransactionScope.cpp

  Paul Licameli split from DBConnection.cpp

**********************************************************************/

#include "TransactionScope.h"
#include "InconsistencyException.h"
#include <wx/log.h>

namespace {
TransactionScope::TransactionScopeImplFactory &GetFactory()
{
   static TransactionScope::TransactionScopeImplFactory theFactory;
   return theFactory;
}

TransactionScope::AutoSaveFunction &GetAutoSave()
{
   static TransactionScope::AutoSaveFunction theFunction;
   return theFunction;
}
}

auto TransactionScope::InstallImplementation(
   TransactionScopeImplFactory factory) -> TransactionScopeImplFactory
{
   auto &theFactory = GetFactory();
   auto result = theFactory;
   theFactory = std::move(factory);
   return result;
}

auto TransactionScope::InstallAutoSave(
   AutoSaveFunction function) -> AutoSaveFunction
{
   auto &theFunction = GetAutoSave();
   auto result = theFunction;
   theFunction = std::move(function);
   return theFunction;
}

void TransactionScope::AutoSave(AudacityProject &project)
{
   auto &function = GetAutoSave();
   if (function)
      function(project);
}

TransactionScopeImpl::~TransactionScopeImpl() = default;

TransactionScope::TransactionScope(
   AudacityProject &project, const char *name)
:  mName(name)
{
   auto &factory = GetFactory();
   if (!factory)
      return;
   mpImpl = factory(project);
   if (!mpImpl)
      return;

   mInTrans = mpImpl->TransactionStart(mName);
   if ( !mInTrans )
      // To do, improve the message
      throw SimpleMessageBoxException( ExceptionType::Internal,
         XO("Database error.  Sorry, but we don't have more details."),
         XO("Warning"),
         "Error:_Disk_full_or_not_writable"
      );
}

TransactionScope::~TransactionScope()
{
   if (mpImpl && mInTrans)
   {
      if (!mpImpl->TransactionRollback(mName))
      {
         // Do not throw from a destructor!
         // This has to be a no-fail cleanup that does the best that it can.
         wxLogMessage("Transaction active at scope destruction");
      }
   }
}

bool TransactionScope::Commit()
{
   if (mpImpl && !mInTrans) {
      wxLogMessage("No active transaction to commit");
      // Misuse of this class
      THROW_INCONSISTENCY_EXCEPTION;
   }

   mInTrans = !mpImpl->TransactionCommit(mName);

   return mInTrans;
}
