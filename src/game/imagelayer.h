#pragma once

#include <SFML/Graphics.hpp>

#include <filesystem>

struct TmxElement;

struct ImageLayer
{
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::BlendMode _blend_mode = sf::BlendAdd;
   int32_t _z_index = 0;

   static std::shared_ptr<ImageLayer> deserialize(
      TmxElement* element,
      const std::filesystem::path& level_path
   );
};

