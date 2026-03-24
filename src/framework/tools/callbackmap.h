#pragma once

#include <cstdint>
#include <functional>
#include <map>

///
/// \brief Stores callback lists keyed by integer id.
///
struct CallbackMap
{
   using Callback = std::function<void(void)>;

   ///
   /// \brief Returns the process-wide callback registry singleton.
   /// \return Callback map singleton.
   ///
   static CallbackMap& getInstance();

   ///
   /// \brief Appends a callback to the list for `id`.
   /// \param id Callback group id.
   /// \param cb Callback to register.
   ///
   void addCallback(int32_t id, const Callback& cb);
   ///
   /// \brief Invokes all callbacks registered for `id`.
   /// \param id Callback group id.
   ///
   void call(int32_t id);

private:
   std::map<int32_t, std::vector<Callback>> _map;

   CallbackMap() = default;
};
