#include "rainoverlay.h"

#include "debugdraw.h"
#include "framework/math/sfmlmath.h"
#include "game/gameconfiguration.h"
#include "level.h"
#include "player/player.h"
#include "texturepool.h"
#include "worldquery.h"


#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{

static const auto drop_count = 500;
static const auto max_age_s = 0.5f;
static const auto velocity_factor = 30.0f;
static const auto randomize_factor_x = 0.0f;
static const auto randomize_factor_y = 0.02f;
static const auto fixed_direction_x = 0.0f;
static const auto fixed_direction_y = 40.0f;


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

   for (auto a = 0; a < drop_count; a++)
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
      target.draw(d._sprite, blend_mode);
   }

   determineRainSurfaces(target);

   for (auto& hit : _hits)
   {
      DebugDraw::drawPoint(target, vecS2B(hit._pos_px), {255, 0, 0});
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

   auto player_position = Player::getCurrent()->getPixelPositionf();
   _clip_rect.left   = player_position.x - _screen.width;
   _clip_rect.top    = player_position.y - _screen.height;
   _clip_rect.height = _screen.height * 2;
   _clip_rect.width  = _screen.width * 2;

   if (!_initialized)
   {
      for (auto& p : _drops)
      {
         const auto sprite_index = std::rand() % 4;

         p._sprite.setTextureRect({static_cast<int32_t>(sprite_index) * 11, 0, 11, 96});
         p._pos_px.x = _clip_rect.left + std::rand() % static_cast<int32_t>(_clip_rect.width);
         p._pos_px.y = _clip_rect.top + std::rand() % static_cast<int32_t>(_clip_rect.height);
         p._age_s = (std::rand() % (static_cast<int32_t>(max_age_s * 10000))) * 0.0001f;

         p.resetDirection();
      }

      _initialized = true;
   }

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
   //

   for (auto& p : _drops)
   {
      p._age_s += dt.asSeconds();

      if (p._age_s > 0.0f)
      {
         auto p_prev_px = p._pos_px;
         p._pos_px += p._dir_px * dt.asSeconds() * velocity_factor;
         p._sprite.setPosition(p._pos_px);

         if (p._age_s > max_age_s)
         {
            p.resetPosition(_clip_rect);
         }
         else
         {
            // intersect rain drop with edges
            for (auto& edge : _edges)
            {
               auto intersection = SfmlMath::intersect(p_prev_px, p._pos_px, edge._p1_px, edge._p2_px);
               if (intersection.has_value())
               {
                  DropHit hit;
                  hit._pos_px = p._pos_px;
                  _hits.push_back(hit);

                  p.resetPosition(_clip_rect);
               }
            }
         }
      }
   }

   // update hit and erase those that are too old
   _hits.erase(
      std::remove_if(
         _hits.begin(),
         _hits.end(),
         [dt](auto& hit) {
             hit._age_s +=  dt.asSeconds();
             return hit._age_s > 4.0f;
         }
      ),
      _hits.end()
   );
}


void RainOverlay::RainDrop::resetDirection()
{
   auto rand_x = (std::rand() % 100) * randomize_factor_x;
   auto rand_y = (std::rand() % 100) * randomize_factor_y;

   _dir_px.x = rand_x + fixed_direction_x;
   _dir_px.y = rand_y + fixed_direction_y;
}


void RainOverlay::RainDrop::resetPosition(const sf::FloatRect& rect)
{
   _age_s = -(std::rand() % (static_cast<int32_t>(max_age_s * 10000))) * 0.0001f;

   const auto x = std::rand() % static_cast<int32_t>(rect.width);

   _pos_px.x = static_cast<float>(rect.left + x);
   _pos_px.y = static_cast<float>(rect.top);
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


void RainOverlay::determineRainSurfaces(sf::RenderTarget& target)
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
         // draw fixtures
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
