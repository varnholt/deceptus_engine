#include "lazytexture.h"

#include <algorithm>
#include <ranges>
#include <stop_token>
#include "framework/tools/log.h"

namespace
{
constexpr int chunk_load_threshold = 2;
}

LazyTexture::LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk>& texture_chunks)
    : _texture_path(texture_path), _texture_chunks(texture_chunks)
{
}

void LazyTexture::update(const Chunk& player_chunk)
{
   const auto should_be_loaded = std::ranges::any_of(
      _texture_chunks,
      [&](const auto& chunk)
      { return std::abs(player_chunk._x - chunk._x) < chunk_load_threshold && std::abs(player_chunk._y - chunk._y) < chunk_load_threshold; }
   );

   // if the texture does not define any chunks, we always need to load the texture.
   // this defeats the point of the lazy texture a bit but that's what it is.
   if (should_be_loaded || _texture_chunks.empty())
   {
      // texture is only touched in the main thread, safe to keep unmutexed
      if (!_texture && !_loading.test_and_set())
      {
         // load texture from disk (jthread)
         loadTexture();
      }
      else
      {
         // shove texture into gpu (main thread)
         uploadTexture();
      }
   }
   else
   {
      // texture is no longer needed, throw it out
      if (_texture)
      {
         unloadTexture();
      }
   }
}

void LazyTexture::loadTexture()
{
   // Log::Info() << "loading " << _texture_path;

   _loading_thread = std::jthread(
      [this](std::stop_token)
      {
         auto image = std::make_unique<sf::Image>();
         if (image->loadFromFile(_texture_path.string()))
         {
            std::lock_guard lock(_mutex);
            _pending_image = std::move(image);
            _image_ready = true;
         }
      }
   );
}

void LazyTexture::uploadTexture()
{
   if (!_image_ready.load())
   {
      return;
   }

   std::lock_guard lock(_mutex);
   if (_pending_image)
   {
      _texture = std::make_shared<sf::Texture>();
      if (_texture->loadFromImage(*_pending_image))
      {
         _pending_image.reset();
         _image_ready = false;
         // Log::Info() << "uploaded texture " << _texture_path << " (" << _texture->getSize().x << ", " << _texture->getSize().y << ")";
      }
      else
      {
         _texture.reset();
         Log::Warning() << "failed to upload texture " << _texture_path;
      }

      _loading.clear();
   }
}

void LazyTexture::unloadTexture()
{
   // Log::Info() << "unloading " << _texture_path;

   // safe to go without mutex since it'll only unload once the texture was uploaded to gpu
   _texture.reset();
   _pending_image.reset();
   _image_ready = false;
}

const std::shared_ptr<sf::Texture>& LazyTexture::getTexture() const
{
   return _texture;
}
