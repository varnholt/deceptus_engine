#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include "sdl/include/SDL.h"

class GameControllerDetection
{
public:
   void start();
   void stop();

   using AddedCallback = std::function<void(int32_t)>;
   using RemovedCallback = std::function<void(int32_t)>;

   void setCallbackAdded(const AddedCallback& callback_added);
   void setCallbackRemoved(const RemovedCallback& callback_removed);

private:
   int32_t processEvent(const SDL_Event& event);

   std::unique_ptr<std::thread> _thread;
   bool _stopped = false;
   AddedCallback _callback_added;
   RemovedCallback _callback_removed;
};
