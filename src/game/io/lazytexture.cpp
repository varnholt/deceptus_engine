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

LazyTexture::~LazyTexture()
{
   Log::Info() << "destructor";
}

void LazyTexture::update(const Chunk& player_chunk)
{
   const auto should_be_loaded = std::ranges::any_of(
      _texture_chunks,
      [&](const auto& chunk)
      { return std::abs(player_chunk._x - chunk._x) < chunk_load_threshold && std::abs(player_chunk._y - chunk._y) < chunk_load_threshold; }
   );

   if (should_be_loaded || _texture_chunks.empty())
   {
      if (!_texture && !_loading.test_and_set())
      {
         loadTexture();
      }

      uploadIfReady();
   }
   else
   {
      if (_texture)
      {
         unloadTexture();
      }
   }
}

void LazyTexture::loadTexture()
{
   Log::Info() << "loading " << _texture_path;

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

         _loading.clear();
      }
   );
}

void LazyTexture::uploadIfReady()
{
   if (!_image_ready.load())
      return;

   std::lock_guard lock(_mutex);
   if (_pending_image)
   {
      _texture = std::make_shared<sf::Texture>();
      if (_texture->loadFromImage(*_pending_image))
      {
         _pending_image.reset();
         _image_ready = false;

         Log::Info() << "uploaded texture " << _texture_path;
      }
      else
      {
         _texture.reset();
         Log::Info() << "failed to upload texture " << _texture_path;
      }
   }
}

void LazyTexture::unloadTexture()
{
   Log::Info() << "unloading " << _texture_path;

   std::lock_guard lock(_mutex);
   _texture.reset();
   _pending_image.reset();
   _image_ready = false;
}

std::shared_ptr<const sf::Texture> LazyTexture::getTexture() const
{
   // std::lock_guard lock(_mutex);
   return _texture;
}
