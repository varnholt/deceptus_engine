#ifndef GAMESTATE_H
#define GAMESTATE_H


#include "constants.h"


class GameState
{

private:

   ExecutionMode mMode = ExecutionMode::Running;
   ExecutionMode mQueuedMode = ExecutionMode::Running;

public:

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
};

#endif // GAMESTATE_H
