#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

/// \brief runs an SDL event loop on a worker thread to detect controller hotplug events.
class GameControllerDetection
{
public:
   /// \brief starts the background event thread that waits for SDL controller events.
   void start();
   /// \brief stops the event thread and waits until it has terminated.
   void stop();

   using AddedCallback = std::function<void(int32_t)>;
   using RemovedCallback = std::function<void(int32_t)>;

   /// \brief sets the callback invoked for SDL joystick-added events.
   /// \param callback_added handler receiving the added device index.
   void setCallbackAdded(const AddedCallback& callback_added);
   /// \brief sets the callback invoked for SDL joystick-removed events.
   /// \param callback_removed handler receiving the removed device instance id.
   void setCallbackRemoved(const RemovedCallback& callback_removed);

private:
   /// \brief dispatches a single SDL event to the configured add/remove callbacks when applicable.
   /// \param event SDL event retrieved from the wait loop.
   /// \return always returns 0 after processing.
   int32_t processEvent(const SDL_Event& event);

   std::unique_ptr<std::thread> _thread;
   bool _stopped = false;
   AddedCallback _callback_added;
   RemovedCallback _callback_removed;
};
