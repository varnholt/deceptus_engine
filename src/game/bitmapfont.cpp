#include "bitmapfont.h"

#include "texturepool.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>


void BitmapFont::load(
   const std::string& texturePath,
   const std::string& mapPath
)
{
   _texture = TexturePool::getInstance().get(texturePath);
   _sprite.setTexture(*_texture);

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
      std::cerr << "font map fucked" << std::endl;
      return;
   }

   i = 0;
   auto x = 0;
   auto y = 0;
   for (auto c : font)
   {
      std::shared_ptr<sf::IntRect> rect = std::make_shared<sf::IntRect>();
      rect->left = x;
      rect->top = y;
      rect->width = _char_width;
      rect->height = _char_height;
      _map.insert(std::pair<char,std::shared_ptr<sf::IntRect>>(c, rect));

      x += _char_width;

      if (x == static_cast<int32_t>(_texture->getSize().x))
      {
         x = 0;
         y += _char_height;
      }

      i++;
   }
}


std::vector<std::shared_ptr<sf::IntRect>> BitmapFont::getCoords(const std::string &text)
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
   const std::vector<std::shared_ptr<sf::IntRect> >& coords,
   int32_t x,
   int32_t y
)
{
   auto x_offset = 0;
   for (auto& coord : coords)
   {
      _sprite.setTextureRect(
         sf::IntRect(
            coord->left,
            coord->top,
            coord->width,
            coord->height
         )
      );

      _sprite.setPosition(
         static_cast<float>(x + x_offset),
         static_cast<float>(y)
      );

      window.draw(_sprite);
      x_offset += _char_width;
   }

   _text_width = x_offset;
}
