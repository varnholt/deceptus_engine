#include "rainoverlay.h"

#include "game/gameconfiguration.h"

#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{
   static const auto dropCount = 500;
   static const auto velocity_factor = 30.0f;
   static const auto width_stretch_factor = 1.5f;
   static const auto start_offset_x = -100.0f;
   static const auto start_offset_y = -20.0f;
   static const auto randomize_factor_x = 0.0f;
   static const auto randomize_factor_y = 0.02f;
   static const auto randomize_factor_length = 0.04f;
   static const auto fixed_direction_x = 4.0f;
   static const auto fixed_direction_y = 10.0f;
   static const auto fixed_length = 0.0f;
}


RainOverlay::RainOverlay()
{
   std::srand(static_cast<uint32_t>(std::time(nullptr))); // use current time as seed for random generator

   for (auto a = 0; a < dropCount; a++)
   {
      _drops.push_back(RainDrop());
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

   static const auto color = sf::Color{174, 194, 224, 30};

   sf::Vertex line[2];

   for (auto& d : _drops)
   {
      line[0] = sf::Vertex{sf::Vector2f{d._pos.x, d._pos.y}, color};
      line[1] = sf::Vertex{sf::Vector2f{d._pos.x + d._length * d._dir.x, d._pos.y + d._length * d._dir.y}, color};

      target.draw(line, 2, sf::Lines);
   }
}


void RainOverlay::update(const sf::Time& dt)
{
   // screen not initialized yet
   if (_screen.width == 0)
   {
      return;
   }

   for (auto& p : _drops)
   {
      p._pos += p._dir * dt.asSeconds() * velocity_factor;

      if (!_screen.contains(p._pos))
      {
         p.reset(_screen);
      }
   }
}


void RainOverlay::RainDrop::reset(const sf::FloatRect& rect)
{
   const auto x = std::rand() % static_cast<int32_t>(rect.width);
   const auto y = std::rand() % static_cast<int32_t>(rect.height);

   _pos.x = static_cast<float>(rect.left + x);
   _pos.y = static_cast<float>(rect.top + y);

   _length = (std::rand() % 100) * randomize_factor_length + fixed_length;

   auto rand_x = (std::rand() % 100) * randomize_factor_x;
   auto rand_y = (std::rand() % 100) * randomize_factor_y;

   _dir.x = rand_x + fixed_direction_x;
   _dir.y = rand_y + fixed_direction_y;
}


