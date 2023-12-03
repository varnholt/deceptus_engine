#pragma once

#include "parallaxsettings.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <optional>

struct TmxElement;

struct ImageLayer
{
   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::BlendMode _blend_mode = sf::BlendAdd;
   int32_t _z_index = 0;

   std::optional<ParallaxSettings> _parallax_settings;
   std::vector<std::string> _restrict_to_rooms;

   static std::shared_ptr<ImageLayer> deserialize(const std::shared_ptr<TmxElement>& element, const std::filesystem::path& level_path);
};

