#include "game/mechanisms/blockingrect.h"

#include <algorithm>
#include <array>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/stringutils.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
static constexpr std::array blocking_rect_properties{
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
   PropertyInfo{.name = "collapse_when_disabled", .type = "bool", .default_value = false},
   PropertyInfo{.name = "texture_offset_y_px", .type = "int", .default_value = int32_t{0}},
   PropertyInfo{.name = "collapse_layers", .type = "string", .default_value = std::string_view{}},
   PropertyInfo{.name = "collapse_extend_up_px", .type = "int", .default_value = int32_t{0}},
};
static constexpr MechanismSchema blocking_rect_schema{
   .type_name = "BlockingRect",
   .layer_name = "blocking_rects",
   .default_width = 96,
   .default_height = 48,
   .properties = blocking_rect_properties,
};
const auto registered_blockingrect = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(blocking_rect_schema);

   registry.mapGroupToLayer("BlockingRect", "blocking_rects");

   registry.registerLayerName(
      "blocking_rects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<BlockingRect>(parent);
         mechanism->setup(data);
         mechanisms["blocking_rects"]->push_back(mechanism);
      }
   );

   registry.registerObjectGroup(
      "BlockingRect",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<BlockingRect>(parent);
         mechanism->setup(data);
         mechanisms["blocking_rects"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

BlockingRect::BlockingRect(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(BlockingRect).name());
}

std::string_view BlockingRect::objectName() const
{
   return "BlockingRect";
}

void BlockingRect::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _rectangle = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   setZ(static_cast<int32_t>(ZDepth::ForegroundMin) + 1);

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      const auto offset_it = data._tmx_object->_properties->_map.find("texture_offset_y_px");
      if (offset_it != data._tmx_object->_properties->_map.end())
      {
         _texture_offset_y_px = static_cast<float>(offset_it->second->_value_int.value_or(0));
      }

      const auto collapse_it = data._tmx_object->_properties->_map.find("collapse_when_disabled");
      if (collapse_it != data._tmx_object->_properties->_map.end())
      {
         _collapse_when_disabled = collapse_it->second->_value_bool.value_or(false);
      }

      const auto collapse_layers_it = data._tmx_object->_properties->_map.find("collapse_layers");
      if (collapse_layers_it != data._tmx_object->_properties->_map.end())
      {
         for (const auto& layer_name : StringUtils::split(collapse_layers_it->second->_value_string.value_or(""), ','))
         {
            _collapse_layer_names.push_back(StringUtils::trim(layer_name));
         }
      }

      const auto collapse_extend_up_it = data._tmx_object->_properties->_map.find("collapse_extend_up_px");
      if (collapse_extend_up_it != data._tmx_object->_properties->_map.end())
      {
         _collapse_extend_up_px = static_cast<float>(collapse_extend_up_it->second->_value_int.value_or(0));
      }

      const auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         const auto enabled = static_cast<bool>(enabled_it->second->_value_bool.value());
         setEnabled(enabled);
      }

      const auto texture_it = data._tmx_object->_properties->_map.find("texture");
      if (texture_it != data._tmx_object->_properties->_map.end())
      {
         const auto texture = texture_it->second->_value_string.value();
         _texture_map = TexturePool::getInstance().get(texture);
#ifdef DECEPTUS_VRSFML
         _sprite = std::make_unique<sf::Sprite>();
         _sprite->position = {data._tmx_object->_x_px, data._tmx_object->_y_px + _texture_offset_y_px};
#else
         _sprite = std::make_unique<sf::Sprite>(*_texture_map);
         _sprite->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px + _texture_offset_y_px});
#endif
      }

      const auto normal_it = data._tmx_object->_properties->_map.find("normal");
      if (normal_it != data._tmx_object->_properties->_map.end())
      {
         const auto normal = normal_it->second->_value_string.value();
         _normal_map = TexturePool::getInstance().get(normal);
      }
   }

   // create body
   _position_b2d = b2Vec2(data._tmx_object->_x_px * MPP, data._tmx_object->_y_px * MPP);
   _position_sfml.x = data._tmx_object->_x_px;
   _position_sfml.y = data._tmx_object->_y_px + data._tmx_object->_height_px;

   b2BodyDef bodyDef;
   bodyDef.type = b2_staticBody;
   bodyDef.position = _position_b2d;

   _body = data._world->CreateBody(&bodyDef);

   auto half_physics_width = data._tmx_object->_width_px * MPP * 0.5f;
   auto half_physics_height = data._tmx_object->_height_px * MPP * 0.5f;

   _shape_bounds.SetAsBox(half_physics_width, half_physics_height, b2Vec2(half_physics_width, half_physics_height), 0.0f);

   b2FixtureDef boundaryFixtureDef;
   boundaryFixtureDef.shape = &_shape_bounds;
   boundaryFixtureDef.density = 1.0f;

   _body->CreateFixture(&boundaryFixtureDef);
   _body->SetEnabled(isEnabled());

   addChunks(_rectangle);
}

const sf::FloatRect& BlockingRect::getPixelRect() const
{
   return _rectangle;
}

void BlockingRect::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   draw(target, normal, {});
}

void BlockingRect::draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   // later might need something like fading when not visible
   if (!isEnabled())
   {
      _collapse.draw(target, normal, states);

      return;
   }

   _drawn = true;

   // nothing to paint
   if (_sprite == nullptr)
   {
      return;
   }

#ifdef DECEPTUS_VRSFML
   sf::RenderStates color_states = states;
   color_states.texture = _texture_map.get();
   target.draw(*_sprite, color_states);

   if (_normal_map)
   {
      sf::RenderStates normal_states = states;
      normal_states.texture = _normal_map.get();
      normal.draw(*_sprite, normal_states);
   }
#else
   if (_normal_map)
   {
      _sprite->setTexture(*_texture_map);
   }

   target.draw(*_sprite, states);

   if (_normal_map)
   {
      _sprite->setTexture(*_normal_map);
   }

   normal.draw(*_sprite, states);
#endif
}

void BlockingRect::update(const sf::Time& dt)
{
   _collapse.update(dt);
}

void BlockingRect::setEnabled(bool enabled)
{
   const auto was_enabled = isEnabled();

   // the flag is what draw() checks, the body may not exist yet while the properties are read
   GameMechanism::setEnabled(enabled);

   if (_body)
   {
      _body->SetEnabled(enabled);
   }

   if (!was_enabled || enabled || !_collapse_when_disabled)
   {
      return;
   }

   // the tiles go either way, they are drawn by their layers and not by the rectangle
   const auto fragments = removeCollapseTiles();

   // a rectangle that is disabled before it was ever seen, e.g. while a save state is applied, just isn't there
   if (!_drawn)
   {
      return;
   }

   if (_collapse_layer_names.empty())
   {
      _collapse.start(_texture_map, _normal_map, {_rectangle.position.x, _rectangle.position.y + _texture_offset_y_px});
   }
   else
   {
      _collapse.start(fragments);
   }
}

void BlockingRect::setTileMaps(const std::vector<std::shared_ptr<TileMap>>& tile_maps)
{
   for (const auto& tile_map : tile_maps)
   {
      if (std::ranges::find(_collapse_layer_names, tile_map->getLayerName()) != _collapse_layer_names.end())
      {
         _collapse_tile_maps.push_back(tile_map);
      }
   }

   // a rectangle that was disabled while the level was set up has no tiles left to show
   if (!isEnabled() && _collapse_when_disabled)
   {
      removeCollapseTiles();
   }
}

std::vector<TextureCollapse::Fragment> BlockingRect::removeCollapseTiles()
{
   std::vector<TextureCollapse::Fragment> fragments;

   const sf::FloatRect collapse_rect_px{
      {_rectangle.position.x, _rectangle.position.y - _collapse_extend_up_px},
      {_rectangle.size.x, _rectangle.size.y + _collapse_extend_up_px}
   };

   for (const auto& tile_map : _collapse_tile_maps)
   {
      for (const auto& removed_tile : tile_map->removeTiles(collapse_rect_px))
      {
         fragments.push_back({tile_map->getTextureMap(), tile_map->getNormalMap(), removed_tile._texture_rect, removed_tile._top_left_px});
      }
   }

   return fragments;
}

std::optional<sf::FloatRect> BlockingRect::getBoundingBoxPx()
{
   return _rectangle;
}
