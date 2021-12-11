#include "sword.h"

#include <iostream>

#include "constants.h"
#include "debugdraw.h"
#include "framework/tools/stopwatch.h"


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

   sf::FloatRect rect_px;
   rect_px.left = _pos_m.x * PPM ;
   rect_px.top = _pos_m.y * PPM - 32;
   rect_px.width = 24;
   rect_px.height = 48;

   DebugDraw::drawRect(target, rect_px, sf::Color{255, 0, 0});
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

   std::cout << "sword attack" << std::endl;
}
