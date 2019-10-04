#pragma once

#include "constants.h"

#include <functional>


class GameState
{

public:

   using StateChangeCallback = std::function<void(ExecutionMode current, ExecutionMode previous)>;

   GameState() = default;

   void enqueue(ExecutionMode mode);
   void sync();

   void enqueuePause();
   void enqueueResume();
   void enqueueTogglePauseResume();

   static GameState sInstance;
   static GameState& getInstance();

   ExecutionMode getMode() const;
   void setMode(const ExecutionMode &mode);

   void addCallback(const StateChangeCallback& cb);


private:

   ExecutionMode mMode = ExecutionMode::Running;
   ExecutionMode mQueuedMode = ExecutionMode::Running;

   std::vector<StateChangeCallback> mCallbacks;

};

