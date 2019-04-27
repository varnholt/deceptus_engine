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


private:

   TexturePool() = default;

   static TexturePool sPool;

   std::mutex mMutex;
   std::map<std::string, std::weak_ptr<sf::Texture>> mPool;

};

