#pragma once

#include <chrono>

namespace TimerLock
{

using HighResTimePoint = std::chrono::high_resolution_clock::time_point;

void lockFor(std::chrono::milliseconds interval);
void lock();
void unlock();
bool isLocked();

};  // namespace MoveLock
