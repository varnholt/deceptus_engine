#pragma once

#include "constants.h"

#include <functional>
#include <mutex>

class GameState
{
public:
   using StateChangeCallback = std::function<void(ExecutionMode current, ExecutionMode previous)>;

   GameState() = default;

   void enqueue(ExecutionMode mode);
   void sync();

   void enqueuePause();
   void enqueueStop();
   void enqueueResume();
   void enqueueTogglePauseResume();

   static GameState& getInstance();

   ExecutionMode getMode() const;
   ExecutionMode getQueuedMode() const;
   void setMode(const ExecutionMode& mode);

   void addCallback(const StateChangeCallback& cb);

private:
   mutable std::mutex _mutex;
   ExecutionMode _mode = ExecutionMode::NotRunning;
   ExecutionMode _queued_mode = ExecutionMode::NotRunning;

   std::vector<StateChangeCallback> _callbacks;
};
