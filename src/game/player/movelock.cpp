#include "movelock.h"

#include <mutex>

namespace
{
std::mutex _mutex;
uint8_t _lock = 0;
}  // namespace

void MoveLock::lock(LockType lock_type)
{
   const std::lock_guard<std::mutex> lock(_mutex);
   _lock |= static_cast<uint8_t>(lock_type);
}

void MoveLock::unlock(LockType lock_type)
{
   const std::lock_guard<std::mutex> lock(_mutex);
   _lock &= ~static_cast<uint8_t>(lock_type);
}

bool MoveLock::hasVerticalLock()
{
   const std::lock_guard<std::mutex> lock(_mutex);
   return _lock & static_cast<uint8_t>(LockType::Vertical);
}

bool MoveLock::hasHorizontalLock()
{
   const std::lock_guard<std::mutex> lock(_mutex);
   return _lock & static_cast<uint8_t>(LockType::Horizontal);
}
