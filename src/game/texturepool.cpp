#include "texturepool.h"

#include <iostream>


TexturePool& TexturePool::getInstance()
{
   static TexturePool __instance;
   return __instance;
}


std::shared_ptr<sf::Texture> TexturePool::get(const std::filesystem::path& path)
{
   std::lock_guard<std::mutex> hold(_mutex);

   const auto key = path.string();
   auto sp = _pool[key].lock();
   if (!sp)
   {
      _pool[key] = sp = std::make_shared<sf::Texture>();
      sp->loadFromFile(key);
   }

   // std::cout << computeSize() << std::endl;

   return sp;
}


size_t TexturePool::computeSize() const
{
   size_t size = 0;

   for (const auto& [key, value] : _pool)
   {
      auto texture = value.lock();
      if (texture)
      {
         size += (texture->getSize().x * texture->getSize().y * 4);
      }
      else
      {
         // std::cout << key << " has been removed" << std::endl;
      }
   }

   return size;
}

