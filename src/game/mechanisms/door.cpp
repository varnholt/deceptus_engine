#include "door.h"

// game
#include "constants.h"
#include "fixturenode.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/timer.h"
#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "level.h"
#include "player/player.h"
#include "savestate.h"
#include "texturepool.h"

#include <iostream>


namespace
{
constexpr auto door_height_tl = 3;
}

//-----------------------------------------------------------------------------
Door::Door(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(Door).name());
}


//-----------------------------------------------------------------------------
Door::~Door()
{
   // destructor for debugging purposes only
   // std::cout << "door destroyed" << std::endl;
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
         if (fabs(_offset) >= PIXELS_PER_TILE * door_height_tl)
         {
            _state = State::Open;
            _offset = static_cast<float_t>(-PIXELS_PER_TILE * door_height_tl);
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
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void Door::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);

   if (enabled)
   {
      open();
   }
   else
   {
      close();
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
   vertices[1] = b2Vec2(x_offset,          door_height_tl * size_y);
   vertices[2] = b2Vec2(x_offset + size_x, door_height_tl * size_y);
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

   return at_door;
}


//-----------------------------------------------------------------------------
const sf::IntRect& Door::getPixelRect() const
{
   return _pixel_rect;
}


//-----------------------------------------------------------------------------
void Door::toggleWithPlayerChecks()
{
   if (!SaveState::getPlayerInfo()._inventory.hasInventoryItem(_required_item))
   {
      // Log::Info() << "player doesn't have key";
      return;
   }

   if (!checkPlayerAtDoor())
   {
      // Log::Info() << "player not in front of door";
      return;
   }

   toggle();
}


//-----------------------------------------------------------------------------
void Door::open()
{
   if (_state == State::Opening)
   {
      return;
   }

   _state = State::Opening;

   if (_automatic_close)
   {
      Timer::add(
         std::chrono::milliseconds(10000),
         [this](){close();},
         Timer::Type::Singleshot,
         Timer::Scope::UpdateIngame
      );
   }
}


//-----------------------------------------------------------------------------
void Door::close()
{
   if (_state == State::Closing)
   {
       return;
   }

   _state = State::Closing;
}


//-----------------------------------------------------------------------------
void Door::toggle()
{
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
void Door::setPlayerAtDoor(bool player_at_door)
{
   _player_at_door = player_at_door;
}


//-----------------------------------------------------------------------------
void Door::setupKeySprite(ItemType item_type, const sf::Vector2f& pos)
{
   static const std::unordered_map<ItemType, int32_t> map{
      std::make_pair(ItemType::KeyRed, 1),
      std::make_pair(ItemType::KeyGreen, 4),
      std::make_pair(ItemType::KeyBlue, 7),
      std::make_pair(ItemType::KeyYellow, 10),
      std::make_pair(ItemType::KeyOrange, 13),
   };

   const auto offset_it = map.find(item_type);

   if (offset_it == map.end())
   {
      return;
   }

   _sprite_icon.setTexture(*_texture);
   _sprite_icon.setTextureRect(sf::IntRect(PIXELS_PER_TILE * offset_it->second, PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE));
   _sprite_icon.setPosition(pos);
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Door::load(const GameDeserializeData& data)
{
   if (!data._tmx_layer)
   {
      Log::Error() << "tmx layer is empty, please fix your level design";
      return {};
   }

   if (!data._tmx_tileset)
   {
      Log::Error() << "tmx tileset is empty, please fix your level design";
      return {};
   }

   std::vector<std::shared_ptr<GameMechanism>> doors;

   const auto tiles    = data._tmx_layer->_data;
   const auto width    = data._tmx_layer->_width_tl;
   const auto height   = data._tmx_layer->_height_tl;
   const auto first_id = data._tmx_tileset->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto j = 0u; j < height; j++)
   {
      for (auto i = 0u; i < width; i++)
      {
         // get the current tile number
         auto tile_number = tiles[i + j * width];

         if (tile_number == 0)
         {
            continue;
         }
         auto tile_id = tile_number - first_id;

         // 21: red
         // 24: green
         // 27: blue
         // ...

         auto required_item = ItemType::Invalid;
         auto create_door = false;

         switch (tile_id)
         {
            case 21:
               required_item = ItemType::KeyRed;
               create_door = true;
               break;
            case 24:
               required_item = ItemType::KeyGreen;
               create_door = true;
               break;
            case 27:
               required_item = ItemType::KeyBlue;
               create_door = true;
               break;
            case 30:
               required_item = ItemType::KeyYellow;
               create_door = true;
               break;
            case 33:
               required_item = ItemType::KeyOrange;
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

            door->_texture = TexturePool::getInstance().get((data._base_path / data._tmx_tileset->_image->_source).string());
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
            door->_pixel_rect = sf::IntRect{
               static_cast<int32_t>(position_x + PIXELS_PER_TILE),
               static_cast<int32_t>(position_y),
               PIXELS_PER_TILE,
               PIXELS_PER_TILE * 3
            };

            // draw required door open icon
            if (required_item != ItemType::Invalid)
            {
               auto key_sprite_pos = sf::Vector2f{
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               };

               door->setupKeySprite(required_item, key_sprite_pos);
            }

            if (data._tmx_layer->_properties)
            {
               door->setZ(data._tmx_layer->_properties->_map["z"]->_value_int.value());
            }
         }
      }
   }

   for (auto& tmp : doors)
   {
      std::dynamic_pointer_cast<Door>(tmp)->setupBody(data._world);
   }

   return doors;
}


void Door::setup(const GameDeserializeData& data)
{
   setObjectId(data._tmx_object->_name);

   const auto x_px = data._tmx_object->_x_px;
   const auto y_px = data._tmx_object->_y_px;

   _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "doors.png");

   _door_quad[0].position.x = x_px - PIXELS_PER_TILE;
   _door_quad[0].position.y = y_px;
   _door_quad[1].position.x = x_px - PIXELS_PER_TILE;
   _door_quad[1].position.y = y_px + 3 * PIXELS_PER_TILE;
   _door_quad[2].position.x = x_px + 3 * PIXELS_PER_TILE - PIXELS_PER_TILE;
   _door_quad[2].position.y = y_px + 3 * PIXELS_PER_TILE;
   _door_quad[3].position.x = x_px + 3 * PIXELS_PER_TILE - PIXELS_PER_TILE;
   _door_quad[3].position.y = y_px;

   _type = Type::Bars;

   _tile_position.x = static_cast<int32_t>(x_px / PIXELS_PER_TILE);
   _tile_position.y = static_cast<int32_t>(y_px / PIXELS_PER_TILE);

   _pixel_rect = sf::IntRect{
      static_cast<int32_t>(x_px),
      static_cast<int32_t>(y_px),
      PIXELS_PER_TILE,
      PIXELS_PER_TILE * 3
   };

   const auto z_it = data._tmx_object->_properties->_map.find("z");
   if (z_it != data._tmx_object->_properties->_map.end())
   {
      const auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
      setZ(z_index);
   }

   const auto key_it = data._tmx_object->_properties->_map.find("key");
   if (key_it != data._tmx_object->_properties->_map.end())
   {
      const auto key = key_it->second->_value_string.value();

      static const std::unordered_map<std::string, ItemType> map{
         std::make_pair("key_red", ItemType::KeyRed),
         std::make_pair("key_green", ItemType::KeyGreen),
         std::make_pair("key_blue", ItemType::KeyBlue),
         std::make_pair("key_yellow", ItemType::KeyYellow),
         std::make_pair("key_orange", ItemType::KeyOrange),
      };

      const auto key_type_it = map.find(key);
      if (key_type_it != map.end())
      {
         _required_item = key_type_it->second;
         setupKeySprite(key_type_it->second, sf::Vector2f{x_px, y_px});
      }
   }

   setupBody(data._world);
}
