#include "scopeexit.h"

ScopeExit::ScopeExit(const ExitFunction& f) : mExitFunction(f)
{
}

ScopeExit::~ScopeExit()
{
   mExitFunction();
}
