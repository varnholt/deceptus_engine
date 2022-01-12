#pragma once

#include "SDL/include/SDL.h"
#include <cstdint>
#include <memory>
#include <thread>

class GameControllerDetection
{
   public:

      void setup();
      void stop();
      int32_t processEvent(const SDL_Event& event);

   private:

      std::unique_ptr<std::thread> _thread;
      bool _stopped = false;
};

