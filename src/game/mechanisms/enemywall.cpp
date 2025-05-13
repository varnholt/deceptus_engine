#include "enemywall.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"

namespace
{
const auto registered_enemywall = []
{
   GameMechanismDeserializerRegistry::instance().registerLayer(
      "enemy_walls",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<EnemyWall>(parent);
         mechanism->setup(data);
         mechanisms["enemy_walls"]->push_back(mechanism);
      }
   );
   GameMechanismDeserializerRegistry::instance().registerTemplateType(
      "EnemyWall",
      [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
      {
         auto mechanism = std::make_shared<EnemyWall>(parent);
         mechanism->setup(data);
         mechanisms["enemy_walls"]->push_back(mechanism);
      }
   );
   return true;
}();
}  // namespace

namespace
{
uint16_t category_bits = CategoryBoundary;                                 // I am a ...
uint16_t mask_bits = CategoryEnemyCollideWith | CategoryEnemyWalkThrough;  // I collide with ...
int16_t group_index = 0;                                                   // 0 is default
}  // namespace

EnemyWall::EnemyWall(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(EnemyWall).name());
   setZ(1);  // bogus z
}

void EnemyWall::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   _rectangle = {{data._tmx_object->_x_px, data._tmx_object->_y_px}, {data._tmx_object->_width_px, data._tmx_object->_height_px}};

   if (data._tmx_object->_properties)
   {
      const auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         const auto enabled = static_cast<bool>(enabled_it->second->_value_bool.value());
         setEnabled(enabled);
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

   b2FixtureDef boundary_fixture_def;
   boundary_fixture_def.shape = &_shape_bounds;
   boundary_fixture_def.density = 1.0f;

   boundary_fixture_def.filter.groupIndex = 0;
   boundary_fixture_def.filter.maskBits = mask_bits;
   boundary_fixture_def.filter.categoryBits = category_bits;

   _body->CreateFixture(&boundary_fixture_def);
}

const sf::FloatRect& EnemyWall::getPixelRect() const
{
   return _rectangle;
}

void EnemyWall::update(const sf::Time& /*dt*/)
{
}

void EnemyWall::setEnabled(bool enabled)
{
   _body->SetEnabled(enabled);
}

std::optional<sf::FloatRect> EnemyWall::getBoundingBoxPx()
{
   return _rectangle;
}
