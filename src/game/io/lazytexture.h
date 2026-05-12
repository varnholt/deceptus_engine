#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "game/level/chunk.h"

/// \brief loads and unloads textures on demand based on player chunk proximity.
class LazyTexture
{
public:
   /// \brief creates a lazy texture controller for one texture file and its owning chunks.
   /// \param texture_path file path of the texture image on disk.
   /// \param texture_chunks chunk list in which this texture is considered relevant.
   explicit LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk>& texture_chunks);

   /// \brief destroys the lazy texture instance and joins any pending loader thread.
   virtual ~LazyTexture() = default;

   /// \brief decides whether texture data should be loaded, uploaded, kept, or released.
   /// \param player_chunk chunk currently occupied by the player.
   void update(const Chunk& player_chunk);

   /// \brief starts background disk loading unconditionally, ignoring chunk proximity.
   /// call during level load to avoid first-in-range hitches from disk I/O on the render thread.
   void preload();

   /// \brief uploads the texture to GPU if the background load has finished.
   /// \return true while a background load is still in flight or waiting to upload.
   bool drain();

   /// \brief returns the currently uploaded texture, if available.
   /// \return shared texture pointer reference used by render code.
   const std::shared_ptr<sf::Texture>& getTexture() const;

private:
   /// \brief starts background loading of image pixels from disk into a pending buffer.
   void loadTexture();

   /// \brief releases uploaded texture and any pending image data.
   void unloadTexture();

   /// \brief uploads a loaded pending image to GPU texture memory on the main thread.
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
