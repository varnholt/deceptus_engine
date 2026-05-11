#pragma once

#include "game/level/levelinterface.h"

#include <memory>

class LevelRegistry
{
public:
   /// \brief registers the active level; called when a level is loaded.
   /// \param level shared ownership of the level to register.
   static void setCurrent(std::shared_ptr<LevelInterface> level);

   /// \brief unregisters the active level; called when a level is unloaded.
   static void clearCurrent();

   /// \brief returns the currently active level.
   /// \return shared pointer to the current level, or nullptr when no level is active.
   static std::shared_ptr<LevelInterface> getCurrent();
};
