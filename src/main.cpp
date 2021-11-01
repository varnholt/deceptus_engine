#include "game/game.h"

#include <chrono>
#include <sstream>

#include "game/constants.h"
#include "game/preloader.h"
#include "game/test.h"


void debugAuthors()
{
   const std::chrono::time_point<std::chrono::system_clock> now{std::chrono::system_clock::now()};
   const std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};

   std::stringstream text;
   text << GAME_NAME;
   text << " (c) ";
   text << ymd.year();
   text << " dstar/mueslee";
   printf("%s\n", text.str().c_str());
   for (auto i = 0u; i < text.str().length(); i++)
   {
      printf("-");
   }
   printf("\n");
}


int main(int /*argc*/, char** /*argv*/)
{
   debugAuthors();
   Test test;
   Game game;
   game.initialize();
   Preloader::preload();
   return game.loop();
}


