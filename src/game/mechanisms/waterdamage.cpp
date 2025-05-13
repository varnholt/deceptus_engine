#include "waterdamage.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

namespace
{
const auto registered_waterdamage = []
{
   GameMechanismDeserializerRegistry::instance().registerLayer(
      "water_damage",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<WaterDamage>(parent);
         mechanism->setup(data);
         mechanisms["water_damage"]->push_back(mechanism);
      }
   );
   GameMechanismDeserializerRegistry::instance().registerTemplateType(
      "WaterDamage",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<WaterDamage>(parent);
         mechanism->setup(data);
         mechanisms["water_damage"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

WaterDamage::WaterDamage(GameNode* parent) : GameNode(parent)
{
}

void WaterDamage::update(const sf::Time& /*dt*/)
{
   if (!isEnabled())
   {
      return;
   }

   if (!Player::getCurrent()->isInWater())
   {
      _last_hurt_timepoint.reset();
      return;
   }

   const auto now = std::chrono::high_resolution_clock::now();

   if (!_last_hurt_timepoint.has_value())
   {
      _last_hurt_timepoint = now;
   }

   if (now - _last_hurt_timepoint.value() > _hurt_interval)
   {
      damage();
      _last_hurt_timepoint = now;
   }
}

void WaterDamage::setup(const GameDeserializeData& data)
{
   if (!data._tmx_object->_properties)
   {
      return;
   }

   auto hurt_interval_it = data._tmx_object->_properties->_map.find("hurt_interval_ms");
   if (hurt_interval_it != data._tmx_object->_properties->_map.end())
   {
      _hurt_interval = std::chrono::milliseconds(hurt_interval_it->second->_value_int.value());
   }

   auto hurt_amount_it = data._tmx_object->_properties->_map.find("hurt_amount");
   if (hurt_amount_it != data._tmx_object->_properties->_map.end())
   {
      _hurt_amount = hurt_amount_it->second->_value_int.value();
   }
}

std::optional<sf::FloatRect> WaterDamage::getBoundingBoxPx()
{
   return std::nullopt;
}

void WaterDamage::damage()
{
   Player::getCurrent()->damage(_hurt_amount);
}
