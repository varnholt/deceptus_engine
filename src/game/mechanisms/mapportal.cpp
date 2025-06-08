#include "mapportal.h"

// game
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <atomic>

namespace
{
const auto registered_moveablebox = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("MapPortal", "map_portals");

   registry.registerLayerName(
      "moveable_objects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<MapPortal>(parent);
         mechanism->setup(data);
         mechanisms["moveable_objects"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "MapPortal",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<MapPortal>(parent);
         mechanism->setup(data);
         mechanisms["map_portals"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

std::atomic<bool> MapPortal::_portal_lock = false;

MapPortal::MapPortal(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(MapPortal).name());
}

void MapPortal::draw(sf::RenderTarget& window, sf::RenderTarget& /*normal*/)
{
   for (const auto& sprite : _sprites)
   {
      window.draw(sprite);
   }
}

void MapPortal::lock()
{
   _portal_lock = true;
}

void MapPortal::unlock()
{
   _portal_lock = false;
}

bool MapPortal::isLocked()
{
   return _portal_lock;
}

void MapPortal::update(const sf::Time& /*dt*/)
{
}

std::optional<sf::FloatRect> MapPortal::getBoundingBoxPx()
{
   return _bounding_box;
}

bool MapPortal::isPlayerAtMapPortal() const
{
   return _player_at_MapPortal;
}

void MapPortal::setPlayerAtMapPortal(bool playerAtMapPortal)
{
   _player_at_MapPortal = playerAtMapPortal;
}

void MapPortal::setup(const GameDeserializeData& data)
{
}
