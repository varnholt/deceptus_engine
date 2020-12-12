#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>

struct TmxElement;

struct ImageLayer
{
   sf::Sprite mSprite;
   std::shared_ptr<sf::Texture> mTexture;
   sf::BlendMode mBlendMode = sf::BlendAdd;
   int32_t mZ = 0;

   static std::shared_ptr<ImageLayer> deserialize(
      TmxElement* element,
      const std::filesystem::path& levelPath
   );
};

