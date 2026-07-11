#include "wind.h"

#include <array>
#include <format>
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/playerregistry.h"

namespace
{
static constexpr float default_wind_direction_x = 0.0f;
static constexpr float default_wind_direction_y = 0.0f;
static constexpr std::array wind_properties{
   PropertyInfo{.name = "direction_x", .type = "float", .default_value = default_wind_direction_x},
   PropertyInfo{.name = "direction_y", .type = "float", .default_value = default_wind_direction_y},
};
static constexpr MechanismSchema wind_schema{
   .type_name = "Wind",
   .layer_name = "wind",
   .default_width = 192,
   .default_height = 192,
   .properties = wind_properties,
};
const auto registered_wind = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(wind_schema);
   registry.markAsNonVisual("wind");
   registry.mapGroupToLayer("Wind", "wind");

   registry.registerLayerName(
      "wind",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto instance = Wind::deserialize(parent, data);
         mechanisms["wind"]->push_back(instance);
      }
   );

   registry.registerObjectGroup(
      "Wind",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto instance = Wind::deserialize(parent, data);
         mechanisms["wind"]->push_back(instance);
      }
   );

   return true;
}();
}  // namespace

Wind::Wind(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Wind).name());
}

std::string_view Wind::objectName() const
{
   return "Wind";
}

std::shared_ptr<Wind> Wind::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto wind = std::make_shared<Wind>(parent);
   wind->setObjectId(data._tmx_object->_name);

   wind->_area = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   if (data._tmx_object->_properties)
   {
      const auto& props = data._tmx_object->_properties->_map;

      wind->_direction.x = ValueReader::readValue<float>("direction_x", props).value_or(default_wind_direction_x);
      wind->_direction.y = ValueReader::readValue<float>("direction_y", props).value_or(default_wind_direction_y);
   }

   wind->addChunks(wind->_area);
   return wind;
}

void Wind::update(const sf::Time& /*dt*/)
{
   const auto player = PlayerRegistry::getFirst();
   const auto& player_rect = player->getPixelRectFloat();
#ifdef __EMSCRIPTEN__
   if (!sf::findIntersection(_area, player_rect).hasValue())
#else
   if (!_area.findIntersection(player_rect).has_value())
#endif
   {
      return;
   }

   auto* body = player->getBody();
   body->ApplyForceToCenter(b2Vec2(_direction.x, -_direction.y), true);
}

void Wind::draw(sf::RenderTarget& /*target*/, sf::RenderTarget& /*normal*/)
{
}

std::optional<sf::FloatRect> Wind::getBoundingBoxPx()
{
   return _area;
}

const sf::Vector2f& Wind::getDirection() const
{
   return _direction;
}
