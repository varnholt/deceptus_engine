#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>

#include "framework/tools/log.h"

/*! \brief a generic resource pool for managing resources with type safety.
 *
 *  the pool holds weak pointers to resources, which means that resources
 *  are deleted once they're no longer referenced elsewhere.
 *
 * \tparam Resource The type of resource to manage (e.g., sf::Texture).
 */
template <typename Resource>
class ResourcePool
{
public:

   /*! \brief retrieve a shared_ptr to a resource from the pool.
    *         if the resource is not already loaded, it is loaded and cached.
    *
    * \param path path to the resource.
    * \return shared pointer to the loaded resource.
    */
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

   /*! \brief compute the total size of all resources in the pool.
    *         this method uses the `computeResourceSize` function to calculate
    *         the size of each resource.
    * \return The total size of all resources in bytes.
    */
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

   /*! \brief load a resource from the given path.
    *         derived classes must implement this function to define how a resource is loaded.
    * \param resource The resource to load.
    * \param path The path to load the resource from.
    * \return \c true if the resource was loaded successfully; otherwise, false.
    */
   virtual bool loadResource(Resource& resource, const std::filesystem::path& path) const = 0;

   /*! \brief compute the size of a resource in bytes.
    *         derived classes must implement this function to calculate the size of a specific resource.
    * \param resource The resource to calculate the size of.
    * \return size of the resource in bytes.
    */
   virtual size_t computeResourceSize(const Resource& resource) const = 0;

private:
   mutable std::mutex m_mutex;
   std::map<std::string, std::weak_ptr<Resource>> m_pool;
};
