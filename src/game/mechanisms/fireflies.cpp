#include "fireflies.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/debug/debugdraw.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
constexpr auto FRAME_COUNT = 4;
constexpr auto ANIMATION_SPEED = 3.0f;
}  // namespace

// #define DEBUG_RECT 1

namespace
{
const auto registered_fireflies = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.mapGroupToLayer("Fireflies", "fireflies");

   registry.registerLayerName(
      "fireflies",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<Fireflies>(parent);
         mechanism->deserialize(data);
         mechanisms["fireflies"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "Fireflies",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<Fireflies>(parent);
         mechanism->deserialize(data);
         mechanisms["fireflies"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

Fireflies::Fireflies(GameNode* parent) : GameNode(parent)
{
}

std::string_view Fireflies::objectName() const
{
   return "Fireflies";
}

void Fireflies::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
#ifdef DEBUG_RECT
   DebugDraw::drawRect(target, _rect_px, sf::Color::Magenta);
#endif

   for (const auto& firefly : _fireflies)
   {
      target.draw(*firefly._sprite);
   }
}

void Fireflies::update(const sf::Time& dt)
{
   for (auto& firefly : _fireflies)
   {
      firefly.update(dt);
   }
}

std::optional<sf::FloatRect> Fireflies::getBoundingBoxPx()
{
   return std::nullopt;
}

void Fireflies::deserialize(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _rect_px.position.x = data._tmx_object->_x_px;
   _rect_px.position.y = data._tmx_object->_y_px;
   _rect_px.size.x = data._tmx_object->_width_px;
   _rect_px.size.y = data._tmx_object->_height_px;

   addChunks(_rect_px);

   auto animation_speed = ANIMATION_SPEED;
   std::array<float, 2> scale_vertical{1.0f, 1.0f};
   std::array<float, 2> scale_horizontal{1.0f, 1.0f};
   std::array<float, 2> speed{1.0, 1.0f + (std::rand() % 1000) * 0.001f};
   auto count = 1;
   if (data._tmx_object->_properties)
   {
      auto it = data._tmx_object->_properties->_map.find("z");
      if (it != data._tmx_object->_properties->_map.end())
      {
         _z_index = it->second->_value_int.value();
      }

      it = data._tmx_object->_properties->_map.find("count");
      if (it != data._tmx_object->_properties->_map.end())
      {
         count = it->second->_value_int.value();
      }

      it = data._tmx_object->_properties->_map.find("scale_vertical_min");
      if (it != data._tmx_object->_properties->_map.end())
      {
         scale_vertical[0] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("scale_vertical_max");
      if (it != data._tmx_object->_properties->_map.end())
      {
         scale_vertical[1] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("scale_horizontal_min");
      if (it != data._tmx_object->_properties->_map.end())
      {
         scale_horizontal[0] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("scale_horizontal_max");
      if (it != data._tmx_object->_properties->_map.end())
      {
         scale_horizontal[1] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("speed_min");
      if (it != data._tmx_object->_properties->_map.end())
      {
         speed[0] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("speed_max");
      if (it != data._tmx_object->_properties->_map.end())
      {
         speed[1] = it->second->_value_float.value();
      }

      it = data._tmx_object->_properties->_map.find("animation_speed");
      if (it != data._tmx_object->_properties->_map.end())
      {
         animation_speed = it->second->_value_float.value();
      }
   }

   _texture = TexturePool::getInstance().get("data/sprites/firefly.png");

   for (auto i = 0; i < count; i++)
   {
      _fireflies.push_back({*_texture});
   }

   auto frand = [](float min, float max)
   {
      const auto val = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
      return min + (val * (max - min));
   };

   for (auto& firefly : _fireflies)
   {
      firefly._instance_number = _instance_counter++;
      firefly._rect_px = _rect_px;
      firefly._sprite->setTextureRect({{0, 0}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
      firefly._elapsed += sf::seconds(static_cast<float>(std::rand() % 999));
      firefly._angle_x = frand(30.0, 360.0) * FACTOR_DEG_TO_RAD;
      firefly._angle_y = frand(30.0, 360.0) * FACTOR_DEG_TO_RAD;
      firefly._speed = frand(speed[0], speed[1]);
      firefly._dir = (std::rand() % 2) ? 1.0f : -1.0f;
      firefly._scale_vertical = frand(scale_vertical[0], scale_vertical[1]);
      firefly._scale_horizontal = frand(scale_horizontal[0], scale_horizontal[1]);
      firefly._animation_speed = animation_speed;
   }
}

void rotate(float& x, float& y, float& z, float angle_x, float angle_y)
{
   float temp_x = x;
   float temp_y = y;
   float temp_z = z;

   // Rotate around the x-axis
   y = temp_y * cos(angle_x) - temp_z * sin(angle_x);
   z = temp_y * sin(angle_x) + temp_z * cos(angle_x);

   // Rotate around the y-axis
   x = temp_x * cos(angle_y) + temp_z * sin(angle_y);
   z = -temp_x * sin(angle_y) + temp_z * cos(angle_y);
}

Fireflies::Firefly::Firefly(const sf::Texture& texture)
{
   _sprite = std::make_unique<sf::Sprite>(texture);
}

void Fireflies::Firefly::update(const sf::Time& dt)
{
   _elapsed += dt;
   const auto time_s = _elapsed.asSeconds();

   // compute 8 shape (lemniscate of Bernoulli)
   auto x = static_cast<float>(std::cos(time_s * _speed * _dir));
   auto y = static_cast<float>(std::sin(2.0f * time_s * _speed * _dir) / 2.0f);
   auto z = 0.0f;
   rotate(x, y, z, _angle_x, _angle_y);

   // scale x and y based on z depth?

   const auto x_scaled_px = x * _rect_px.size.x * 0.5f * _scale_horizontal;
   const auto y_scaled_px = y * _rect_px.size.y * 0.5f * _scale_vertical;

   // the above should be rotated around x, y, z axes
   _position.x = _rect_px.position.x + (_rect_px.size.x * 0.5f) + x_scaled_px;
   _position.y = _rect_px.position.y + (_rect_px.size.y * 0.5f) + y_scaled_px;

   _sprite->setPosition(_position);
   _sprite->setOrigin({PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2});

   updateTextureRect();
}

void Fireflies::Firefly::updateTextureRect()
{
   const auto elapsed_s = _elapsed.asSeconds();
   const auto frame = static_cast<int32_t>(elapsed_s * _animation_speed) % FRAME_COUNT;

   if (frame != _current_frame)
   {
      _current_frame = frame;
      _sprite->setTextureRect({{_current_frame * PIXELS_PER_TILE, 0}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
   }
}
