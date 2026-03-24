#pragma once

#include <functional>

///
/// \brief Executes a callback automatically when leaving scope.
///
class ScopeExit
{
public:
   using ExitFunction = std::function<void()>;

   ///
   /// \brief Stores the callback to run on destruction.
   /// \param f Callback executed at scope exit.
   ///
   ScopeExit(const ExitFunction& f);
   ///
   /// \brief Runs the stored exit callback.
   ///
   ~ScopeExit();

private:
   ExitFunction mExitFunction;
};
