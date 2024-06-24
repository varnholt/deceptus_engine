#pragma once

#include <functional>

class ScopeExit
{
public:
   using ExitFunction = std::function<void()>;

   ScopeExit(ExitFunction  f);
   ~ScopeExit();

private:
   ExitFunction mExitFunction;
};
