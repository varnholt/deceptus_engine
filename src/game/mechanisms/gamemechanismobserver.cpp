#include "gamemechanismobserver.h"

void GameMechanismObserver::onEnabled(bool enabled)
{
   for (const auto& listener : _enabled_listeners)
   {
      listener(enabled);
   }
}
