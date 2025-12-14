#pragma once

#include <SFML/Graphics.hpp>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include "framework/tools/log.h"

struct AsyncTextureHandle
{
   std::shared_ptr<std::shared_future<std::shared_ptr<sf::Texture>>> future_ptr;
   std::shared_ptr<std::atomic<bool>> loaded;

   AsyncTextureHandle() : loaded(std::make_shared<std::atomic<bool>>(false)) {}
   explicit AsyncTextureHandle(std::shared_future<std::shared_ptr<sf::Texture>> fut)
   {
      future_ptr = std::make_shared<std::shared_future<std::shared_ptr<sf::Texture>>>(std::move(fut));
      loaded = std::make_shared<std::atomic<bool>>(false);
   }

   std::shared_future<std::shared_ptr<sf::Texture>>& future()
   {
      return *future_ptr;
   }
   const std::shared_future<std::shared_ptr<sf::Texture>>& future() const
   {
      return *future_ptr;
   }

   void setLoaded() { loaded->store(true, std::memory_order_release); }
   bool isLoaded() const { return loaded->load(std::memory_order_acquire); }
};

class AsyncTexturePool
{
public:
   static AsyncTexturePool& getInstance()
   {
      static AsyncTexturePool instance;
      return instance;
   }

   // Request a texture asynchronously
   // Returns a handle that can be checked for loading status
   AsyncTextureHandle getAsync(const std::filesystem::path& path)
   {
      std::lock_guard<std::mutex> lock(_request_mutex);

      auto it = _handles.find(path.string());
      if (it != _handles.end())
      {
         return it->second;
      }

      // Create promise/future pair for async loading
      auto prom = std::make_shared<std::promise<std::shared_ptr<sf::Texture>>>();
      auto fut = prom->get_future().share();

      // Store the promise to complete later
      _promises[path.string()] = prom;

      // Queue the loading request
      _loading_queue.push(path);
      _request_condition.notify_one();

      AsyncTextureHandle handle(fut);
      _handles[path.string()] = handle;

      return handle;
   }

   // Get a texture synchronously if it's already loaded
   // If not loaded, it will block until it's ready
   std::shared_ptr<sf::Texture> get(const std::filesystem::path& path)
   {
      auto handle = getAsync(path);

      // Wait for the texture to load and return it
      return handle.future().get();
   }

   // Check if a texture is loaded without blocking
   bool isLoaded(const std::filesystem::path& path) const
   {
      std::lock_guard<std::mutex> lock(_request_mutex);
      auto it = _handles.find(path.string());
      if (it != _handles.end())
      {
         // First check using our loaded flag for efficiency
         if (it->second.isLoaded()) {
            return true;
         }
         // Then check if the future is ready (fallback)
         auto status = it->second.future().wait_for(std::chrono::milliseconds(1));
         if (status == std::future_status::ready) {
             return true;
         }
      }
      return false;
   }

   // Non-blocking check with texture retrieval
   std::shared_ptr<sf::Texture> tryGet(const std::filesystem::path& path)
   {
      std::lock_guard<std::mutex> lock(_request_mutex);
      auto it = _handles.find(path.string());
      if (it != _handles.end())
      {
         // Check using our loaded flag first for efficiency
         if (it->second.isLoaded()) {
             return it->second.future().get(); // This shouldn't block since it's loaded
         }
         // Then check if the future is ready (fallback)
         auto status = it->second.future().wait_for(std::chrono::milliseconds(1));
         if (status == std::future_status::ready)
         {
            return it->second.future().get();  // This won't block since it's ready
         }
      }
      return nullptr;  // Not loaded yet
   }

   // Update method - for this simplified version, we just keep the method for compatibility
   // The completion is handled directly in the worker thread
   void update()
   {
      // In this implementation, the async loading is handled directly by the
      // promises being fulfilled by the background thread, so no additional
      // update logic is needed here.
   }

private:
   AsyncTexturePool()
   {
      // Start the background loading thread
      _worker_thread = std::thread(&AsyncTexturePool::backgroundLoader, this);
   }

   ~AsyncTexturePool()
   {
      _shutdown = true;
      _request_condition.notify_all();

      if (_worker_thread.joinable())
      {
         _worker_thread.join();
      }
   }

   void backgroundLoader()
   {
      while (!_shutdown)
      {
         std::filesystem::path path;

         // Wait for a loading request
         {
            std::unique_lock<std::mutex> lock(_request_mutex);
            _request_condition.wait(lock, [this] { return !_loading_queue.empty() || _shutdown; });

            if (_shutdown && _loading_queue.empty())
            {
               break;
            }

            if (!_loading_queue.empty())
            {
               path = _loading_queue.front();
               _loading_queue.pop();
            }
         }

         if (!path.empty())
         {
            // Load the texture in the background
            auto texture = std::make_shared<sf::Texture>();

            bool success = false;

            // Check if the file exists before attempting to load
            if (!std::filesystem::exists(path)) {
               Log::Warning() << "texture file does not exist: " << path;
            }
            else {
               // Time the file loading operation (disk I/O)
               auto start_time = std::chrono::high_resolution_clock::now();
               success = texture->loadFromFile(path.string());
               auto end_time = std::chrono::high_resolution_clock::now();

               auto file_load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

               if (!success)
               {
                  Log::Warning() << "error loading texture: " << path;
               }
               else if (file_load_duration.count() >= 100)
               {
                  Log::Info() << "Async disk load time for " << path << ": " << file_load_duration.count() << " ms (Background thread)";
               }
            }

            // Complete the promise and mark as loaded
            std::lock_guard<std::mutex> lock(_request_mutex);
            auto prom_it = _promises.find(path.string());
            if (prom_it != _promises.end() && prom_it->second)
            {
               if (success)
               {
                  prom_it->second->set_value(texture);
               }
               else
               {
                  prom_it->second->set_value(nullptr);
               }
               prom_it->second.reset();  // Release the shared_ptr to promise
               _promises.erase(prom_it);

               // Mark the corresponding handle as loaded
               auto handle_it = _handles.find(path.string());
               if (handle_it != _handles.end()) {
                   handle_it->second.setLoaded();
               }
            }
         }
      }
   }

private:
   mutable std::mutex _request_mutex;
   std::queue<std::filesystem::path> _loading_queue;
   std::condition_variable _request_condition;
   std::unordered_map<std::string, AsyncTextureHandle> _handles;
   std::unordered_map<std::string, std::shared_ptr<std::promise<std::shared_ptr<sf::Texture>>>> _promises;

   std::thread _worker_thread;
   std::atomic<bool> _shutdown{false};
};
