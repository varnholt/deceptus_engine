#include "game/game.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <sstream>

#include "framework/tools/logthread.h"
#include "game/constants.h"
#include "game/io/preloader.h"
#include "game/tests/test.h"

#ifdef __linux__
extern "C" int XInitThreads();
#endif

void debugAuthors()
{
   const std::chrono::time_point<std::chrono::system_clock> now{std::chrono::system_clock::now()};
   const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};

   std::stringstream text;
   text << GAME_NAME;
   text << " (c) ";
   text << static_cast<int32_t>(ymd.year());
   text << " dstar/mueslee";

   std::cout << text.str() << std::endl;
   for (auto i = 0u; i < text.str().length(); i++)
   {
      std::cout << "-";
   }
   std::cout << std::endl;
}

int main(int /*argc*/, char** /*argv*/)
{
#ifndef DEBUG
   // setup logging to file
   LogThread log_thread;
   Log::registerListenerCallback(
      std::bind(&LogThread::log, &log_thread, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)
   );
#endif

#ifdef __linux__
   XInitThreads();
#endif

   debugAuthors();
   Test test;
   Game game;
   game.initialize();
   Preloader::preload();
   return game.loop();
}
