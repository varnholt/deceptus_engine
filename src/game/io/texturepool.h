#pragma once

#include <SFML/Graphics.hpp>
#include "framework/tools/resourcepool.h"

/// \brief singleton texture cache that loads sf::Texture resources through ResourcePool.
class TexturePool : public ResourcePool<sf::Texture>
{
public:
   /// \brief returns the global texture pool instance.
   /// \return shared singleton used for texture caching across the game.
   static TexturePool& getInstance()
   {
      static TexturePool instance;
      return instance;
   }

protected:
   /// \brief loads a texture from disk into the provided resource instance.
   /// \param texture texture object that receives file content.
   /// \param path texture file path.
   /// \return true when the texture file was loaded successfully.
   bool loadResource(sf::Texture& texture, const std::filesystem::path& path) const override
   {
      return texture.loadFromFile(path.string());
   }

   /// \brief estimates texture memory footprint in bytes for cache accounting.
   /// \param texture texture whose dimensions are used for estimation.
   /// \return estimated byte size using rgba8 layout.
   size_t computeResourceSize(const sf::Texture& texture) const override
   {
      const auto size = texture.getSize();
      return size.x * size.y * 4;  // rgba
   }
};
