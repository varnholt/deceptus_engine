#include "gamemechanismobserver.h"

namespace GameMechanismObserver
{
std::vector<EnabledCallback> _enabled_listeners;
std::vector<EventCallback> _event_listeners;
}  // namespace GameMechanismObserver

void GameMechanismObserver::onEnabled(const std::string& object_id, const std::string& group_id, bool enabled)
{
   for (const auto& listener : _enabled_listeners)
   {
      listener(object_id, group_id, enabled);
   }
}

void GameMechanismObserver::onEvent(
   const std::string& object_id,
   const std::string& object_group,
   const std::string& event_name,
   const LuaVariant& value
)
{
   Log::Info() << _event_listeners.size();

   for (const auto& listener : _event_listeners)
   {
      Log::Info() << "notifying event receiver";
      listener(object_id, object_group, event_name, value);
   }
}
