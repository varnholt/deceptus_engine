#include "gamestate.h"

//-----------------------------------------------------------------------------
GameState GameState::sInstance;


//-----------------------------------------------------------------------------
void GameState::enqueue(ExecutionMode mode)
{
   mQueuedMode = mode;
}

//-----------------------------------------------------------------------------
void GameState::sync()
{
   setMode(mQueuedMode);
}


//-----------------------------------------------------------------------------
GameState &GameState::getInstance()
{
   return sInstance;
}


//-----------------------------------------------------------------------------
ExecutionMode GameState::getMode() const
{
   return mMode;
}


//-----------------------------------------------------------------------------
void GameState::setMode(const ExecutionMode& current)
{
   if (current == mMode)
   {
      return;
   }

   auto previous = mMode;
   mMode = current;

   for (auto& f : mCallbacks)
   {
      f(current, previous);
   }
}


//-----------------------------------------------------------------------------
void GameState::addCallback(const GameState::StateChangeCallback& cb)
{
   mCallbacks.push_back(cb);
}

ExecutionMode GameState::getQueuedMode() const
{
   return mQueuedMode;
}


//-----------------------------------------------------------------------------
void GameState::enqueuePause()
{
   enqueue(ExecutionMode::Paused);
}


//-----------------------------------------------------------------------------
void GameState::enqueueResume()
{
   enqueue(ExecutionMode::Running);
}


//-----------------------------------------------------------------------------
void GameState::enqueueTogglePauseResume()
{
   if (mMode == ExecutionMode::Running)
   {
      enqueue(ExecutionMode::Paused);
   }
   else
   {
      enqueue(ExecutionMode::Running);
   }
}
