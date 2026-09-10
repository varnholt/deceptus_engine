#include "footstepsurface.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <array>

FootstepSurface::FootstepSurface(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(FootstepSurface).name());
}

std::string_view FootstepSurface::objectName() const
{
   return "FootstepSurface";
}

std::shared_ptr<FootstepSurface> FootstepSurface::deserialize(GameNode* parent, const GameDeserializeData& data)
{
   auto instance = std::make_shared<FootstepSurface>(parent);

   instance->setObjectId(data._tmx_object->_name);
   instance->_rect_px =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;
      instance->_surface = ValueReader::readValue<std::string>("surface", map).value_or("");
      instance->setEnabled(ValueReader::readValue<bool>("enabled", map).value_or(true));
   }

   return instance;
}

std::optional<sf::FloatRect> FootstepSurface::getBoundingBoxPx()
{
   return _rect_px;
}

const std::string& FootstepSurface::getSurface() const
{
   return _surface;
}

namespace
{
static constexpr std::array footstep_surface_properties{
   PropertyInfo{.name = "surface", .type = "string", .default_value = std::string_view{""}, .required = true},
   PropertyInfo{.name = "enabled", .type = "bool", .default_value = true},
};
static constexpr MechanismSchema footstep_surface_schema{
   .type_name = "FootstepSurface",
   .layer_name = "footstep_surfaces",
   .default_width = 192,
   .default_height = 192,
   .properties = footstep_surface_properties,
};
const auto registered_footstep_surface = []
{
   GameMechanismDeserializerRegistry::instance().registerSchema(footstep_surface_schema);
   return true;
}();
}  // namespace
