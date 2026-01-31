#include "playerdive.h"

#include "framework/tools/log.h"
// #include "game/player/player.h"
#include "game/ui/messagebox.h"

void PlayerDive::update(const sf::Time& dt, bool in_water)
{
   _dive_duration = in_water ? (_dive_duration + dt.toDuration()) : HighResDuration::zero();

   using namespace std::chrono_literals;
   if (_dive_duration > 10min)
   {
      Log::Info() << "no super hero can hold their breath longer than 10 minutes";
      _dive_duration = HighResDuration::zero();

      MessageBox::info(
         "You believe you can hold your breath for 10 minutes.[br]Fortunately, this game is generous.", [](MessageBox::Button) {}
      );

      // Player::getCurrent()->damage(100);
   }
}
