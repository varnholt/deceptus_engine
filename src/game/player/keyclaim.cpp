#include "keyclaim.h"

KeyClaimRegistry::KeyClaimRegistry(AxisReader vertical_axis_reader, AxisReader horizontal_axis_reader)
    : _vertical_axis_reader(std::move(vertical_axis_reader)), _horizontal_axis_reader(std::move(horizontal_axis_reader))
{
}

bool KeyClaimRegistry::isClaimed(KeyPressed key) const
{
   return _claims.find(key) != _claims.end();
}

void KeyClaimRegistry::claim(const std::vector<KeyPressed>& keys)
{
   for (const auto key : keys)
   {
      _claims[key]++;
   }
}

void KeyClaimRegistry::unclaim(const std::vector<KeyPressed>& keys)
{
   for (const auto key : keys)
   {
      const auto it = _claims.find(key);

      if (it == _claims.end())
      {
         continue;
      }

      it->second--;

      if (it->second <= 0)
      {
         _claims.erase(it);
      }
   }
}

float KeyClaimRegistry::readVerticalAxis() const
{
   return _vertical_axis_reader ? _vertical_axis_reader() : 0.0f;
}

float KeyClaimRegistry::readHorizontalAxis() const
{
   return _horizontal_axis_reader ? _horizontal_axis_reader() : 0.0f;
}

KeyClaim::KeyClaim(const std::shared_ptr<KeyClaimRegistry>& registry, const std::vector<KeyPressed>& keys)
    : _registry(registry), _keys(keys)
{
   if (registry)
   {
      registry->claim(_keys);
   }
}

KeyClaim::~KeyClaim()
{
   release();
}

KeyClaim::KeyClaim(KeyClaim&& other) noexcept : _registry(std::move(other._registry)), _keys(std::move(other._keys))
{
   other._registry.reset();
   other._keys.clear();
}

KeyClaim& KeyClaim::operator=(KeyClaim&& other) noexcept
{
   if (this == &other)
   {
      return *this;
   }

   release();

   _registry = std::move(other._registry);
   _keys = std::move(other._keys);

   other._registry.reset();
   other._keys.clear();

   return *this;
}

bool KeyClaim::isActive() const
{
   return !_keys.empty();
}

float KeyClaim::readVerticalAxis() const
{
   const auto registry = _registry.lock();

   if (!registry || _keys.empty())
   {
      return 0.0f;
   }

   return registry->readVerticalAxis();
}

float KeyClaim::readHorizontalAxis() const
{
   const auto registry = _registry.lock();

   if (!registry || _keys.empty())
   {
      return 0.0f;
   }

   return registry->readHorizontalAxis();
}

void KeyClaim::release()
{
   if (const auto registry = _registry.lock())
   {
      registry->unclaim(_keys);
   }

   _registry.reset();
   _keys.clear();
}
