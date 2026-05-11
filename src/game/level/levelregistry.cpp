#include "levelregistry.h"

namespace
{
std::shared_ptr<LevelInterface> current_level;
}

void LevelRegistry::setCurrent(std::shared_ptr<LevelInterface> level)
{
   current_level = std::move(level);
}

void LevelRegistry::clearCurrent()
{
   current_level.reset();
}

std::shared_ptr<LevelInterface> LevelRegistry::getCurrent()
{
   return current_level;
}
