#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "game/level/chunk.h"

class LazyTexture
{
public:
   explicit LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk>& texture_chunks);
   virtual ~LazyTexture() = default;

   void update(const Chunk& player_chunk);
   const std::shared_ptr<sf::Texture>& getTexture() const;

private:
   void loadTexture();
   void unloadTexture();
   void uploadTexture();

   std::filesystem::path _texture_path;
   std::shared_ptr<sf::Texture> _texture;
   std::vector<Chunk> _texture_chunks;

   std::unique_ptr<sf::Image> _pending_image;
   std::atomic<bool> _image_ready = false;

   mutable std::mutex _mutex;
   std::atomic_flag _loading = ATOMIC_FLAG_INIT;
   std::jthread _loading_thread;
};
