#include "game/mechanisms/blockingrect.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
const auto registered_blockingrect = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();

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
         _sprite = std::make_unique<sf::Sprite>(*_texture_map);
         _sprite->setPosition({data._tmx_object->_x_px, data._tmx_object->_y_px});
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
}

const sf::FloatRect& BlockingRect::getPixelRect() const
{
   return _rectangle;
}

void BlockingRect::draw(sf::RenderTarget& target, sf::RenderTarget& normal)
{
   // nothing to paint
   if (_sprite == nullptr)
   {
      return;
   }

   // later might need something like fading when not visible
   if (!isEnabled())
   {
      return;
   }

   if (_normal_map)
   {
      _sprite->setTexture(*_texture_map);
   }

   target.draw(*_sprite);

   if (_normal_map)
   {
      _sprite->setTexture(*_normal_map);
   }

   normal.draw(*_sprite);
}

void BlockingRect::update(const sf::Time& /*dt*/)
{
}

void BlockingRect::setEnabled(bool enabled)
{
   _body->SetEnabled(enabled);
}

std::optional<sf::FloatRect> BlockingRect::getBoundingBoxPx()
{
   return _rectangle;
}
