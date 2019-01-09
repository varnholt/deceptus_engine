#include "game/game.h"

#include <chrono>
#include <sstream>

// to read
// http://www.gamasutra.com/blogs/YoannPignole/20140103/207987/Platformer_controls_how_to_avoid_limpness_and_rigidity_feelings.php
// https://stackoverflow.com/questions/8205088/preventing-box2d-player-from-pressing-against-walls-in-midair


// TODO:

// http://www.emanueleferonato.com/2013/09/10/creation-of-a-box2d-hook-like-the-one-seen-on-ios-mikey-hooks-game/
// http://www.iforce2d.net/b2dtut/joints-overview
// https://www.youtube.com/watch?v=C-ScURIRTGA

void debugAuthors()
{
   auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
   auto year = (*gmtime(&now)).tm_year + 1900;

   std::stringstream text;
   text << "a.d.a.m (c) ";
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
   Game* game = new Game();
   game->initialize();
   return game->loop();
}
