#include "buttonrect.h"

#include <array>

#include "framework/tmxparser/tmxproperties.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/mechanisms/gamemechanismobserver.h"
#include "game/player/playerregistry.h"

namespace
{
static constexpr std::string_view default_button_rect_button = "b";
static constexpr std::array button_rect_properties{
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
   PropertyInfo{.name = "button", .type = "string", .default_value = default_button_rect_button},
};
static constexpr MechanismSchema button_rect_schema{
   .type_name = "ButtonRect",
   .layer_name = "button_rects",
   .default_width = 48,
   .default_height = 24,
   .properties = button_rect_properties,
};
const auto registered_buttonrect = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(button_rect_schema);

   registry.markAsNonVisual("button_rects");
   registry.mapGroupToLayer("ButtonRect", "button_rects");

   registry.registerLayerName(
      "button_rects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<ButtonRect>(parent);
         mechanism->setup(data);
         mechanisms["button_rects"]->push_back(mechanism);
      }
   );

   registry.registerObjectGroup(
      "ButtonRect",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<ButtonRect>(parent);
         mechanism->setup(data);
         mechanisms["button_rects"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

ButtonRect::ButtonRect(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(GameNode).name());
}

std::string_view ButtonRect::objectName() const
{
   return "ButtonRect";
}

void ButtonRect::update(const sf::Time& /*dt*/)
{
   _player_intersects = PlayerRegistry::getFirst()->getPixelRectFloat().findIntersection(_rect).has_value();

   if (!isEnabled())
   {
      return;
   }

   if (!_player_intersects)
   {
      return;
   }

   if ((_button == Button::A && PlayerRegistry::getFirst()->getControls()->isButtonAPressed()) ||
       (_button == Button::B && PlayerRegistry::getFirst()->getControls()->isButtonBPressed()) ||
       (_button == Button::X && PlayerRegistry::getFirst()->getControls()->isButtonXPressed()) ||
       (_button == Button::Y && PlayerRegistry::getFirst()->getControls()->isButtonYPressed()))
   {
      GameMechanismObserver::onEvent(getObjectId(), "button_rects", "pressed", "true");
   }
}

std::optional<sf::FloatRect> ButtonRect::getBoundingBoxPx()
{
   return _rect;
}

void ButtonRect::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);
   _rect = sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   if (data._tmx_object->_properties)
   {
      static std::unordered_map<std::string, Button> button_map{{"a", Button::A}, {"b", Button::B}, {"x", Button::X}, {"y", Button::Y}};

      const auto& map = data._tmx_object->_properties->_map;
      const auto button_id_str = ValueReader::readValue<std::string>("button", map).value_or(std::string(default_button_rect_button));
      const auto button_it = button_map.find(button_id_str);
      if (button_it != button_map.end())
      {
         _button = button_it->second;
      }
   }

   addChunks(_rect);
}
