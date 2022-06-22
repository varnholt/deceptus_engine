#include "sword.h"

#include <iostream>

#include "constants.h"
#include "debugdraw.h"
#include "framework/tools/stopwatch.h"
#include "player/player.h"
#include "worldquery.h"


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

   const auto& player_rect = Player::getCurrent()->getPlayerPixelRect();

   sf::FloatRect attack_rect_px(player_rect);
   attack_rect_px.left += (24 * _dir_m.x);

   // DebugDraw::drawRect(target, rect_px, sf::Color{255, 0, 0});
   DebugDraw::drawCircle(
      target,
      sf::Vector2f{
         Player::getCurrent()->getPixelPositionf().x * MPP + 0.5f * _dir_m.x,
         Player::getCurrent()->getPixelPositionf().y * MPP - 0.6f ,
      },
      0.75f,
      {1.0f, 0.0f, 0.0f}
   );

   auto hit_nodes = WorldQuery::findNodes(attack_rect_px);

   // this ought to go to a separate class for hit/damage management
   for (auto& node : hit_nodes)
   {
      std::cout << "hit: " << node->_script_name << " " << node->_id << std::endl;
   }
}


void Sword::update(const sf::Time& /*time*/)
{
}


void Sword::initialize()
{
}


void Sword::use(const std::shared_ptr<b2World>& /*world*/, const b2Vec2& pos, const b2Vec2& dir)
{
   _timepoint_used = StopWatch::getInstance().now();

   // are those really needed?
   _pos_m = pos;
   _dir_m = dir;

   // std::cout << "sword attack" << std::endl;
}
