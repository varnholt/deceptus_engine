#include "rainoverlay.h"

#include "debugdraw.h"
#include "framework/math/sfmlmath.h"
#include "game/gameconfiguration.h"
#include "level.h"
#include "player/player.h"
#include "texturepool.h"
#include "worldquery.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{

static const auto max_age_s = 1.5f;            // time for raindrop to move through all screens
static const auto randomize_factor_y = 0.02f;  // randomized to 0..2
static const auto fixed_direction_y = 1000.0f;


sf::Vector2f vecB2S(const b2Vec2 &vector)
{
   return{vector.x * PPM, vector.y * PPM};
}


b2Vec2 vecS2B(const sf::Vector2f& vector)
{
   return{vector.x * MPP, vector.y * MPP};
}

}


RainOverlay::RainOverlay()
{
   _texture = TexturePool::getInstance().get("data/sprites/rain.png");

   std::srand(static_cast<uint32_t>(std::time(nullptr))); // use current time as seed for random generator

   for (auto a = 0; a < _settings._drop_count; a++)
   {
      RainDrop drop;
      drop._sprite.setTexture(*_texture);
      _drops.push_back(drop);
   }
}


void RainOverlay::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   const auto& screen_view = target.getView();

   _screen = {
      screen_view.getCenter().x - screen_view.getSize().x / 2.0f,
      screen_view.getCenter().y - screen_view.getSize().y / 2.0f,
      screen_view.getSize().x,
      screen_view.getSize().y
   };

   // source: foreground
   // dest:   background

   static sf::BlendMode blend_mode(
         sf::BlendMode::SrcAlpha,         // colorSourceFactor
         sf::BlendMode::OneMinusSrcAlpha, // colorDestinationFactor
         sf::BlendMode::Add,              // colorBlendEquation
         sf::BlendMode::SrcAlpha,         // alphaSourceFactor
         sf::BlendMode::OneMinusSrcAlpha, // alphaDestinationFactor
         sf::BlendMode::Add               // alphaBlendEquation
   );

   for (auto& d : _drops)
   {
      if (d._age_s >= 0.0f)
      {
         // DebugDraw::drawLine(target, d._origin_px, d._pos_px + sf::Vector2f{0.0f, 96.0f}, {0, 0, 1});
         target.draw(d._sprite, blend_mode);
      }
   }

   if (_settings._collide)
   {
      for (auto& hit : _hits)
      {
         // DebugDraw::drawPoint(target, hit._pos_px, {1, 0, 0});
         target.draw(hit._sprite);
      }
   }
}


// rain tileset
//    11px x 12px
//    4 rows
//    4 columns
//
// rain drops
//    0 1 2 3
//    4 5 6 7
//
// drop hit
//    0 1 2 3
//    4 5 6 7

void RainOverlay::update(const sf::Time& dt)
{
   // screen not initialized yet
   if (_screen.width == 0)
   {
      return;
   }

   // go through all edges and find the closest intersection; this way we know when the rain drop will hit the floor
   auto update_colliding_edge = [this](RainDrop& p) {

      p._intersections.clear();

      for (const auto& edge : _edges)
      {
         auto intersection = SfmlMath::intersect(
            p._origin_px,
            p._pos_px + sf::Vector2f{0.0f, 1000.0f},
            edge._p1_px,
            edge._p2_px
         );

         if (intersection.has_value())
         {
            p._intersections.push_back(intersection.value().y);
         }
      }

      std::sort(p._intersections.begin(), p._intersections.end());
   };

   // set up the rain area like below:
   //
   //   +- - - - +----------------------+- - - - +
   //   :        :                      :        :
   //   :        :                      :        :
   //   +- - - - +----------------------+- - - - +
   //   :        |                      |        :
   //   :        |                      |        :
   //   :        |                      |        :
   //   :        |                      |        :
   //   :        |                      |        :
   //   +- - - - +----------------------+- - - - +
   //   :        :                      :        :
   //   :        :                      :        :
   //   +- - - - +----------------------+- - - - +
   auto player_position = Player::getCurrent()->getPixelPositionf();
   _clip_rect.left   = player_position.x - _screen.width;
   _clip_rect.top    = player_position.y - _screen.height;
   _clip_rect.height = _screen.height * 2;
   _clip_rect.width  = _screen.width * 2;

   // initialize all drops if that hasn't been done yet
   if (!_initialized)
   {
      for (auto& p : _drops)
      {
         const auto sprite_index = std::rand() % 4;

         p._sprite.setTextureRect({static_cast<int32_t>(sprite_index) * 11, 0, 11, 96});
         p._sprite.setOrigin(6, 0);
         p._pos_px.x = _clip_rect.left + std::rand() % static_cast<int32_t>(_clip_rect.width);
         p._pos_px.y = _clip_rect.top + std::rand() % static_cast<int32_t>(_clip_rect.height);
         p._age_s = (std::rand() % (static_cast<int32_t>(max_age_s * 10000))) * 0.0001f;
         p._dir_px.y = (std::rand() % 100) * randomize_factor_y + fixed_direction_y;
         update_colliding_edge(p);
      }

      _initialized = true;
   }

   auto fallthrough_index = 0;
   for (auto& p : _drops)
   {
      p._age_s += dt.asSeconds();

      if (p._age_s > 0.0f)
      {
         const auto step_width_px = p._dir_px * dt.asSeconds();
         p._pos_px += step_width_px;
         p._sprite.setPosition(p._pos_px);

         if (p._age_s > max_age_s)
         {
            p.reset(_clip_rect);
            update_colliding_edge(p);
         }
         else
         {
            if (_settings._fall_through_rate == 0 || (fallthrough_index % _settings._fall_through_rate) == 0)
            {
               // intersect rain drop with edges
               if (!p._intersections.empty())
               {
                  const auto& closest_point = p._intersections.front();

                  if (p._pos_px.y + 96 > p._intersections.at(0))
                  {
                     const sf::Vector2f hit_position{p._pos_px.x, closest_point};

                     DropHit hit;
                     hit._sprite.setTexture(*_texture);
                     hit._sprite.setPosition(hit_position);
                     hit._pos_px = hit_position;
                     _hits.push_back(hit);

                     p.reset(_clip_rect);

                     update_colliding_edge(p);
                  }
               }
            }
         }
      }

      fallthrough_index++;
   }

   if (_settings._collide)
   {
      // only refresh the box2d information every 30 frames
      if (_edges.empty() || _refresh_surface_counter == 30)
      {
         determineRainSurfaces();
         _refresh_surface_counter = 0;
      }

      _refresh_surface_counter++;

      // update hit and erase those that are too old
      _hits.erase(
         std::remove_if(
            _hits.begin(),
            _hits.end(),
            [dt](auto& hit) {
               hit._age_s +=  dt.asSeconds();
               hit._sprite.setOrigin(5, 11);
               hit._sprite.setTextureRect({
                     11 * std::min(3, static_cast<int32_t>(hit._age_s * 10.0f)),
                     96,
                     11,
                     12
                  }
               );
               return hit._age_s > 1.0f;
            }
         ),
         _hits.end()
      );
   }
}


void RainOverlay::RainDrop::reset(const sf::FloatRect& rect)
{
   _age_s = - (std::rand() % 10000) * 0.0001f;

   const auto x = std::rand() % static_cast<int32_t>(rect.width);

   _pos_px.x = static_cast<float>(rect.left + x);
   _pos_px.y = rect.top;

   _origin_px = _pos_px;
}


std::vector<b2Body*> retrieveBodiesOnScreen(const std::shared_ptr<b2World>& world, const sf::FloatRect& screen)
{
   b2AABB aabb;

   const auto l = screen.left;
   const auto r = screen.left + screen.width;
   const auto t = screen.top;
   const auto b = screen.top + screen.height;

   aabb.upperBound = vecS2B({std::max(l, r), std::max(b, t)});
   aabb.lowerBound = vecS2B({std::min(l, r), std::min(b, t)});

   return WorldQuery::queryBodies(world, aabb);
}


void RainOverlay::determineRainSurfaces()
{
   _edges.clear();

   auto level = Level::getCurrentLevel();

   std::vector<b2Body*> bodies = retrieveBodiesOnScreen(level->getWorld(), _screen);

   for (auto body : bodies)
   {
      if (
            body->GetType() == b2_dynamicBody
         || body->GetType() == b2_kinematicBody
         || body->GetType() == b2_staticBody
      )
      {
         auto f = body->GetFixtureList();
         while (f)
         {
            auto next = f->GetNext();
            auto shape = f->GetShape();

            switch (shape->GetType())
            {
               case b2Shape::e_chain:
               {
                  const auto offset = body->GetPosition();
                  auto chain = dynamic_cast<b2ChainShape*>(shape);

                  for (auto i = 0; i< chain->m_count - 1; i++)
                  {
                     auto v1_m = offset + chain->m_vertices[i];
                     auto v2_m = offset + chain->m_vertices[i + 1];

                     // filter out lines where the normal is facing down
                     if ((v2_m - v1_m).x < 0.0f)
                     {
                        // re-enable to debug colliding surfaces
                        // b2Color red(1,0,0);
                        // DebugDraw::drawLine(target, v1_m, v2_m, red);
                        Edge edge{vecB2S(v1_m), vecB2S(v2_m)};
                        _edges.push_back(edge);
                     }
                  }
                  break;
               }

               default:
               {
                  break;
               }
            }

            f = next;
         }
      }
   }
}


void RainOverlay::setSettings(const RainSettings& settings)
{
   _settings = settings;
}
