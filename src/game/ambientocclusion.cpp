#include "ambientocclusion.h"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>

#include "framework/math/sfmlmath.h"
#include "framework/tools/log.h"
#include "player/player.h"
#include "texturepool.h"


void AmbientOcclusion::load(
  const std::filesystem::path& path,
  const std::string& base_filename
)
{
   const auto ao_base_filename = base_filename + "_ao_tiles.png";
   const auto texture = (path / ao_base_filename).string();
   const auto uv = (path / (base_filename + "_ao_tiles.uv")).string();

   if (!std::filesystem::exists(texture))
   {
      Log::Error() << "need to create an ambient occlusion map (" << ao_base_filename << ")";
      return;
   }

   _texture = TexturePool::getInstance().get(texture);

   auto xi = 0;
   auto yi = 0;
   auto i = 0;
   auto x = 0;
   auto y = 0;
   auto w = 0;
   auto h = 0;

   auto group_x = 0;
   auto group_y = 0;

   std::string line;
   std::ifstream uv_file(uv);
   if (uv_file.is_open())
   {
      while (uv_file.good())
      {
         std::getline(uv_file, line);
         std::sscanf(line.c_str(), "%d;%d;%d;%d;%d", &i, &x, &y, &w, &h);

         sf::Sprite sprite;
         sprite.setPosition(static_cast<float>(x - 5), static_cast<float>(y - 6));
         sprite.setTexture(*_texture);
         sprite.setTextureRect({xi, yi, w, h});

         group_x = (x >> 8);
         group_y = (y >> 8);
         _sprite_map[group_y][group_x].push_back(sprite);

         xi += w;
         if (xi == static_cast<int32_t>(_texture->getSize().x))
         {
            xi = 0;
            yi += h;
         }
      }

      uv_file.close();
   }
   else
   {
      Log::Error() << "AmbientOcclusion::load: unable to open uv file: " << uv;
   }
}


void AmbientOcclusion::draw(sf::RenderTarget& window)
{
   const auto& pos_i = Player::getCurrent()->getPixelPositionInt();
   const int32_t bx = pos_i.x >> 8;
   const int32_t by = pos_i.y >> 8;

   // increase the range if you have smaller AO block sizes
   constexpr int32_t rxl = 4;
   constexpr int32_t rxr = 4;

   constexpr int32_t ryl = 3;
   constexpr int32_t ryr = 3;

   for (auto y = by - ryl; y < by + ryr; y++)
   {
      const auto& y_it = _sprite_map.find(y);
      if (y_it == _sprite_map.end())
      {
         continue;
      }

      for (auto x = bx - rxl; x < bx + rxr; x++)
      {
         const auto& x_it = y_it->second.find(x);
         if (x_it == y_it->second.end())
         {
            continue;
         }

         // Log::Info() << "draw " << x_it->second.size() << " sprites";

         for (const auto& sprite : x_it->second)
         {
            window.draw(sprite, {sf::BlendAlpha});
         }
      }
   }
}
