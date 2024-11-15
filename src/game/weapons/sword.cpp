#include "sword.h"

#include <iostream>
#include <numbers>

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

std::optional<sf::Vector2f> closestCenterToPoint(const sf::Vector2f& point, const std::vector<sf::FloatRect>& rects)
{
   if (rects.empty())
   {
      return std::nullopt;
   }

   const auto get_center = [](const sf::FloatRect& rect) -> sf::Vector2f
   { return {rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f}; };

   std::optional<sf::Vector2f> closest_center;
   auto min_distance_squared = std::numeric_limits<float>::max();

   for (const auto& rect : rects)
   {
      const auto center = get_center(rect);
      const auto dx = center.x - point.x;
      const auto dy = center.y - point.y;
      const auto distance_squared = dx * dx + dy * dy;

      if (distance_squared < min_distance_squared)
      {
         min_distance_squared = distance_squared;
         closest_center = center;
      }
   }

   return closest_center;
}

class SwordRayCastCallback : public b2RayCastCallback
{
public:
   SwordRayCastCallback(const std::unordered_set<b2Body*>& ignored_bodies) : _ignored_bodies(ignored_bodies)
   {
   }

   float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
   {
      if (_ignored_bodies.contains(fixture->GetBody()))
      {
         return -1.0f;
      }

      impact_point = point;
      impact_normal = normal;
      return fraction;
   }

   b2Vec2 impact_point;
   b2Vec2 impact_normal;
   std::unordered_set<b2Body*> _ignored_bodies;
};
}

using namespace std::chrono_literals;

Sword::Sword() : _duration_from_swing_start_to_hit(200ms), _duration_from_hit_start_to_end(120ms)
{
   _animation_pool.setGarbageCollectorEnabled(true);
   _type = WeaponType::Sword;
}

void Sword::draw(sf::RenderTarget& target)
{
   for (auto& animation : _animations)
   {
      animation->draw(target);
   }

   if (!checkHitWindowActive())
   {
      return;
   }

   if (DrawStates::_draw_debug_info)
   {
      DebugDraw::drawRect(target, _hit_rect_px, sf::Color{255, 0, 0});

      for (auto& rect : _octree_rects)
      {
         DebugDraw::drawRect(target, rect, sf::Color::Cyan);
      }

      for (auto& ray : _rays)
      {
         DebugDraw::drawLine(target, ray.first, ray.second, b2Color(0, 1, 0, 1));
      }
   }

   _rays.clear();
}

void Sword::cameraShake()
{
   const auto x = 0.05f;
   const auto y = 0.3f;
   const auto intensity = 0.2f;
   Level::getCurrentLevel()->getBoomEffect().boom(x, y, BoomSettings{intensity, 0.5f, BoomSettings::ShakeType::Random});
}

void Sword::update(const WeaponUpdateData& data)
{
   for (auto it = _animations.begin(); it != _animations.end();)
   {
      (*it)->update(data._time);

      if ((*it)->_paused && !(*it)->_looped)
      {
         it = _animations.erase(it);
      }
      else
      {
         ++it;
      }
   }

   if (!_cleared_to_attack)
   {
      return;
   }

   if (checkHitWindowActive())
   {
      _cleared_to_attack = false;

      updateHitbox();

      std::unordered_set<b2Body*> ignored_bodies{{Player::getCurrent()->getBody()}};
      const auto collided_nodes = WorldQuery::findNodes(_hit_rect_px);
      for (auto& collided_node : collided_nodes)
      {
         collided_node._node->luaHit(sword_damage);

         if (collided_node._node->_body != nullptr)
         {
            ignored_bodies.insert(collided_node._node->_body);
         }

         const auto& rect = collided_node._hitbox;
         const auto hitbox_center_pos = sf::Vector2f{rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f};

         // try avoid spamming new animations
         if (_attack_frame == 0)
         {
            cameraShake();

            auto animation = _animation_pool.create(
               _points_left ? "impact_hit_l" : "impact_hit_r", hitbox_center_pos.x, hitbox_center_pos.y, true, false
            );

            _animations.push_back(animation);
         }
      }

      // shoot rays
      SwordRayCastCallback raycast_callback(ignored_bodies);
      const auto ray_position_source_px = Player::getCurrent()->getPixelPositionFloat() - sf::Vector2f(0, 24);
      const auto ray_position_source_m = DebugDraw::vecS2B(ray_position_source_px);

#ifdef SHOOT_RAYS
      constexpr auto ray_count = 5;
      constexpr auto sword_fov = 60.0f * FACTOR_DEG_TO_RAD;
      constexpr auto sword_fov_half = sword_fov / 2.0f;
      const auto base_angle = (_points_left) ? std::numbers::pi : 0;
      const auto start_angle = base_angle - sword_fov_half;
      const auto angle_step = sword_fov / ray_count;

      for (auto i = 0; i < ray_count; ++i)
      {
         const auto ray_angle = start_angle + i * angle_step;
         b2Vec2 ray_direction = b2Vec2(cos(ray_angle), sin(ray_angle));
         b2Vec2 ray_position_target_m = DebugDraw::vecS2B(ray_position_source_px) + (72.0f * MPP) * ray_direction;

         data._world->RayCast(&raycast_callback, ray_position_source_m, ray_position_target_m);

         if (raycast_callback.impact_point.IsValid())
         {
         }

         _rays.push_back({DebugDraw::vecB2S(ray_position_source_m), DebugDraw::vecB2S(ray_position_target_m)});
      }
#endif

      // those are mostly collisions with walls
      constexpr auto octree_depth{3};
      WorldQuery::OctreeNode octree(_hit_rect_px, data._world, 0, octree_depth, ignored_bodies);
      _octree_rects = octree.collectLeafBounds(0, octree_depth);

      const auto player_rect_px = Player::getCurrent()->getPixelRectFloat();
      const auto player_center_px =
         sf::Vector2f{player_rect_px.left + player_rect_px.width / 2.0f, player_rect_px.top + player_rect_px.height / 2.0f};

      auto solid_object_hit_pos_px = closestCenterToPoint(player_center_px, _octree_rects);

      if (solid_object_hit_pos_px.has_value())
      {
         // correct hit position with raycast
         const auto solid_object_hit_pos_m = DebugDraw::vecS2B(solid_object_hit_pos_px.value());
         const auto extended_hit_ray_m = 1.5f * (solid_object_hit_pos_m - ray_position_source_m);
         const auto extended_hit_pos_m = solid_object_hit_pos_m + extended_hit_ray_m;
         data._world->RayCast(&raycast_callback, ray_position_source_m, extended_hit_pos_m);

         _rays.push_back({DebugDraw::vecB2S(ray_position_source_m), DebugDraw::vecB2S(extended_hit_pos_m)});

         if (raycast_callback.impact_point.IsValid())
         {
            solid_object_hit_pos_px = DebugDraw::vecB2S(raycast_callback.impact_point);
         }

         // shake camera and draw animation at detected point
         if (_attack_frame == 0)
         {
            cameraShake();

            auto animation = _animation_pool.create(
               _points_left ? "impact_hit_l" : "impact_hit_r", (*solid_object_hit_pos_px).x, (*solid_object_hit_pos_px).y, true, false
            );

            _animations.push_back(animation);
         }
      }

      if (!collided_nodes.empty() || solid_object_hit_pos_px.has_value())
      {
         _attack_frame++;
      }
   }
   else
   {
      _octree_rects.clear();
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
   _attack_frame = 0;
   _timepoint_swing_start = StopWatch::getInstance().now();
   _dir_m = dir;
   _points_left = (dir.x < 0.0f);
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
   auto* player = Player::getCurrent();
   const auto crouching = player->getBend().isCrouching();
   constexpr auto hitbox_width_px = 60.0f;
   auto hitbox_height_px = crouching ? 20.0f : 40.0f;

   const auto offset = crouching ? sf::Vector2f{0, -15} : sf::Vector2f{0, -10};
   const auto center = player->getPixelPositionFloat();

   const auto hitbox_pos = sf::Vector2f{
      offset.x + center.x + 0.1f * PPM * _dir_m.x + ((_dir_m.x < 0.0f) ? -hitbox_width_px : 0.0f),
      offset.y + center.y + (crouching ? (-0.1f * PPM) : (-0.6f * PPM)),
   };

   _hit_rect_px = sf::FloatRect(hitbox_pos, sf::Vector2f(hitbox_width_px, hitbox_height_px));
}
