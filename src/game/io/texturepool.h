#pragma once

#include <SFML/Graphics.hpp>
#include <future>
#include <filesystem>
#include <memory>
#include "framework/tools/resourcepool.h"
#include "framework/tools/asynctexturepool.h"

/*! \brief specialized resource pool for sf::Texture with async loading support.
 */
class TexturePool : public ResourcePool<sf::Texture>
{
public:
   static TexturePool& getInstance()
   {
      static TexturePool instance;
      return instance;
   }

   // Synchronous texture loading (existing functionality) with timing
   std::shared_ptr<sf::Texture> get(const std::filesystem::path& path)
   {
      std::lock_guard<std::mutex> hold(m_mutex);

      const auto key = path.string();
      auto sp = m_pool[key].lock();
      if (!sp)
      {
         sp = std::make_shared<sf::Texture>();

         // Check if the file exists before attempting to load
         if (!std::filesystem::exists(path)) {
            Log::Warning() << "texture file does not exist: " << path;
            // Still store the failed texture in the pool to prevent repeated attempts
            m_pool[key] = sp;
            return sp; // Return the empty texture
         }

         // Time the texture loading operation
         auto start_time = std::chrono::high_resolution_clock::now();
         if (!loadResource(*sp, path))
         {
            Log::Warning() << "error loading texture: " << path;
         }
         auto end_time = std::chrono::high_resolution_clock::now();

         auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
         if (load_duration.count() >= 100) {
             Log::Info() << "Synchronous disk load time for " << path << ": " << load_duration.count() << " ms (Main thread - may cause hiccups)";
         }

         m_pool[key] = sp;
      }

      return sp;
   }

   // Asynchronous texture loading
   AsyncTextureHandle getAsync(const std::filesystem::path& path)
   {
       return AsyncTexturePool::getInstance().getAsync(path);
   }

   // Check if async texture is loaded
   bool isLoaded(const std::filesystem::path& path) const
   {
       return AsyncTexturePool::getInstance().isLoaded(path);
   }

   // Try to get texture without blocking
   std::shared_ptr<sf::Texture> tryGet(const std::filesystem::path& path)
   {
       return AsyncTexturePool::getInstance().tryGet(path);
   }

   // Update async texture system (call regularly in main loop)
   void update()
   {
       AsyncTexturePool::getInstance().update();
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
