#include "lever.h"

#include "constants.h"
#include "conveyorbelt.h"
#include "door.h"
#include "fan.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "gamemechanism.h"
#include "gamemechanismaudio.h"
#include "laser.h"
#include "movingplatform.h"
#include "onoffblock.h"
#include "player/player.h"
#include "rotatingblade.h"
#include "spikeblock.h"
#include "spikes.h"
#include "texturepool.h"

#include <iostream>

std::vector<std::shared_ptr<TmxObject>> Lever::__rectangles;

namespace
{
constexpr auto sprites_per_row = 11;
constexpr auto row_center = 6;
constexpr auto left_offset = (sprites_per_row - 1) * 3 * PIXELS_PER_TILE;
constexpr auto idle_animation_speed = 10.0f;
}  // namespace

//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Lever::load(GameNode* parent, const GameDeserializeData& data)
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

   std::vector<std::shared_ptr<GameMechanism>> levers;

   auto tiles = data._tmx_layer->_data;
   auto width = data._tmx_layer->_width_tl;
   auto height = data._tmx_layer->_height_tl;
   auto first_id = data._tmx_tileset->_first_gid;

   for (auto j = 0; j < static_cast<int32_t>(height); j++)
   {
      for (auto i = 0; i < static_cast<int32_t>(width); i++)
      {
         auto tile_number = tiles[i + j * width];

         if (tile_number != 0)
         {
            auto tile_id = tile_number - first_id;

            if (tile_id == 33)
            {
               auto lever = std::make_shared<Lever>(parent);

               if (data._tmx_layer->_properties)
               {
                  auto z_it = data._tmx_layer->_properties->_map.find("z");
                  if (z_it != data._tmx_layer->_properties->_map.end())
                  {
                     auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
                     lever->setZ(z_index);
                  }
               }

               // sprite is two tiles high
               const auto x = PIXELS_PER_TILE * i;
               const auto y = PIXELS_PER_TILE * (j - 1);

               lever->_rect.left = static_cast<float>(x);
               lever->_rect.top = static_cast<float>(y);
               lever->_rect.width = PIXELS_PER_TILE * 3;
               lever->_rect.height = PIXELS_PER_TILE * 2;

               lever->_sprite.setPosition(static_cast<float>(x), static_cast<float>(y));
               lever->_texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "levers.png");
               lever->_sprite.setTexture(*lever->_texture);
               lever->updateSprite();

               levers.push_back(lever);
            }
         }
      }
   }

   return levers;
}

//-----------------------------------------------------------------------------
void Lever::setup(const GameDeserializeData& data)
{
   if (data._tmx_object->_properties)
   {
      auto z_it = data._tmx_object->_properties->_map.find("z");
      if (z_it != data._tmx_object->_properties->_map.end())
      {
         auto z_index = static_cast<uint32_t>(z_it->second->_value_int.value());
         setZ(z_index);
      }

      auto enabled_it = data._tmx_object->_properties->_map.find("enabled");
      if (enabled_it != data._tmx_object->_properties->_map.end())
      {
         _enabled = enabled_it->second->_value_bool.value();
      }

      auto serialized_it = data._tmx_object->_properties->_map.find("serialized");
      if (serialized_it != data._tmx_object->_properties->_map.end())
      {
         _serialized = serialized_it->second->_value_bool.value();
      }
   }

   const auto x = data._tmx_object->_x_px;
   const auto y = data._tmx_object->_y_px;

   _rect.left = x;
   _rect.top = y;
   _rect.width = PIXELS_PER_TILE * 3;
   _rect.height = PIXELS_PER_TILE * 2;

   _sprite.setPosition(x, y);
   _texture = TexturePool::getInstance().get(data._base_path / "tilesets" / "levers.png");
   _sprite.setTexture(*_texture);

   setObjectId(data._tmx_object->_name);

   updateSprite();
}

//-----------------------------------------------------------------------------
void Lever::updateSprite()
{
   if (_reached && (_target_state == State::Right))
   {
      _sprite.setTextureRect(
         {(static_cast<int32_t>(_idle_time_s * idle_animation_speed) % 6) * PIXELS_PER_TILE * 3,
          PIXELS_PER_TILE * 3 * 2,
          PIXELS_PER_TILE * 3,
          PIXELS_PER_TILE * 3}
      );
   }
   else
   {
      const auto left = _dir == -1;

      _sprite.setTextureRect(
         {left ? (left_offset - _offset * 3 * PIXELS_PER_TILE) : (_offset * 3 * PIXELS_PER_TILE),
          left ? (3 * PIXELS_PER_TILE) : 0,
          PIXELS_PER_TILE * 3,
          PIXELS_PER_TILE * 3}
      );
   }
}

//-----------------------------------------------------------------------------
const sf::FloatRect& Lever::getPixelRect() const
{
   return _rect;
}

//-----------------------------------------------------------------------------
Lever::Lever(GameNode* parent) : GameNode(parent)
{
   setClassName(typeid(Lever).name());
}

//-----------------------------------------------------------------------------
void Lever::updateDirection()
{
   if (_target_state == State::Left)
   {
      _dir = -1;
   }
   else if (_target_state == State::Right)
   {
      _dir = 1;
   }
   else if (_target_state == State::Middle)
   {
      _dir = (_state_previous == State::Left) ? 1 : -1;
   };
}

//-----------------------------------------------------------------------------
void Lever::updateTargetPositionReached()
{
   if (_target_state == State::Left)
   {
      _reached = (_offset == 0);
   }
   else if (_target_state == State::Right)
   {
      _reached = (_offset == sprites_per_row - 1);
   }
   else if (_target_state == State::Middle)
   {
      _reached = (_offset == row_center);
   };
}

//-----------------------------------------------------------------------------
void Lever::update(const sf::Time& dt)
{
   const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
   _player_at_lever = _rect.intersects(player_rect);

   updateTargetPositionReached();
   updateDirection();

   if (!_reached)
   {
      _offset += _dir;
   }
   else
   {
      if (_reached_previous)
      {
         _idle_time_s += dt.asSeconds();
      }
      else
      {
         _idle_time_s = 0.0f;
      }
   }

   updateSprite();

   _reached_previous = _reached;
}

//-----------------------------------------------------------------------------
void Lever::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}

//-----------------------------------------------------------------------------
std::optional<sf::FloatRect> Lever::getBoundingBoxPx()
{
   return sf::FloatRect(_rect.left, _rect.top, _rect.width, _rect.height);
}

//-----------------------------------------------------------------------------
void Lever::setEnabled(bool enabled)
{
   if (enabled)
   {
      _target_state = State::Right;
      _state_previous = State::Left;
   }
   else
   {
      _target_state = State::Left;
      _state_previous = State::Right;
   }

   // during setup the callbacks are not connected yet, so it's safe to call setEnabled during setup
   updateReceivers();
}

//-----------------------------------------------------------------------------
void Lever::updateReceivers()
{
   for (auto& cb : _callbacks)
   {
      cb(static_cast<int32_t>(_target_state));
   }
}

//-----------------------------------------------------------------------------
void Lever::toggle()
{
   if (!_player_at_lever)
   {
      return;
   }

   if (_type == Type::TwoState)
   {
      switch (_target_state)
      {
         case State::Left:
            _target_state = State::Right;
            break;
         case State::Right:
            _target_state = State::Left;
            break;
         case State::Middle:
            break;
      }
   }

   else if (_type == Type::TriState)
   {
      switch (_target_state)
      {
         case State::Left:
         {
            _target_state = State::Middle;
            break;
         }
         case State::Middle:
         {
            if (_state_previous == State::Left)
            {
               _target_state = State::Right;
            }
            else
            {
               _target_state = State::Left;
            }
            break;
         }
         case State::Right:
         {
            _target_state = State::Middle;
            break;
         }
      }

      _state_previous = _target_state;
   }

   GameMechanismAudio::play(_target_state == State::Left ? GameMechanismAudio::Effect::LeverOff : GameMechanismAudio::Effect::LeverOn);
   updateReceivers();
}

//-----------------------------------------------------------------------------
void Lever::setCallbacks(const std::vector<Callback>& callbacks)
{
   _callbacks = callbacks;
}

//-----------------------------------------------------------------------------
void Lever::addSearchRect(const std::shared_ptr<TmxObject>& rect)
{
   __rectangles.push_back(rect);
}

//-----------------------------------------------------------------------------
void Lever::merge(
   const std::vector<std::shared_ptr<GameMechanism>>& levers,
   const std::vector<std::shared_ptr<GameMechanism>>& lasers,
   const std::vector<std::shared_ptr<GameMechanism>>& platforms,
   const std::vector<std::shared_ptr<GameMechanism>>& fans,
   const std::vector<std::shared_ptr<GameMechanism>>& belts,
   const std::vector<std::shared_ptr<GameMechanism>>& spikes,
   const std::vector<std::shared_ptr<GameMechanism>>& spike_blocks,
   const std::vector<std::shared_ptr<GameMechanism>>& on_off_blocks,
   const std::vector<std::shared_ptr<GameMechanism>>& rotating_blades,
   const std::vector<std::shared_ptr<GameMechanism>>& doors
)
{
   for (auto rect : __rectangles)
   {
      sf::FloatRect search_rect;
      search_rect.left = rect->_x_px;
      search_rect.top = rect->_y_px;
      search_rect.width = rect->_width_px;
      search_rect.height = rect->_height_px;

      // Log::Info()
      //    << "x: " << searchRect.left << " "
      //    << "y: " << searchRect.top << " "
      //    << "w: " << searchRect.width << " "
      //    << "h: " << searchRect.height << " ";

      for (auto& tmp : levers)
      {
         auto lever = std::dynamic_pointer_cast<Lever>(tmp);

         if (lever->_rect.intersects(search_rect))
         {
            std::vector<Callback> callbacks;

            for (auto& l : lasers)
            {
               auto mechanism = std::dynamic_pointer_cast<Laser>(l);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& b : belts)
            {
               auto mechanism = std::dynamic_pointer_cast<ConveyorBelt>(b);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& f : fans)
            {
               auto mechanism = std::dynamic_pointer_cast<Fan>(f);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& p : platforms)
            {
               auto mechanism = std::dynamic_pointer_cast<MovingPlatform>(p);

               const auto& pixel_path = mechanism->getPixelPath();

               for (const auto& pixel : pixel_path)
               {
                  if (search_rect.contains(pixel.x, pixel.y))
                  {
                     callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });

                     break;
                  }
               }
            }

            for (auto& s : spikes)
            {
               auto mechanism = std::dynamic_pointer_cast<Spikes>(s);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& s : spike_blocks)
            {
               auto mechanism = std::dynamic_pointer_cast<SpikeBlock>(s);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& instance : on_off_blocks)
            {
               auto mechanism = std::dynamic_pointer_cast<OnOffBlock>(instance);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& instance : rotating_blades)
            {
               auto mechanism = std::dynamic_pointer_cast<RotatingBlade>(instance);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([mechanism](int32_t state) { mechanism->setEnabled(state == -1 ? false : true); });
               }
            }

            for (auto& instance : doors)
            {
               auto mechanism = std::dynamic_pointer_cast<Door>(instance);

               if (mechanism->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back(
                     [mechanism](int32_t state)
                     {
                        if (state == -1)
                        {
                           mechanism->close();
                        }
                        else
                        {
                           mechanism->open();
                        }
                     }
                  );
               }
            }

            // the rect can be configured to enable a lever, too
            // this approach could be deprecated at a later point in time because
            // the standard way should be to configure everything via the lever
            // tmxobject properties
            if (rect->_properties)
            {
               auto enabled_it = rect->_properties->_map.find("enabled");
               if (enabled_it != rect->_properties->_map.end())
               {
                  const auto enabled = enabled_it->second->_value_bool.value();
                  lever->setEnabled(enabled);
               }
            }

            lever->setCallbacks(callbacks);
            lever->updateReceivers();

            break;
         }
      }
   }

   __rectangles.clear();
}

//-----------------------------------------------------------------------------
void Lever::serializeState(nlohmann::json& j)
{
   if (!_serialized)
   {
      return;
   }

   if (_object_id.empty())
   {
      Log::Warning() << "a lever has been configured to be serialized but it doesn't have any id";
      return;
   }

   j[_object_id] = {{"state", static_cast<int32_t>(_target_state)}};
}

//-----------------------------------------------------------------------------
void Lever::deserializeState(const nlohmann::json& j)
{
   _target_state = static_cast<State>(j.at("state").get<int32_t>());
   _enabled = (_target_state == State::Right);
   updateReceivers();
}
