#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

#include "game/level/chunk.h"

/**
 * @class LazyTexture
 * @brief Loads and uploads a texture on demand based on player proximity.
 *
 * This class handles lazy-loading of textures in two phases:
 * - It loads the image data from disk in a background thread (non-blocking).
 * - Once loading is complete, it uploads the texture to the GPU on the main thread.
 *
 * The texture is only loaded when the player is near one of the specified chunks.
 * This avoids loading unnecessary textures and reduces memory and GPU usage.
 *
 * Thread safety:
 * - Disk loading is performed in a detached background thread (`std::jthread`).
 * - GPU uploads are deferred and executed on the main thread during `update()`.
 * - Access to the loaded texture is provided via an atomic `std::shared_ptr`.
 *
 * Important: The class must be owned via `std::shared_ptr` because it uses
 * `std::enable_shared_from_this` for lifetime safety when capturing `this`.
 */
class LazyTexture : public std::enable_shared_from_this<LazyTexture>
{
public:
   /**
    * @brief Constructs a LazyTexture with the associated disk path and coverage area.
    * @param texture_path Path to the texture file on disk.
    * @param texture_chunks List of chunks near which the texture should be loaded.
    */
   LazyTexture(const std::filesystem::path& texture_path, std::vector<Chunk> texture_chunks);

   /**
    * @brief Destructor. Automatically joins the background loader thread.
    */
   ~LazyTexture() = default;

   /**
    * @brief Main entry point for proximity checking and GPU upload.
    *
    * Should be called once per frame from the main thread.
    * Triggers background loading if the player is close to the texture's chunks,
    * and finalizes any pending uploads if background loading is complete.
    *
    * @param player_chunk The chunk where the player currently is.
    */
   void update(const Chunk& player_chunk);

   /**
    * @brief Returns the loaded texture if available.
    * @return A shared pointer to the texture, or std::nullopt if not loaded yet.
    */
   std::optional<std::shared_ptr<const sf::Texture>> getTexture() const;

private:
   /**
    * @brief Spawns a background thread to load the image from disk.
    *
    * The load is performed asynchronously using std::jthread.
    * Once complete, a deferred GPU upload task is queued to be executed in `update()`.
    */
   void requestLoad();

   /**
    * @brief Uploads the loaded image to the GPU (must run on main thread).
    * @param image The image previously loaded by the background thread.
    */
   void finishUpload(sf::Image image);

   std::filesystem::path _texture_path;  ///< File path to the texture image.
   std::vector<Chunk> _texture_chunks;   ///< Chunks that trigger loading when nearby.

   std::atomic<bool> _loaded = false;                         ///< Indicates whether the texture has been uploaded.
   std::atomic<bool> _loading = false;                        ///< Indicates whether a load is currently in progress.
   std::atomic<std::shared_ptr<const sf::Texture>> _texture;  ///< Atomically shared texture pointer.

   std::jthread _loader_thread;  ///< Background thread for disk loading (auto-joined).

   std::mutex _upload_mutex;                         ///< Guards the upload task queue.
   std::queue<std::function<void()>> _upload_tasks;  ///< Tasks to run on the main thread for GPU upload.
};
