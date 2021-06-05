#include "game/game.h"

#include <chrono>
#include <sstream>

#include "game/constants.h"
#include "game/test.h"


void debugAuthors()
{
   auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
   auto year = (*gmtime(&now)).tm_year + 1900;

   std::stringstream text;
   text << GAME_NAME;
   text << " (c) ";
   text << year;
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
   return game.loop();
}


