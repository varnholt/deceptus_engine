#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

#include <SFML/Graphics.hpp>


/*! \brief A texture cache implementation
 *         It holds weak pointers to textures so they get deleted once no longer needed.
 *
 *  A shared_ptr is retrieved by just passing in a path to the texture.
 */
class TexturePool
{

public:

   static TexturePool& getInstance();
   std::shared_ptr<sf::Texture> get(const std::filesystem::path&);

   size_t computeSize() const;


private:

   TexturePool() = default;

   std::mutex _mutex;
   std::map<std::string, std::weak_ptr<sf::Texture>> _pool;
};

