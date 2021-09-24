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
   std::vector<sf::Sprite> _sprites;

};

