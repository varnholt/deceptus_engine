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

   mTexture = TexturePool::getInstance().get(texture);

   auto xi = 0;
   auto yi = 0;
   auto i = 0;
   auto x = 0;
   auto y = 0;
   auto w = 0;
   auto h = 0;

   std::string line;
   std::ifstream uvFile(uv);
   if (uvFile.is_open())
   {
      while (uvFile.good())
      {
         std::getline(uvFile, line);
         std::sscanf(line.c_str(), "%d;%d;%d;%d;%d", &i, &x, &y, &w, &h);

         // std::cout << "x: " << x << " y: " << y << " w: " << w << " h: " << h << std::endl;

         sf::Sprite sprite;
         sprite.setPosition(static_cast<float>(x - 5), static_cast<float>(y - 6));
         sprite.setTexture(*mTexture);
         sprite.setTextureRect({xi, yi, w, h});
         mSprites.push_back(sprite);

         xi += w;
         if (xi == static_cast<int32_t>(mTexture->getSize().x))
         {
            xi = 0;
            yi += h;
         }
      }

      uvFile.close();

      std::cout << "[x] loaded " << mSprites.size() << " ao sprites" << std::endl;
   }
   else
   {
      std::cout << "AmbientOcclusion::load: unable to open uv file: " << uv << std::endl;
   }
}


// optimize this!
// just check if the sprite is outside the current screen
// also group sprites to blocks of equal positions

void AmbientOcclusion::draw(sf::RenderTarget& window)
{
   const auto pos = Player::getCurrent()->getPixelPositionf();

   for (auto& sprite : mSprites)
   {
      auto diff = SfmlMath::lengthSquared(pos - sprite.getPosition());

      if (diff > 300000)
         continue;

      window.draw(sprite, {sf::BlendAlpha});
   }
}
