#pragma once

#include <SFML/Graphics.hpp>
#include "framework/tools/resourcepool.h"

/*! \brief specialized resource pool for sf::Texture.
 */
class TexturePool : public ResourcePool<sf::Texture>
{
public:
   static TexturePool& getInstance()
   {
      static TexturePool instance;
      return instance;
   }

protected:
   bool loadResource(sf::Texture& texture, const std::filesystem::path& path) const override
   {
      return texture.loadFromFile(path.string());
   }

   size_t computeResourceSize(const sf::Texture& texture) const override
   {
      const auto size = texture.getSize();
      return size.x * size.y * 4;  // rgba
   }
};
