#include "game/mechanisms/blockingrect.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tools/sfmlcompat.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
static constexpr std::array blocking_rect_properties{
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
   PropertyInfo{.name = "collapse_when_disabled", .type = "bool", .default_value = false},
};

constexpr auto collapse_piece_size_px = 24;
constexpr auto collapse_gravity_px_s2 = 900.0f;
constexpr auto collapse_max_delay_s = 0.25f;
constexpr auto collapse_fade_start_s = 0.4f;
constexpr auto collapse_fade_duration_s = 0.6f;
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

      const auto collapse_it = data._tmx_object->_properties->_map.find("collapse_when_disabled");
      if (collapse_it != data._tmx_object->_properties->_map.end())
      {
         _collapse_when_disabled = collapse_it->second->_value_bool.value_or(false);
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
         _sprite->position = {data._tmx_object->_x_px, data._tmx_object->_y_px};
#else
         _sprite = std::make_unique<sf::Sprite>(*_texture_map);
         _sprite->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px});
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
   // nothing to paint
   if (_sprite == nullptr)
   {
      return;
   }

   // later might need something like fading when not visible
   if (!isEnabled())
   {
      if (!_collapse_pieces.empty())
      {
         drawCollapse(target, normal, states);
      }

      return;
   }

   _drawn = true;

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
   if (_collapse_pieces.empty())
   {
      return;
   }

   const auto dt_s = dt.asSeconds();
   _collapse_elapsed_s += dt_s;

   for (auto& piece : _collapse_pieces)
   {
      if (_collapse_elapsed_s < piece._delay_s)
      {
         continue;
      }

      piece._velocity_y_px_s += collapse_gravity_px_s2 * dt_s;
      piece._position_px.y += piece._velocity_y_px_s * dt_s;
      piece._rotation_degrees += piece._rotation_speed_degrees_s * dt_s;
   }

   if (_collapse_elapsed_s > collapse_max_delay_s + collapse_fade_start_s + collapse_fade_duration_s)
   {
      _collapse_pieces.clear();
   }
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

   // a rectangle that is disabled before it was ever seen, e.g. while a save state is applied, just isn't there
   if (was_enabled && !enabled && _collapse_when_disabled && _drawn)
   {
      startCollapse();
   }
}

void BlockingRect::startCollapse()
{
   if (!_texture_map)
   {
      return;
   }

   _collapse_sprite = sfcompat::createSprite(*_texture_map);
   sfcompat::setOrigin(*_collapse_sprite, {collapse_piece_size_px * 0.5f, collapse_piece_size_px * 0.5f});

   _collapse_elapsed_s = 0.0f;
   _collapse_pieces.clear();

   const auto columns = static_cast<int32_t>(_rectangle.size.x) / collapse_piece_size_px;
   const auto rows = static_cast<int32_t>(_rectangle.size.y) / collapse_piece_size_px;

   for (auto row = 0; row < rows; ++row)
   {
      for (auto column = 0; column < columns; ++column)
      {
         CollapsePiece piece;
         piece._texture_rect = {
            {column * collapse_piece_size_px, row * collapse_piece_size_px}, {collapse_piece_size_px, collapse_piece_size_px}
         };
         piece._position_px = {
            _rectangle.position.x + (column + 0.5f) * collapse_piece_size_px,
            _rectangle.position.y + (row + 0.5f) * collapse_piece_size_px
         };

         // the top row breaks first and the middle of the hole goes before its edges
         const auto random_unit = (std::rand() % 256) / 255.0f;
         const auto distance_from_centre = std::abs(column - (columns - 1) * 0.5f) / std::max(1.0f, columns * 0.5f);
         piece._delay_s = collapse_max_delay_s * (0.5f * distance_from_centre + 0.3f * random_unit + 0.2f * row / std::max(1, rows));
         piece._rotation_speed_degrees_s = (random_unit - 0.5f) * 240.0f;
         _collapse_pieces.push_back(piece);
      }
   }
}

void BlockingRect::drawCollapse(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states)
{
   const auto fade_progress = std::clamp((_collapse_elapsed_s - collapse_fade_start_s) / collapse_fade_duration_s, 0.0f, 1.0f);
   const auto alpha = static_cast<uint8_t>(255.0f * (1.0f - fade_progress));
   sfcompat::setColor(*_collapse_sprite, sf::Color{255, 255, 255, alpha});

   for (const auto& piece : _collapse_pieces)
   {
      sfcompat::setTextureRect(*_collapse_sprite, piece._texture_rect);
      sfcompat::setPosition(*_collapse_sprite, piece._position_px);
      sfcompat::setRotation(*_collapse_sprite, sf::degrees(piece._rotation_degrees));

#ifdef DECEPTUS_VRSFML
      sf::RenderStates color_states = states;
      color_states.texture = _texture_map.get();
      target.draw(*_collapse_sprite, color_states);

      if (_normal_map)
      {
         sf::RenderStates normal_states = states;
         normal_states.texture = _normal_map.get();
         normal.draw(*_collapse_sprite, normal_states);
      }
#else
      _collapse_sprite->setTexture(*_texture_map);
      target.draw(*_collapse_sprite, states);

      if (_normal_map)
      {
         _collapse_sprite->setTexture(*_normal_map);
         normal.draw(*_collapse_sprite, states);
      }
#endif
   }
}

std::optional<sf::FloatRect> BlockingRect::getBoundingBoxPx()
{
   return _rectangle;
}
