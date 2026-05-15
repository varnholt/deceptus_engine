#include "itemplayerfirefly.h"

#include "game/level/levelregistry.h"
#include "game/player/playerfirefly.h"

namespace
{
void setFireflyEnabled(bool enabled)
{
   const auto level = LevelRegistry::getCurrent();
   if (!level)
   {
      return;
   }

   const auto matches =
      level->getMechanismRegistry().searchMechanismsIf([](const auto& mechanism, auto)
                                                       { return dynamic_cast<PlayerFirefly*>(mechanism.get()) != nullptr; });

   for (const auto& mechanism : matches)
   {
      mechanism->setEnabled(enabled);
   }
}
}  // namespace

void ItemPlayerFirefly::onEquipped()
{
   setFireflyEnabled(true);
}

void ItemPlayerFirefly::onUnequipped()
{
   setFireflyEnabled(false);
}

std::string ItemPlayerFirefly::getName() const
{
   return "PlayerFirefly";
}
