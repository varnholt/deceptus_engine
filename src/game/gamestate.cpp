#include "gamestate.h"


//-----------------------------------------------------------------------------
void GameState::enqueue(ExecutionMode mode)
{
   std::lock_guard<std::mutex> guard(_mutex);
   _queued_mode = mode;
}

//-----------------------------------------------------------------------------
void GameState::sync()
{
   setMode(getQueuedMode());
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
   std::lock_guard<std::mutex> guard(_mutex);
   return _mode;
}


//-----------------------------------------------------------------------------
void GameState::setMode(const ExecutionMode& current)
{
   std::lock_guard<std::mutex> guard(_mutex);
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
   std::lock_guard<std::mutex> guard(_mutex);
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
   if (getMode() == ExecutionMode::Running)
   {
      enqueue(ExecutionMode::Paused);
   }
   else
   {
      enqueue(ExecutionMode::Running);
   }
}
