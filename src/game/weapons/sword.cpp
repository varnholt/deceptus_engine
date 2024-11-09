#include "sword.h"

#include <iostream>

#include "framework/tools/stopwatch.h"
#include "game/constants.h"
#include "game/debug/debugdraw.h"
#include "game/debug/drawstates.h"
#include "game/level/fixturenode.h"
#include "game/physics/worldquery.h"
#include "game/player/player.h"

namespace
{
constexpr auto sword_damage = 10;
}

using namespace std::chrono_literals;

Sword::Sword() : _duration_from_swing_start_to_hit(200ms), _duration_from_hit_start_to_end(120ms)
{
   _type = WeaponType::Sword;
}

void Sword::draw(sf::RenderTarget& target [[maybe_unused]])
{
   if (!checkHitWindowActive())
   {
      return;
   }

   if (DrawStates::_draw_debug_info)
   {
      DebugDraw::drawRect(target, _hit_rect_px, sf::Color{255, 0, 0});
   }
}

void Sword::checkBodyCollisions(const std::shared_ptr<b2World>& world)
{
   const auto remaining_bodies = WorldQuery::retrieveBodiesInsideRect(world, _hit_rect_px);
   for (auto* body : remaining_bodies)
   {
      auto* fixture = body->GetFixtureList();
      while (fixture)
      {
         auto* next = fixture->GetNext();

         // fixture doesn't have valid user data
         auto* fixture_node = static_cast<FixtureNode*>(fixture->GetUserData().pointer);
         if (!fixture_node)
         {
            continue;
         }

         // fixture belongs to player
         const auto is_player = (fixture_node->getParent() == Player::getCurrent());
         if (is_player)
         {
            break;
         }

         // std::cout << fixture_node->getType() << std::endl;
         //
         // if (fixture_node->getType() == ObjectTypeSolid)
         // {
         //    cameraShake();
         // }

         // std::cout << fixture->GetFilterData().maskBits << std::endl;

         fixture = next;
      }
   }
}

void Sword::cameraShake()
{
   const auto x = 0.05f;
   const auto y = 0.3f;
   const auto intensity = 0.2f;
   Level::getCurrentLevel()->getBoomEffect().boom(x, y, BoomSettings{intensity, 0.3f, BoomSettings::ShakeType::Random});
}

void Sword::update(const WeaponUpdateData& data)
{
   if (!_cleared_to_attack)
   {
      return;
   }

   if (checkHitWindowActive())
   {
      _cleared_to_attack = false;

      updateHitbox();

      const auto collided_nodes = WorldQuery::findNodes(_hit_rect_px);
      for (auto& collided_node : collided_nodes)
      {
         collided_node._node->luaHit(sword_damage);
      }

      checkBodyCollisions(data._world);

      if (!collided_nodes.empty())
      {
         cameraShake();
      }
   }
}

int32_t Sword::getDamage() const
{
   return 20;
}

std::string Sword::getName() const
{
   return "sword";
}


void Sword::use(const std::shared_ptr<b2World>& world, const b2Vec2& dir)
{
   _cleared_to_attack = true;
   _timepoint_swing_start = StopWatch::getInstance().now();
   _dir_m = dir;
}

bool Sword::checkHitWindowActive() const
{
   const auto now = StopWatch::getInstance().now();
   const auto start = _timepoint_swing_start + _duration_from_swing_start_to_hit;
   const auto end = _timepoint_swing_start + _duration_from_swing_start_to_hit + _duration_from_hit_start_to_end;
   const auto within_active_time_window = (now >= start && now <= end);
   return within_active_time_window;
}

void Sword::updateHitbox()
{
   constexpr auto hitbox_width_px = 60.0f;
   constexpr auto hitbox_height_px = 40.0f;
   auto* player = Player::getCurrent();
   const auto center = player->getPixelPositionFloat();
   const auto crouching = player->getBend().isCrouching();

   const auto hitbox_pos = sf::Vector2f{
      center.x + 0.1f * PPM * _dir_m.x + ((_dir_m.x < 0.0f) ? -hitbox_width_px : 0.0f),
      center.y + (crouching ? -0.1f * PPM : -0.6f * PPM),
   };

   _hit_rect_px = sf::FloatRect(hitbox_pos, sf::Vector2f(hitbox_width_px, hitbox_height_px));
}
