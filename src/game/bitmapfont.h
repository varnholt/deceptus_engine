#pragma once

#include <cstdint>
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>


struct BitmapFont
{
   BitmapFont() = default;
   void load(const std::string& texture, const std::string &map);
   std::vector<std::shared_ptr<sf::IntRect>> getCoords(const std::string& text);

   void draw(
      sf::RenderTarget& window,
      const std::vector<std::shared_ptr<sf::IntRect>>& coords,
      int32_t x = 0,
      int32_t y = 0
   );

   std::shared_ptr<sf::Texture> mTexture;
   sf::Sprite mSprite;
   std::map<char, std::shared_ptr<sf::IntRect>> mMap;
   int32_t mCharWidth = 0;
   int32_t mCharHeight = 0;
   int32_t mTextWidth = 0;
};

