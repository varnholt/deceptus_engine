#include "sword.h"

#include <iostream>

#include "constants.h"
#include "debugdraw.h"
#include "framework/tools/stopwatch.h"
#include "player/player.h"
#include "worldquery.h"

namespace
{
constexpr auto sword_damage = 10;
}

Sword::Sword()
{
   _type = WeaponType::Sword;
}

void Sword::draw(sf::RenderTarget& target)
{
   using namespace std::chrono_literals;

   if (StopWatch::getInstance().now() - _timepoint_used > 2s)
   {
      return;
   }

   // TODO: hit must only be carried out at button press timepoint plus frame time offset

   // TODO: must be refactored to separate function and recycled in draw call
   const auto center = sf::Vector2f{
      Player::getCurrent()->getPixelPositionFloat().x + 0.5f * _dir_m.x * PPM,
      Player::getCurrent()->getPixelPositionFloat().y - 0.6f * PPM,
   };

   const auto hit_rect = sf::FloatRect(center, sf::Vector2f(20.0f, 20.0f));

   DebugDraw::drawRect(target, hit_rect, sf::Color{255, 0, 0});
   // DebugDraw::drawCircle(target, circle_center, 0.75f, {1.0f, 0.0f, 0.0f});
}

void Sword::update(const sf::Time& /*time*/)
{
   if (!_cleared_to_attack)
   {
      return;
   }

   const auto attack_frame_has_been_reached = true;

   if (attack_frame_has_been_reached)
   {
      _cleared_to_attack = false;

      // TODO: must be refactored to separate function and recycled in draw call
      const auto& player_rect = Player::getCurrent()->getPixelRectInt();
      sf::FloatRect attack_rect_px(player_rect);
      attack_rect_px.left += (48 * _dir_m.x);

      auto hit_nodes = WorldQuery::findNodes(attack_rect_px);

      // this ought to go to a separate class for hit/damage management
      for (auto& node : hit_nodes)
      {
         std::cout << "hit: " << node->_script_name << " " << node->_id << std::endl;
         node->luaHit(sword_damage);
      }
   }
}

void Sword::initialize()
{
}

void Sword::use(const std::shared_ptr<b2World>& /*world*/, const b2Vec2& dir)
{
   _cleared_to_attack = true;

   _timepoint_used = StopWatch::getInstance().now();

   _dir_m = dir;
}
