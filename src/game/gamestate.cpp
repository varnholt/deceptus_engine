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
   mMode = mQueuedMode;
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
void GameState::setMode(const ExecutionMode &mode)
{
   mMode = mode;
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
