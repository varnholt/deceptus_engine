#include "rainoverlay.h"

#include "game/gameconfiguration.h"
#include "player/player.h"
#include "texturepool.h"

#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{
static const auto drop_count = 2000;
static const auto max_age_s = 3.0f;
static const auto velocity_factor = 30.0f;
static const auto randomize_factor_x = 0.0f;
static const auto randomize_factor_y = 0.02f;
static const auto fixed_direction_x = 4.0f;
static const auto fixed_direction_y = 10.0f;
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

   sf::Vertex line[2];

   for (auto& d : _drops)
   {
      target.draw(d._sprite);
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
         p._pos_px.x = _clip_rect.left + std::rand() % static_cast<int32_t>(_clip_rect.width);
         p._pos_px.y = _clip_rect.top + std::rand() % static_cast<int32_t>(_clip_rect.height);
         p._age_s = (std::rand() % (static_cast<int32_t>(max_age_s) * 1000)) * 0.001f;
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
      p._pos_px += p._dir_px * dt.asSeconds() * velocity_factor;

      p._age_s += dt.asSeconds();
      const auto sprite_index = std::min(p._age_s * 3.0f, 3.0f);

      p._sprite.setPosition(p._pos_px);
      p._sprite.setTextureRect({
            static_cast<int32_t>(sprite_index) * 11,
            0,
            11,
            12
         }
      );

      if (p._age_s > max_age_s)
      {
         p.resetPosition(_clip_rect);
      }
   }
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
   _age_s = 0.0f;

   const auto x = std::rand() % static_cast<int32_t>(rect.width);
   const auto y = std::rand() % static_cast<int32_t>(rect.height / 3);

   _pos_px.x = static_cast<float>(rect.left + x);
   _pos_px.y = static_cast<float>(rect.top + y);
}


