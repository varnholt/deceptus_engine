#pragma once

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>

class AmbientOcclusion
{
public:

   AmbientOcclusion() = default;

   void load(
      const std::filesystem::path& path,
      const std::string& aoBaseFilename
   );

   void draw(sf::RenderTarget &window);

private:

   std::shared_ptr<sf::Texture> _texture;

   // for unoptimized approach
   // std::vector<sf::Sprite> _sprites;

   std::map<int32_t, std::map<int32_t, std::vector<sf::Sprite>>> _sprite_map;
};

