#pragma once

#include "constants.h"

#include <functional>
#include <mutex>

/// \brief tracks current and queued game execution modes and notifies listeners on changes.
class GameState
{
public:
   /// \brief callback signature invoked when the active mode changes.
   /// \param current new active mode after the change.
   /// \param previous previous active mode before the change.
   using StateChangeCallback = std::function<void(ExecutionMode current, ExecutionMode previous)>;

   /// \brief constructs a game state with not-running as active and queued mode.
   GameState() = default;

   /// \brief stores a mode request to be applied later by sync.
   /// \param mode mode that should become active on the next sync call.
   void enqueue(ExecutionMode mode);

   /// \brief applies the queued mode by forwarding it to setMode.
   void sync();

   /// \brief queues the paused mode.
   void enqueuePause();

   /// \brief queues the not-running mode.
   void enqueueStop();

   /// \brief queues the running mode.
   void enqueueResume();

   /// \brief toggles the queued mode between running and paused based on current mode.
   void enqueueTogglePauseResume();

   /// \brief returns the global game state singleton.
   /// \return reference to the shared game state instance.
   static GameState& getInstance();

   /// \brief returns the currently active execution mode.
   /// \return current execution mode.
   ExecutionMode getMode() const;

   /// \brief returns the currently queued execution mode.
   /// \return queued execution mode.
   ExecutionMode getQueuedMode() const;

   /// \brief changes the active mode and notifies registered callbacks when it actually changes.
   /// \param mode new mode to activate.
   void setMode(const ExecutionMode& mode);

   /// \brief registers a callback invoked on each successful mode transition.
   /// \param cb callback receiving current and previous mode values.
   void addCallback(const StateChangeCallback& cb);

private:
   mutable std::mutex _mutex;
   ExecutionMode _mode = ExecutionMode::NotRunning;
   ExecutionMode _queued_mode = ExecutionMode::NotRunning;

   std::vector<StateChangeCallback> _callbacks;
};
