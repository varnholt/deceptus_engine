#include "lazytexture.h"
#include <ranges>
#include <stop_token>
#include "framework/tools/log.h"

namespace
{
constexpr int chunk_load_threshold = 2;  // load texture if player is within this many chunks
}

LazyTexture::LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk>& _texture_chunks)
    : _texture_path(texture_path), _texture_chunks(_texture_chunks), _loaded(false)
{
}

void LazyTexture::update(const Chunk& player_chunk)
{
   // check if the texture should be loaded
   const auto should_be_loaded = std::ranges::any_of(
      _texture_chunks,
      [&](const auto& chunk)
      { return std::abs(player_chunk._x - chunk._x) < chunk_load_threshold && std::abs(player_chunk._y - chunk._y) < chunk_load_threshold; }
   );

   if (should_be_loaded || _texture_chunks.empty())
   {
      if (!_loaded && !_loading.test_and_set())
      {
         loadTexture();
      }
   }
   else
   {
      if (_loaded)
      {
         unloadTexture();
      }
   }
}

void LazyTexture::loadTexture()
{
   Log::Info() << "loading " << _texture_path;
   _loading_thread = std::jthread(
      [this](std::stop_token token)
      {
         auto texture_to_load = std::make_unique<sf::Texture>();
         if (texture_to_load->loadFromFile(_texture_path.string()))
         {
            std::lock_guard lock(_mutex);
            _texture = std::move(texture_to_load);
            _loaded = true;
         }

         // loading is complete
         _loading.clear();
      }
   );
}

void LazyTexture::unloadTexture()
{
   Log::Info() << "unloading " << _texture_path;

   std::lock_guard lock(_mutex);
   _texture.reset();
   _loaded = false;
}

std::optional<std::reference_wrapper<const sf::Texture>> LazyTexture::getTexture() const
{
   if (!_loaded)
   {
      return std::nullopt;
   }

   std::lock_guard lock(_mutex);
   return std::cref(*_texture);
}
