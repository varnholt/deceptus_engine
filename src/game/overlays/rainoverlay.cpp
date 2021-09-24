#include "rainoverlay.h"

#include "game/gameconfiguration.h"

#include <cstdlib>
#include <iostream>
#include <ctime>


namespace
{
   // todo: resolve static initialization fiasco
   // https://isocpp.org/wiki/faq/ctors#static-init-order
   static const auto w = 640; // GameConfiguration::getInstance().mViewWidth;
   static const auto h = 360; // GameConfiguration::getInstance().mViewHeight;

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

   _render_texture.create(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
}


void RainOverlay::draw(sf::RenderTarget& window, sf::RenderStates /*states*/)
{
   static const auto color = sf::Color{174, 194, 224, 30};

   _render_texture.clear();

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   sf::Vertex line[2];

   for (auto& d : _drops)
   {
      line[0] = sf::Vertex{sf::Vector2f{d._pos.x, d._pos.y}, color};
      line[1] = sf::Vertex{sf::Vector2f{d._pos.x + d._length * d._dir.x, d._pos.y + d._length * d._dir.y}, color};

      _render_texture.draw(line, 2, sf::Lines);
   }

   _render_texture.setView(view);
   _render_texture.display();

   auto sprite = sf::Sprite(_render_texture.getTexture());
   window.draw(sprite, sf::BlendMode{sf::BlendAdd});
}


void RainOverlay::update(const sf::Time& dt)
{
   for (auto& p : _drops)
   {
      p._pos += p._dir * dt.asSeconds() * velocity_factor;

      if (p._pos.x > w || p._pos.y > h)
      {
         p._pos.x = static_cast<float>(std::rand() % w) * width_stretch_factor + start_offset_x;
         p._pos.y = start_offset_y;
      }
   }
}


RainOverlay::RainDrop::RainDrop()
{
   _pos.x = static_cast<float>(std::rand() % w);
   _pos.y = static_cast<float>(std::rand() % h);

   _length = (std::rand() % 100) * randomize_factor_length + fixed_length;

   auto rand_x = (std::rand() % 100) * randomize_factor_x;
   auto rand_y = (std::rand() % 100) * randomize_factor_y;

   _dir.x = rand_x + fixed_direction_x;
   _dir.y = rand_y + fixed_direction_y;
}


