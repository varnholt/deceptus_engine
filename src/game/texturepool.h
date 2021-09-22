#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

#include <SFML/Graphics.hpp>


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

