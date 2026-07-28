#include "leveltransition.h"

#include <array>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/log.h"
#include "framework/tools/sfmlcompat.h"
#include "game/io/valuereader.h"
#include "game/level/leveltransitionhandler.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/playerregistry.h"

namespace
{
static constexpr std::string_view default_level_transition_level = "";
static constexpr std::array level_transition_properties{
   PropertyInfo{.name = "level", .type = "string", .default_value = default_level_transition_level},
   PropertyInfo{.name = "spawn_position_x_px", .type = "int", .default_value = int32_t{0}},
   PropertyInfo{.name = "spawn_position_y_px", .type = "int", .default_value = int32_t{0}},
};
static constexpr MechanismSchema level_transition_schema{
   .type_name = "LevelTransition",
   .layer_name = "level_transitions",
   .default_width = 48,
   .default_height = 96,
   .properties = level_transition_properties,
};
const auto registered_leveltransition = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(level_transition_schema);
   registry.markAsNonVisual("level_transitions");
   registry.mapGroupToLayer("LevelTransition", "level_transitions");

   registry.registerLayerName(
      "level_transitions",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<LevelTransition>(parent);
         mechanism->setup(data);
         mechanisms["level_transitions"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "LevelTransition",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<LevelTransition>(parent);
         mechanism->setup(data);
         mechanisms["level_transitions"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

LevelTransition::LevelTransition(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(LevelTransition).name());
}

std::string_view LevelTransition::objectName() const
{
   return "LevelTransition";
}

void LevelTransition::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   _rect_px =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   if (data._tmx_object->_properties)
   {
      const auto& property_map = data._tmx_object->_properties->_map;

      _level_description_filename =
         ValueReader::readValue<std::string>("level", property_map).value_or(std::string{default_level_transition_level});

      const auto spawn_position_x_px = ValueReader::readValue<int32_t>("spawn_position_x_px", property_map);
      const auto spawn_position_y_px = ValueReader::readValue<int32_t>("spawn_position_y_px", property_map);

      if (spawn_position_x_px.has_value() && spawn_position_y_px.has_value())
      {
         _spawn_position_px =
            sf::Vector2f{static_cast<float>(spawn_position_x_px.value()), static_cast<float>(spawn_position_y_px.value())};
      }
   }

   if (_level_description_filename.empty())
   {
      Log::Error() << "level transition '" << getObjectId() << "' has no 'level' property, it will not do anything";
   }
}

void LevelTransition::update(const sf::Time& /*dt*/)
{
   if (_level_description_filename.empty())
   {
      return;
   }

   const auto player_intersects = sfcompat::findIntersection(PlayerRegistry::getFirst()->getPixelRectFloat(), _rect_px).has_value();

   // fire once when the player enters the rect; the level stays alive while the screen fades out,
   // so without the guard the request would be filed again on every frame the player keeps standing inside
   if (player_intersects && !_player_intersects && !_requested)
   {
      _requested = true;
      Log::Info() << "level transition '" << getObjectId() << "' entered, loading " << _level_description_filename;
      LevelTransitionHandler::getInstance().request(_level_description_filename, _spawn_position_px);
   }

   _player_intersects = player_intersects;
}

std::optional<sf::FloatRect> LevelTransition::getBoundingBoxPx()
{
   return _rect_px;
}
