#include "playersword.h"

#include <iostream>
#include <numbers>

#include "framework/tools/stopwatch.h"
#include "game/constants.h"
#include "game/debug/debugdraw.h"
#include "game/debug/drawstates.h"
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
   { return {rect.position.x + rect.size.x / 2.0f, rect.position.y + rect.size.y / 2.0f}; };

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

PlayerSword::PlayerSword() : _duration_from_swing_start_to_hit(200ms), _duration_from_hit_start_to_end(120ms)
{
   _animation_pool.setGarbageCollectorEnabled(true);
   _type = WeaponType::Sword;
}

void PlayerSword::draw(sf::RenderTarget& target)
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
}

void PlayerSword::cameraShake()
{
   const auto x = 0.05f;
   const auto y = 0.3f;
   const auto intensity = 0.2f;
   Level::getCurrentLevel()->getBoomEffect().boom(x, y, BoomSettings{intensity, 0.5f, BoomSettings::ShakeType::Random});
}

std::vector<WorldQuery::CollidedNode> PlayerSword::impactLuaNode(std::unordered_set<b2Body*>& ignored_bodies)
{
   const auto collided_nodes = WorldQuery::findNodesByHitbox(_hit_rect_px);
   for (auto& collided_node : collided_nodes)
   {
      collided_node._node->luaHit(sword_damage);

      if (collided_node._node->_body)
      {
         ignored_bodies.insert(collided_node._node->_body);
      }

      const auto& rect = collided_node._hitbox;
      const auto hitbox_center_pos = sf::Vector2f{rect.position.x + rect.size.x * 0.5f, rect.position.y + rect.size.y * 0.5f};

      // try avoid spamming new animations
      if (_attack_frame == 0)
      {
         cameraShake();

         auto animation =
            _animation_pool.create(_points_left ? "impact_hit_l" : "impact_hit_r", hitbox_center_pos.x, hitbox_center_pos.y, true, false);

         _animations.push_back(animation);
      }
   }

   return collided_nodes;
}

std::vector<std::shared_ptr<GameMechanism>> PlayerSword::impactMechanisms(std::unordered_set<b2Body*>& ignored_bodies)
{
   // could be okay to just go over all destructible mechanisms here
   // read the hitbox, then call hit
   // this can be refactored to return mechanisms that are 'hittable'
   // luahit should then go to the base class and be renamed.
   // only from group "props"
   auto& registry = Level::getCurrentLevel()->getMechanismRegistry();
   auto destructible_mechanisms = registry.searchMechanismsIf(
      [](const std::shared_ptr<GameMechanism>& mechanism, std::string_view /*group_key*/)
      {
         const bool is_destructible = mechanism->isDestructible();
         return is_destructible;
      }
   );

   std::ranges::for_each(
      destructible_mechanisms,
      [&](const auto& mechanism)
      {
         if (std::ranges::any_of(
                mechanism->getHitboxes(),
                [&](const auto& hitbox) { return hitbox.getRectTranslated().findIntersection(_hit_rect_px).has_value(); }
             ))
         {
            mechanism->hit(sword_damage);
         }
      }
   );

   return {};
}

std::optional<sf::Vector2f> PlayerSword::impactSolidObjects(const WeaponUpdateData& data, std::unordered_set<b2Body*>& ignored_bodies)
{
   // since we drew already all impacts on enemies via hitboxes, ignore all enemy bodies for further impact animations
   const auto enemy_bodies = WorldQuery::retrieveEnemyBodiesInsideRect(data._world, _hit_rect_px, ignored_bodies);
   ignored_bodies.insert(enemy_bodies.cbegin(), enemy_bodies.cend());

   // collect collisions with solid objects and everything that's not an enemy
   constexpr auto octree_depth{3};
   WorldQuery::OctreeNode octree(_hit_rect_px, data._world, 0, octree_depth, ignored_bodies);
   _octree_rects = octree.collectLeafBounds(0, octree_depth);

   const auto player_rect_px = Player::getCurrent()->getPixelRectFloat();
   const auto player_center_px =
      sf::Vector2f{player_rect_px.position.x + player_rect_px.size.x / 2.0f, player_rect_px.position.y + player_rect_px.size.y / 2.0f};

   auto solid_object_hit_pos_px = closestCenterToPoint(player_center_px, _octree_rects);
   if (solid_object_hit_pos_px.has_value())
   {
      // correct hit position by shooting a ray
      SwordRayCastCallback raycast_callback(ignored_bodies);
      const auto ray_position_source_px = Player::getCurrent()->getPixelPositionFloat() - sf::Vector2f(0, 24);
      const auto ray_position_source_m = DebugDraw::vecS2B(ray_position_source_px);
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

   return solid_object_hit_pos_px;
}

void PlayerSword::updateImpact(const WeaponUpdateData& data)
{
   if (checkHitWindowActive())
   {
      std::unordered_set<b2Body*> ignored_bodies{{Player::getCurrent()->getBody()}};
      _cleared_to_attack = false;

      updateHitbox();

      // hit lua nodes
      const auto collided_lua_nodes = impactLuaNode(ignored_bodies);

      // hit mechanisms
      const auto collided_mechanisms = impactMechanisms(ignored_bodies);

      // hit solid objects
      auto solid_object_hit_pos_px = impactSolidObjects(data, ignored_bodies);

      if (!collided_lua_nodes.empty() || !collided_mechanisms.empty() || solid_object_hit_pos_px.has_value())
      {
         _attack_frame++;
      }
   }
   else
   {
      _octree_rects.clear();
   }
}

void PlayerSword::updateAnimations(const WeaponUpdateData& data)
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
}

void PlayerSword::update(const WeaponUpdateData& data)
{
   _rays.clear();

   updateAnimations(data);

   if (_cleared_to_attack)
   {
      updateImpact(data);
   }
}

int32_t PlayerSword::getDamage() const
{
   return 20;
}

std::string PlayerSword::getName() const
{
   return "sword";
}

void PlayerSword::use(const std::shared_ptr<b2World>& world, const b2Vec2& dir)
{
   _cleared_to_attack = true;
   _attack_frame = 0;
   _timepoint_swing_start = StopWatch::getInstance().now();
   _dir_m = dir;
   _points_left = (dir.x < 0.0f);
}

bool PlayerSword::checkHitWindowActive() const
{
   const auto now = StopWatch::getInstance().now();
   const auto start = _timepoint_swing_start + _duration_from_swing_start_to_hit;
   const auto end = _timepoint_swing_start + _duration_from_swing_start_to_hit + _duration_from_hit_start_to_end;
   const auto within_active_time_window = (now >= start && now <= end);
   return within_active_time_window;
}

void PlayerSword::updateHitbox()
{
   auto* player = Player::getCurrent();
   const auto crouching = player->getBend().isCrouching();

   const auto hitbox_width_px = crouching ? 40.0f : 60.0f;
   const auto hitbox_height_px = crouching ? 20.0f : 40.0f;

   const auto offset_px = crouching ? sf::Vector2f{0, -15} : sf::Vector2f{0, -10};
   const auto center_px = player->getPixelPositionFloat();

   const auto hitbox_pos = sf::Vector2f{
      offset_px.x + center_px.x + 0.1f * PPM * _dir_m.x + ((_dir_m.x < 0.0f) ? -hitbox_width_px : 0.0f),
      offset_px.y + center_px.y + (crouching ? (-0.1f * PPM) : (-0.6f * PPM)),
   };

   _hit_rect_px = sf::FloatRect(hitbox_pos, sf::Vector2f(hitbox_width_px, hitbox_height_px));
}
