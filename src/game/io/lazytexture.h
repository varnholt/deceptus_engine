#include <SFML/Graphics.hpp>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

#include "game/level/chunk.h"

class LazyTexture
{
public:
   explicit LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk>& _texture_chunks);
   void update(const Chunk& player_chunk);
   std::optional<std::reference_wrapper<const sf::Texture>> getTexture() const;

private:
   void loadTexture();
   void unloadTexture();

   std::filesystem::path _texture_path;
   std::unique_ptr<sf::Texture> _texture;
   std::vector<Chunk> _texture_chunks;

   mutable std::mutex _mutex;
   std::atomic<bool> _loaded = false;
   std::atomic_flag _loading = ATOMIC_FLAG_INIT;
   std::jthread _loading_thread;
};
