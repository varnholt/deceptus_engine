#pragma once

#include "parallaxsettings.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <optional>

struct TmxElement;

struct ImageLayer
{
   void updateView(float level_view_x, float level_view_y, float view_width, float view_height);
   void resetView(float view_width, float view_height);
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   static std::shared_ptr<ImageLayer> deserialize(const std::shared_ptr<TmxElement>& element, const std::filesystem::path& level_path);

   sf::Sprite _sprite;
   std::shared_ptr<sf::Texture> _texture;
   sf::BlendMode _blend_mode = sf::BlendAlpha;
   int32_t _z_index = 0;

   sf::View _parallax_view;
   std::optional<ParallaxSettings> _parallax_settings;
   std::vector<std::string> _restrict_to_rooms;
};
