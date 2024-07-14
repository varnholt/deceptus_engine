#include "game/io/texturepool.h"

#include "framework/tools/log.h"

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
      if (!sp->loadFromFile(key))
      {
         Log::Warning() << "error loading texture: " << path;
      }
   }

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
   }

   return size;
}

