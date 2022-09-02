#pragma once

#include <functional>

class ScopeExit
{
public:
   using ExitFunction = std::function<void()>;

   ScopeExit(const ExitFunction& f);
   ~ScopeExit();

private:
   ExitFunction mExitFunction;
};
