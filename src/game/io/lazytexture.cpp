#include "lazytexture.h"
#include "framework/tools/log.h"

#include <ranges>

namespace
{
constexpr int chunk_load_threshold = 2;
}

LazyTexture::LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk> texture_chunks)
    : _texture_path(texture_path), _texture_chunks(std::move(texture_chunks))
{
}

void LazyTexture::update(const Chunk& player_chunk)
{
   const bool should_be_loaded = std::ranges::any_of(
      _texture_chunks,
      [&](const auto& chunk)
      { return std::abs(player_chunk._x - chunk._x) < chunk_load_threshold && std::abs(player_chunk._y - chunk._y) < chunk_load_threshold; }
   );

   if (should_be_loaded && !_loaded && !_loading.exchange(true))
   {
      requestLoad();
   }

   // Process pending upload tasks (must be on main thread)
   {
      std::lock_guard lock(_upload_mutex);
      while (!_upload_tasks.empty())
      {
         auto task = std::move(_upload_tasks.front());
         _upload_tasks.pop();
         task();
      }
   }
}

void LazyTexture::requestLoad()
{
   Log::Info() << "loading image " << _texture_path;

   // Capture a weak_ptr so the lambda doesn't outlive the object
   std::weak_ptr<LazyTexture> self = shared_from_this();
   auto texture_path = _texture_path;

   _loader_thread = std::jthread(
      [self, texture_path](std::stop_token stop)
      {
         if (stop.stop_requested())
            return;

         sf::Image img;
         if (!img.loadFromFile(texture_path.string()))
         {
            Log::Warning() << "failed to load image " << texture_path;
            if (auto strong = self.lock())
            {
               strong->_loading = false;
            }
            return;
         }

         if (auto strong = self.lock())
         {
            std::lock_guard lock(strong->_upload_mutex);
            strong->_upload_tasks.push([strong, img = std::move(img)]() mutable { strong->finishUpload(std::move(img)); });
         }
      }
   );
}

void LazyTexture::finishUpload(sf::Image image)
{
   Log::Info() << "uploading texture " << _texture_path;

   try
   {
      auto tex = std::make_shared<sf::Texture>(image);
      _texture.store(tex);
      _loaded = true;
   }
   catch (const std::exception& e)
   {
      Log::Warning() << "failed to construct texture from image: " << e.what();
   }

   _loading = false;
}

std::optional<std::shared_ptr<const sf::Texture>> LazyTexture::getTexture() const
{
   auto ptr = _texture.load();
   if (ptr && !ptr.get())
   {
      Log::Error() << "shared_ptr is set but null!";
      return std::nullopt;
   }
   return ptr ? std::optional(ptr) : std::nullopt;
}
