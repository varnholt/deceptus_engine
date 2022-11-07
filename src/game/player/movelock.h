#pragma once

namespace MoveLock
{

enum class LockType
{
   Vertical = 0x01,
   Horizontal = 0x02
};

void lock(LockType lock_type);
void unlock(LockType lock_type);

bool hasVerticalLock();
bool hasHorizontalLock();

};  // namespace MoveLock
