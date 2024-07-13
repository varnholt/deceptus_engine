#include "moveablebox.h"

#include "framework/math/box2dtools.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/fixturenode.h"
#include "game/texturepool.h"

#include <iostream>

//--------------------------------------------------------------------------------------------------
MoveableBox::MoveableBox(GameNode* node) : GameNode(node)
{
   setClassName(typeid(MoveableBox).name());
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::preload()
{
   Audio::getInstance().addSample("mechanism_moveable_object_01.wav");
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::update(const sf::Time& /*dt*/)
{
   const auto x = _body->GetPosition().x * PPM;
   const auto y = _body->GetPosition().y * PPM;
   _sprite.setPosition(x, y - 24);

   // if the thing is moving, start playing a scratching sound
   if (fabs(_body->GetLinearVelocity().x) > 0.01)
   {
      if (!_pushing_sample.has_value())
      {
         _pushing_sample = Audio::getInstance().playSample({"mechanism_moveable_object_01.wav", 1.0, true});
      }
   }
   else
   {
      if (_pushing_sample.has_value())
      {
         Audio::getInstance().stopSample(_pushing_sample.value());
         _pushing_sample.reset();
      }
   }
}

//--------------------------------------------------------------------------------------------------
std::optional<sf::FloatRect> MoveableBox::getBoundingBoxPx()
{
   return _sprite.getGlobalBounds();
}

// box: pos: 5160 x 1056 size: 48 x 48
// box: pos: 5376 x 1080 size: 24 x 24

/*

   +----+
   | __ |
   |//\\|
   +----+
   |####|
   |####|
   +----+

   +----+----+
   | ___|___ |
   |////|\\\\|
   +----+----+
   |####|####|
   |####|####|
   +----+----+
   |####|####|
   |####|####|
   +----+----+


*/

//--------------------------------------------------------------------------------------------------
void MoveableBox::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _texture = TexturePool::getInstance().get("data/sprites/moveable_box.png");
   _sprite.setTexture(*_texture.get());

   _size.x = data._tmx_object->_width_px;
   _size.y = data._tmx_object->_height_px;

   _sprite.setPosition(data._tmx_object->_x_px, data._tmx_object->_y_px - 24);

   if (data._tmx_object->_properties)
   {
      const auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }
   }

   switch (static_cast<int32_t>(_size.x))
   {
      case 24:
      {
         _sprite.setTextureRect(sf::IntRect(168, 0, 24, 2 * 24));
         break;
      }

      case 48:
      {
         _sprite.setTextureRect(sf::IntRect(72, 24, 2 * 24, 3 * 24));
         break;
      }

      default:
      {
         break;
      }
   }

   setupBody(data._world);
   setupTransform();
}

//-----------------------------------------------------------------------------
void MoveableBox::setupTransform()
{
   auto x = _sprite.getPosition().x / PPM;
   auto y = _sprite.getPosition().y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

//--------------------------------------------------------------------------------------------------
void MoveableBox::setupBody(const std::shared_ptr<b2World>& world)
{
   const auto size_x = _size.x / PPM;
   const auto size_y = _size.y / PPM;
   b2PolygonShape polygon_shape = Box2DTools::createBeveledBox(size_x, size_y, 0.1f);

   b2BodyDef body_def;
   body_def.type = b2_dynamicBody;
   _body = world->CreateBody(&body_def);

   setupTransform();

   auto fixture = _body->CreateFixture(&polygon_shape, 1.0);
   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeMoveableBox);
   fixture->SetUserData(static_cast<void*>(object_data));
}
