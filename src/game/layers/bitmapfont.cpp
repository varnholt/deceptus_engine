#include "bitmapfont.h"

#include "framework/tools/log.h"
#include "game/io/texturepool.h"

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

void BitmapFont::load(const std::string& texturePath, const std::string& mapPath)
{
   _texture = TexturePool::getInstance().get(texturePath);
#ifdef __EMSCRIPTEN__
   _sprite = std::make_unique<sf::Sprite>();
#else
   _sprite = std::make_unique<sf::Sprite>(*_texture);
#endif

   std::ifstream file(mapPath);

   auto i = 0;
   std::string line;
   std::string font;
   std::vector<int32_t> vals;
   while (std::getline(file, line))
   {
      if (i == 0)
      {
         auto tmp = 0;
         std::stringstream ss(line);
         while (ss >> tmp)
         {
            vals.push_back(tmp);
            if (ss.peek() == ',' || ss.peek() == ';')
            {
               ss.ignore();
            }
         }
      }
      else
      {
         font += line;
      }
      i++;
   }

   if (vals.size() == 2)
   {
      _char_width = vals[0];
      _char_height = vals[1];
   }
   else
   {
      Log::Error() << "font map fucked";
      return;
   }

   i = 0;
   auto x = 0;
   auto y = 0;
   for (auto c : font)
   {
      std::shared_ptr<sf::IntRect> rect = std::make_shared<sf::IntRect>();
      rect->position.x = x;
      rect->position.y = y;
      rect->size.x = _char_width;
      rect->size.y = _char_height;
      _map.insert(std::pair<char, std::shared_ptr<sf::IntRect>>(c, rect));

      x += _char_width;

      if (x == static_cast<int32_t>(_texture->getSize().x))
      {
         x = 0;
         y += _char_height;
      }

      i++;
   }
}

std::vector<std::shared_ptr<sf::IntRect>> BitmapFont::getCoords(const std::string& text)
{
   std::vector<std::shared_ptr<sf::IntRect>> coords;

   for (auto c : text)
   {
      auto it = _map.find(c);

      if (it != _map.end())
      {
         coords.push_back(it->second);
      }
   }

   return coords;
}

void BitmapFont::draw(
   sf::RenderTarget& window,
   const std::vector<std::shared_ptr<sf::IntRect>>& coords,
   int32_t x,
   int32_t y,
   const std::optional<sf::Color>& color,
   const sf::RenderStates& states
)
{
   sf::RenderStates draw_states = states;
   draw_states.texture = _texture.get();

   auto x_offset = 0;
   for (const auto& coord : coords)
   {
#ifdef __EMSCRIPTEN__
      _sprite->textureRect = sf::FloatRect{{static_cast<float>(coord->position.x), static_cast<float>(coord->position.y)}, {static_cast<float>(coord->size.x), static_cast<float>(coord->size.y)}};
      _sprite->position = {static_cast<float>(x + x_offset), static_cast<float>(y)};
      _sprite->color = color.value_or(sf::Color::White);
#else
      _sprite->setTextureRect(sf::IntRect({coord->position.x, coord->position.y}, {coord->size.x, coord->size.y}));
      _sprite->setPosition({static_cast<float>(x + x_offset), static_cast<float>(y)});
      _sprite->setColor(color.value_or(sf::Color::White));
#endif

      window.draw(*_sprite, draw_states);
      x_offset += _char_width;
   }

   _text_width = x_offset;
}
