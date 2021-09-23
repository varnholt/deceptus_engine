#include "gamestate.h"


//-----------------------------------------------------------------------------
void GameState::enqueue(ExecutionMode mode)
{
   _queued_mode = mode;
}

//-----------------------------------------------------------------------------
void GameState::sync()
{
   setMode(_queued_mode);
}


//-----------------------------------------------------------------------------
GameState& GameState::getInstance()
{
   static GameState sInstance;
   return sInstance;
}


//-----------------------------------------------------------------------------
ExecutionMode GameState::getMode() const
{
   return _mode;
}


//-----------------------------------------------------------------------------
void GameState::setMode(const ExecutionMode& current)
{
   if (current == _mode)
   {
      return;
   }

   auto previous = _mode;
   _mode = current;

   for (auto& f : _callbacks)
   {
      f(current, previous);
   }
}


//-----------------------------------------------------------------------------
void GameState::addCallback(const GameState::StateChangeCallback& cb)
{
   _callbacks.push_back(cb);
}


//-----------------------------------------------------------------------------
ExecutionMode GameState::getQueuedMode() const
{
   return _queued_mode;
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
   if (_mode == ExecutionMode::Running)
   {
      enqueue(ExecutionMode::Paused);
   }
   else
   {
      enqueue(ExecutionMode::Running);
   }
}
