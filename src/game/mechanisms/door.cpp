#include "door.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "level.h"
#include "player/player.h"
#include "savestate.h"
#include "texturepool.h"

#include "framework/math/sfmlmath.h"
#include "framework/tools/timer.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"

#include <iostream>


//-----------------------------------------------------------------------------
Door::Door(GameNode* parent)
 : GameNode(parent)
{
   setName(typeid(Door).name());
}


//-----------------------------------------------------------------------------
void Door::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite_icon);
   color.draw(_door_quad, _texture.get());
}


//-----------------------------------------------------------------------------
void Door::updateBars(const sf::Time& dt)
{
   const float left   = 0.0f;
   const float right  = left + 3.0f * PIXELS_PER_TILE;
   const float top    = 3.0f * PIXELS_PER_TILE - _offset;
   const float bottom = top + 3.0f * PIXELS_PER_TILE;

   _door_quad[0].texCoords = sf::Vector2f(left, top);
   _door_quad[1].texCoords = sf::Vector2f(left, bottom);
   _door_quad[2].texCoords = sf::Vector2f(right, bottom);
   _door_quad[3].texCoords = sf::Vector2f(right, top);

   constexpr auto open_speed = 50.0f;
   constexpr auto close_speed = 200.0f;

   switch (_state)
   {
      case State::Opening:
      {
         _offset -= open_speed * dt.asSeconds();
         if (fabs(_offset) >= PIXELS_PER_TILE * _height)
         {
            _state = State::Open;
            _offset = static_cast<float_t>(-PIXELS_PER_TILE * _height);
         }
         break;
      }
      case State::Closing:
      {
         _offset += close_speed * dt.asSeconds();
         if (_offset >= 0.0f)
         {
            _state = State::Closed;
            _offset = 0.0f;
         }
         break;
      }
      case State::Open:
      case State::Closed:
         break;
   }

   updateTransform();
}


//-----------------------------------------------------------------------------
void Door::update(const sf::Time& dt)
{
   switch (_type)
   {
      case Type::Conventional:
      {
         break;
      }
      case Type::Bars:
      {
         updateBars(dt);
         // updateBars(dt);
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void Door::updateTransform()
{
   auto x =            _tile_position.x * PIXELS_PER_TILE / PPM;
   auto y = (_offset + _tile_position.y * PIXELS_PER_TILE) / PPM;
   _body->SetTransform(b2Vec2(x, y), 0);
}


//-----------------------------------------------------------------------------
void Door::reset()
{
   _state = _initial_state;

   switch (_state)
   {
      case State::Open:
         _offset = 1.0f;
         break;
      case State::Closed:
         _offset = 0.0f;
         break;
      case State::Opening:
      case State::Closing:
         break;
   }
}


//-----------------------------------------------------------------------------
void Door::setupBody(
   const std::shared_ptr<b2World>& world,
   float x_offset,
   float x_scale
)
{
   b2PolygonShape polygon_shape;
   auto size_x = (PIXELS_PER_TILE / PPM) * x_scale;
   auto size_y = (PIXELS_PER_TILE / PPM);

   b2Vec2 vertices[4];
   vertices[0] = b2Vec2(x_offset,          0);
   vertices[1] = b2Vec2(x_offset,          _height * size_y);
   vertices[2] = b2Vec2(x_offset + size_x, _height * size_y);
   vertices[3] = b2Vec2(x_offset + size_x, 0);
   polygon_shape.Set(vertices, 4);

   b2BodyDef body_def;
   body_def.type = b2_kinematicBody;
   _body = world->CreateBody(&body_def);

   updateTransform();

   auto fixture = _body->CreateFixture(&polygon_shape, 0);
   auto object_data = new FixtureNode(this);
   object_data->setType(ObjectTypeDoor);
   fixture->SetUserData(static_cast<void*>(object_data));
}


//-----------------------------------------------------------------------------
bool Door::checkPlayerAtDoor() const
{
   const auto player_pos = Player::getCurrent()->getPixelPositionf();
   const auto door_pos = _sprite_icon.getPosition();

   sf::Vector2f a(player_pos.x, player_pos.y);
   sf::Vector2f b(door_pos.x + PIXELS_PER_TILE * 0.5f, door_pos.y + 3 * PIXELS_PER_TILE);

   const auto distance = SfmlMath::length(a - b);
   const auto at_door = (distance < PIXELS_PER_TILE * 1.5f);

   // std::cout << fabs(a.x - b.x) << std::endl;
   // std::cout << fabs(a.y - b.y) << std::endl;

   return at_door;
}


//-----------------------------------------------------------------------------
void Door::toggle()
{
   if (!SaveState::getPlayerInfo().mInventory.hasInventoryItem(_required_item))
   {
      // std::cout << "player doesn't have key" << std::endl;
      return;
   }

   if (!checkPlayerAtDoor())
   {
      // std::cout << "player not in front of door" << std::endl;
      return;
   }

   switch (_state)
   {
      case State::Open:
         close();
         break;
      case State::Closed:
         open();
         break;
      case State::Opening:
      case State::Closing:
         break;
   }
}


//-----------------------------------------------------------------------------
void Door::open()
{
   _state = State::Opening;
   Timer::add(std::chrono::milliseconds(10000), [this](){close();}, Timer::Type::Singleshot);
}


//-----------------------------------------------------------------------------
void Door::close()
{
   _state = State::Closing;
}


//-----------------------------------------------------------------------------
const sf::Vector2i& Door::getTilePosition() const
{
   return _tile_position;
}


//-----------------------------------------------------------------------------
bool Door::isPlayerAtDoor() const
{
   return _player_at_door;
}



//-----------------------------------------------------------------------------
void Door::setPlayerAtDoor(bool playerAtDoor)
{
   _player_at_door = playerAtDoor;
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Door::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>& world
)
{
   std::vector<std::shared_ptr<GameMechanism>> doors;

   const auto tiles      = layer->_data;
   const auto width    = layer->_width_px;
   const auto height    = layer->_height_px;
   const auto first_id  = tileSet->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto j = 0u; j < height; j++)
   {
      for (auto i = 0u; i < width; i++)
      {
         // get the current tile number
         auto tile_number = tiles[i + j * width];

         if (tile_number != 0)
         {
            auto tile_id = tile_number - first_id;

            // 21: red
            // 24: green
            // 27: blue
            // ...

            auto required_item = ItemType::Invalid;
            auto icon_offset = 0;
            auto create_door = false;

            switch (tile_id)
            {
               case 21:
                  required_item = ItemType::KeyRed;
                  icon_offset = 1;
                  create_door = true;
                  break;
               case 24:
                  required_item = ItemType::KeyGreen;
                  icon_offset = 4;
                  create_door = true;
                  break;
               case 27:
                  required_item = ItemType::KeyBlue;
                  icon_offset = 7;
                  create_door = true;
                  break;
               case 30:
                  required_item = ItemType::KeyYellow;
                  icon_offset = 10;
                  create_door = true;
                  break;
               case 33:
                  required_item = ItemType::KeyOrange;
                  icon_offset = 13;
                  create_door = true;
                  break;
               case 36:
                  required_item = ItemType::Invalid;
                  create_door = true;
                  break;

               default:
                  break;
            }

            if (create_door)
            {
               const auto position_x = static_cast<float>(i * PIXELS_PER_TILE - PIXELS_PER_TILE);
               const auto position_y = static_cast<float>(j * PIXELS_PER_TILE + PIXELS_PER_TILE);

               auto door = std::make_shared<Door>(Level::getCurrentLevel());
               doors.push_back(door);

               door->_texture = TexturePool::getInstance().get((basePath / tileSet->_image->_source).string());
               door->_door_quad[0].position.x = position_x;
               door->_door_quad[0].position.y = position_y;
               door->_door_quad[1].position.x = position_x;
               door->_door_quad[1].position.y = position_y + 3 * PIXELS_PER_TILE;
               door->_door_quad[2].position.x = position_x + 3 * PIXELS_PER_TILE;
               door->_door_quad[2].position.y = position_y + 3 * PIXELS_PER_TILE;
               door->_door_quad[3].position.x = position_x + 3 * PIXELS_PER_TILE;
               door->_door_quad[3].position.y = position_y;
               door->_type = Type::Bars;
               door->_tile_id = tile_id;
               door->_tile_position.x = static_cast<int32_t>(i);
               door->_tile_position.y = static_cast<int32_t>(j) + 1; // the actual door is a tile lower
               door->_required_item = required_item;
               door->_height = 3; // hardcoded 3 tiles

               if (required_item != ItemType::Invalid)
               {
                  door->_sprite_icon.setTexture(*door->_texture);
                  door->_sprite_icon.setTextureRect(sf::IntRect(PIXELS_PER_TILE * icon_offset, PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE));
                  door->_sprite_icon.setPosition(
                     static_cast<float>(i * PIXELS_PER_TILE),
                     static_cast<float>(j * PIXELS_PER_TILE)
                  );
               }

               if (layer->_properties != nullptr)
               {
                  door->setZ(layer->_properties->_map["z"]->_value_int.value());
               }
            }

            // std::cout << "found new door: " << tileId << std::endl;
         }
      }
   }

   for (auto& tmp : doors)
   {
      std::dynamic_pointer_cast<Door>(tmp)->setupBody(world);
   }

   return doors;
}
