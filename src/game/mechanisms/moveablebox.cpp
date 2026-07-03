#include "moveablebox.h"

#include "framework/math/box2dtools.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/audio/audio.h"
#include "game/constants.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/level/fixturenode.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

#include <array>
#include <iostream>

namespace
{
static constexpr std::array moveable_box_properties{
   PropertyInfo{.name = "z", .type = "int", .default_value = int32_t{20}},
};
static constexpr MechanismSchema moveable_box_schema{
   .type_name = "MoveableObject",
   .layer_name = "moveable_objects",
   .default_width = 24,
   .default_height = 24,
   .properties = moveable_box_properties,
};
const auto registered_moveablebox = []
{
   auto& registry = GameMechanismDeserializerRegistry::instance();
   registry.registerSchema(moveable_box_schema);

   registry.mapGroupToLayer("MoveableObject", "moveable_objects");

   registry.registerLayerName(
      "moveable_objects",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<MoveableBox>(parent);
         mechanism->setup(data);
         mechanisms["moveable_objects"]->push_back(mechanism);
      }
   );
   registry.registerObjectGroup(
      "MoveableObject",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<MoveableBox>(parent);
         mechanism->setup(data);
         mechanisms["moveable_objects"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

MoveableBox::MoveableBox(GameNode* node) : GameNode(node)
{
   setClassName(typeid(MoveableBox).name());
}

std::string_view MoveableBox::objectName() const
{
   return "MoveableBox";
}

void MoveableBox::preload()
{
   Audio::getInstance().addSample("mechanism_moveable_object_01.wav");
}

void MoveableBox::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   draw(color, normal, {});
}

void MoveableBox::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/, const sf::RenderStates& states)
{
   sf::RenderStates draw_states = states;
   draw_states.texture = _texture.get();
   color.draw(*_sprite, draw_states);
}

void MoveableBox::update(const sf::Time& /*dt*/)
{
   const auto x = _body->GetPosition().x * PPM;
   const auto y = _body->GetPosition().y * PPM;
   _sprite->position = {x, y - 24};

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

std::optional<sf::FloatRect> MoveableBox::getBoundingBoxPx()
{
   return _sprite->getGlobalBounds();
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

void MoveableBox::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _texture = TexturePool::getInstance().get("data/sprites/moveable_box.png");
   _sprite = std::make_unique<sf::Sprite>();

   _size.x = data._tmx_object->_width_px;
   _size.y = data._tmx_object->_height_px;

   _sprite->position = {data._tmx_object->_x_px, data._tmx_object->_y_px - 24};

   const auto rect =
      sf::FloatRect{{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   addChunks(rect);

   if (data._tmx_object->_properties)
   {
      const auto& map = data._tmx_object->_properties->_map;

      _settings._density = ValueReader::readValue<float>("density", map).value_or(_settings._density);
      _settings._friction = ValueReader::readValue<float>("friction", map).value_or(_settings._friction);
      _settings._gravity_scale = ValueReader::readValue<float>("gravity_scale", map).value_or(_settings._gravity_scale);
      setZ(ValueReader::readValue<int32_t>("z", map).value_or(0));
   }

   switch (static_cast<int32_t>(_size.x))
   {
      case 24:
      {
         _sprite->textureRect = sf::FloatRect{{168.f, 0.f}, {24.f, 2.f * 24.f}};
         break;
      }

      case 48:
      {
         _sprite->textureRect = sf::FloatRect{{72.f, 24.f}, {2.f * 24.f, 3.f * 24.f}};
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

void MoveableBox::setupTransform()
{
   auto x = _sprite->position.x / PPM;
   auto y = _sprite->position.y / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}

void MoveableBox::setupBody(const std::shared_ptr<b2World>& world)
{
   const auto size_x = _size.x / PPM;
   const auto size_y = _size.y / PPM;
   b2PolygonShape polygon_shape = Box2DTools::createBeveledBox(size_x, size_y, 0.1f);

   b2FixtureDef fixture_def;
   fixture_def.shape = &polygon_shape;
   fixture_def.density = _settings._density;
   fixture_def.friction = _settings._friction;
   fixture_def.filter.categoryBits = CategoryMoveableBox;                                    // I am a
   fixture_def.filter.maskBits = CategoryBoundary | CategoryMoveableBox | CategoryFriendly;  // I collide with

   b2BodyDef body_def;
   body_def.type = b2_dynamicBody;
   _body = world->CreateBody(&body_def);
   _body->SetGravityScale(_settings._gravity_scale);

   setupTransform();

   auto* fixture = _body->CreateFixture(&fixture_def);
   auto* object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeMoveableBox);
   fixture->SetUserData(static_cast<void*>(object_data));
}
