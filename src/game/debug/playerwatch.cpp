#include "playerwatch.h"

#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/constants.h"
#include "game/level/luainterface.h"
#include "game/level/luanode.h"
#include "game/player/playerinterface.h"
#include "game/player/playerregistry.h"
#include "game/state/savestate.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// override WinUser.h
#ifdef MessageBox
#undef MessageBox
#endif

#include "game/ui/messagebox.h"

namespace
{
int32_t __interval_ms{0};  //!< how often the player position is logged, 0 turns the watch off
sf::Time __elapsed;        //!< time since the last log line
}  // namespace

void PlayerWatch::setIntervalInMs(int32_t interval_ms)
{
   __interval_ms = std::max(0, interval_ms);
   __elapsed = sfcompat::timeZero();
}

int32_t PlayerWatch::getIntervalInMs()
{
   return __interval_ms;
}

std::string PlayerWatch::describe()
{
   if (__interval_ms <= 0)
   {
      return "player position watch: off";
   }

   std::ostringstream message;
   message << "player position watch: every " << __interval_ms << "ms";
   return message.str();
}

void PlayerWatch::update(const sf::Time& delta_time)
{
   if (__interval_ms <= 0)
   {
      return;
   }

   __elapsed += delta_time;
   if (__elapsed.asMilliseconds() < __interval_ms)
   {
      return;
   }

   __elapsed = sfcompat::timeZero();

   const auto& player = PlayerRegistry::getFirst();
   if (!player)
   {
      return;
   }

   const auto& position_px = player->getPixelPositionFloat();
   const auto position_x_tl = static_cast<int32_t>(std::floor(position_px.x / PIXELS_PER_TILE));
   const auto position_y_tl = static_cast<int32_t>(std::floor(position_px.y / PIXELS_PER_TILE));

   const auto& health = SaveState::getPlayerInfo()._extra_table._health;

   Log::Info() << "player position: tile " << position_x_tl << " " << position_y_tl << " px " << position_px.x << " " << position_px.y
               << " health " << health._health << "/" << health._health_max << " lives " << health._life_count << " dialogue "
               << (MessageBox::isActive() ? 1 : 0);

   // the enemies close enough to matter for the next few seconds, roughly a screen around the
   // player, so a script driving the game can react to them instead of walking into them
   constexpr auto enemy_range_x_tl = 20;
   constexpr auto enemy_range_y_tl = 12;

   std::ostringstream enemies;
   auto enemy_count = 0;
   for (const auto& node : LuaInterface::instance().getObjectList())
   {
      if (!node || node->_dead)
      {
         continue;
      }

      const auto distance_x_tl = static_cast<int32_t>(std::floor((node->_position_px.x - position_px.x) / PIXELS_PER_TILE));
      const auto distance_y_tl = static_cast<int32_t>(std::floor((node->_position_px.y - position_px.y) / PIXELS_PER_TILE));
      if (std::abs(distance_x_tl) > enemy_range_x_tl || std::abs(distance_y_tl) > enemy_range_y_tl)
      {
         continue;
      }

      enemies << " | " << node->_script_name << " " << distance_x_tl << " " << distance_y_tl;
      enemy_count++;
   }

   Log::Info() << "player enemies: " << enemy_count << enemies.str();
}
