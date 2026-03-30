#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

#include "framework/tools/log.h"

///
/// \brief Caches loaded resources and reloads them on demand when expired.
/// \tparam Resource Resource type managed by the pool.
///
template <typename Resource>
class ResourcePool
{
public:
   ///
   /// \brief Returns a shared resource for `path`, loading it when needed.
   ///
   /// If the cached weak pointer has expired, this function creates a new resource,
   /// calls loadResource(), stores it in the pool, and returns it.
   ///
   /// \param path Resource file path used as cache key.
   /// \return Shared pointer to the cached or newly loaded resource.
   ///
   std::shared_ptr<Resource> get(const std::filesystem::path& path)
   {
      std::lock_guard<std::mutex> hold(m_mutex);

      const auto key = path.string();
      auto sp = m_pool[key].lock();
      if (!sp)
      {
         sp = std::make_shared<Resource>();
         if (!loadResource(*sp, path))
         {
            Log::Warning() << "error loading texture: " << path;
         }
         m_pool[key] = sp;
      }

      return sp;
   }

   ///
   /// \brief Sums sizes of all currently alive resources in the cache.
   /// \return Total size in bytes.
   ///
   size_t computeSize() const
   {
      size_t size = 0;
      std::lock_guard<std::mutex> hold(m_mutex);

      for (const auto& [key, value] : m_pool)
      {
         if (auto sp = value.lock())
         {
            size += computeResourceSize(*sp);
         }
      }

      return size;
   }

protected:
   ResourcePool() = default;

   ///
   /// \brief Loads `resource` from `path`.
   /// \param resource Resource instance to initialize.
   /// \param path Source path for loading.
   /// \return `true` when loading succeeded; otherwise `false`.
   ///
   virtual bool loadResource(Resource& resource, const std::filesystem::path& path) const = 0;

   ///
   /// \brief Computes the memory footprint of one resource.
   /// \param resource Resource instance to measure.
   /// \return Resource size in bytes.
   ///
   virtual size_t computeResourceSize(const Resource& resource) const = 0;

private:
   mutable std::mutex m_mutex;
   std::map<std::string, std::weak_ptr<Resource>> m_pool;
};
