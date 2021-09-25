#include "ambientocclusion.h"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>

#include "framework/math/sfmlmath.h"
#include "player/player.h"
#include "texturepool.h"


void AmbientOcclusion::load(
  const std::filesystem::path& path,
  const std::string &aoBaseFilename
)
{
  auto texture = (path / (aoBaseFilename + "_ao_tiles.png")).string();
  auto uv = (path / (aoBaseFilename + "_ao_tiles.uv")).string();

   if (!std::filesystem::exists(texture))
   {
      std::cerr << "[!] need to create an ambient occlusion map (" << texture << ")" << std::endl;
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

   std::string line;
   std::ifstream uv_file(uv);
   if (uv_file.is_open())
   {
      while (uv_file.good())
      {
         std::getline(uv_file, line);
         std::sscanf(line.c_str(), "%d;%d;%d;%d;%d", &i, &x, &y, &w, &h);

         // std::cout << "x: " << x << " y: " << y << " w: " << w << " h: " << h << std::endl;

         sf::Sprite sprite;
         sprite.setPosition(static_cast<float>(x - 5), static_cast<float>(y - 6));
         sprite.setTexture(*_texture);
         sprite.setTextureRect({xi, yi, w, h});
         _sprites.push_back(sprite);

         xi += w;
         if (xi == static_cast<int32_t>(_texture->getSize().x))
         {
            xi = 0;
            yi += h;
         }
      }

      uv_file.close();

      std::cout << "[x] loaded " << _sprites.size() << " ao sprites" << std::endl;
   }
   else
   {
      std::cout << "AmbientOcclusion::load: unable to open uv file: " << uv << std::endl;
   }
}


// optimize this!
// just check if the sprite is outside the current screen
// also group sprites to blocks of equal positions

#include "level.h"

void AmbientOcclusion::draw(sf::RenderTarget& window)
{
   const auto pos = Player::getCurrent()->getPixelPositionf();

//   const auto& screen_rect = Level::getCurrentLevel()->getLevelView();

   for (auto& sprite : _sprites)
   {

//      const auto& sprite_rect = sprite.getGlobalBounds();
//      if (!screen_rect.get()->getViewport().intersects(sprite_rect))
//      {
//         continue;
//      }

      auto diff = SfmlMath::lengthSquared(pos - sprite.getPosition());

      if (diff > 300000)
      {
         continue;
      }

      window.draw(sprite, {sf::BlendAlpha});
   }
}
