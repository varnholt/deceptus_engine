#include "texturepool.h"

#include <iostream>


TexturePool TexturePool::sPool;


TexturePool& TexturePool::getInstance()
{
   return sPool;
}


std::shared_ptr<sf::Texture> TexturePool::get(const std::filesystem::path& path)
{
   std::lock_guard<std::mutex> hold(mMutex);

   const auto key = path.string();
   auto sp = mPool[key].lock();
   if (!sp)
   {
      mPool[key] = sp = std::make_shared<sf::Texture>();
      sp->loadFromFile(key);
   }

   return sp;
}


size_t TexturePool::computeSize() const
{
   size_t size = 0;

   for (const auto& [key, value] : mPool)
   {
      size += (value.lock()->getSize().x * value.lock()->getSize().y * 4);
   }

   return size;
}

