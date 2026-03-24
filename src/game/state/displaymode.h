#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include "constants.h"

/// \brief manages queued bitflag changes for the current display overlays.
class DisplayMode
{
public:
   /// \brief constructs a display mode with the game flag enabled.
   DisplayMode() = default;

   /// \brief returns the global display mode singleton.
   /// \return reference to the shared display mode instance.
   static DisplayMode& getInstance();

   /// \brief applies all queued mode mutations in insertion order.
   void sync();

   /// \brief queues a bitwise set operation for a display flag.
   /// \param mode display flag that will be enabled during the next sync call.
   void enqueueSet(Display mode);

   /// \brief queues a bitwise clear operation for a display flag.
   /// \param mode display flag that will be disabled during the next sync call.
   void enqueueUnset(Display mode);

   /// \brief queues a toggle operation for a display flag.
   /// \param mode display flag that will be toggled during the next sync call.
   void enqueueToggle(Display mode);

   /// \brief returns the raw bitmask of currently active display flags.
   /// \return current display mode bitmask.
   int32_t get() const;

   /// \brief checks whether a specific display flag is currently set.
   /// \param mode display flag to test.
   /// \return true when the given flag bit is set in the current mask.
   bool isSet(Display mode) const;

private:
   /// \brief toggles one display flag immediately.
   /// \param mode display flag to toggle.
   void toggle(Display mode);
   int32_t _mode = static_cast<int32_t>(Display::Game);
   using QueuedFunction = std::function<void(void)>;
   std::vector<QueuedFunction> _queue;
};
