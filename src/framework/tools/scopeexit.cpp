#include "scopeexit.h"

#include <utility>

ScopeExit::ScopeExit(ExitFunction  f) : mExitFunction(std::move(f))
{
}

ScopeExit::~ScopeExit()
{
   mExitFunction();
}
