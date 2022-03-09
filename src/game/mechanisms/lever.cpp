#include "lever.h"

#include "constants.h"
#include "conveyorbelt.h"
#include "gamemechanism.h"
#include "fan.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "laser.h"
#include "movingplatform.h"
#include "player/player.h"
#include "spikes.h"
#include "spikeblock.h"
#include "texturepool.h"

#include <iostream>


std::vector<TmxObject*> Lever::__rectangles;


namespace
{
constexpr auto sprites_per_row = 11;
constexpr auto row_center = 6;
constexpr auto left_offset = (sprites_per_row - 1) * 3 * PIXELS_PER_TILE;
}


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

   auto tiles    = data._tmx_layer->_data;
   auto width    = data._tmx_layer->_width_px;
   auto height   = data._tmx_layer->_height_px;
   auto first_id = data._tmx_tileset->_first_gid;

   // populate the vertex array, with one quad per tile
   for (auto j = 0; j < static_cast<int32_t>(height); j++)
   {
      for (auto i = 0; i < static_cast<int32_t>(width); i++)
      {
         // get the current tile number
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

               lever->_rect.left = x;
               lever->_rect.top = y;
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
void Lever::updateSprite()
{
   const auto left = _dir == -1;

   _sprite.setTextureRect({
      left ? (left_offset - _offset * 3 * PIXELS_PER_TILE) : (_offset * 3 * PIXELS_PER_TILE),
      left ? (3 * PIXELS_PER_TILE) : 0,
      PIXELS_PER_TILE * 3,
      PIXELS_PER_TILE * 2
   });
}


//-----------------------------------------------------------------------------
const sf::Rect<int32_t>& Lever::getPixelRect() const
{
   return _rect;
}


//-----------------------------------------------------------------------------
Lever::Lever(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(Lever).name());
}


//-----------------------------------------------------------------------------
void Lever::update(const sf::Time& /*dt*/)
{
   const auto& player_rect = Player::getCurrent()->getPlayerPixelRect();
   _player_at_lever = _rect.intersects(player_rect);

   bool reached = false;

   if (_target_state == State::Left)
   {
      _dir = -1;
      reached = (_offset == 0);
   }
   else if (_target_state == State::Right)
   {
      _dir = 1;
      reached = (_offset == sprites_per_row - 1);
   }
   else if (_target_state == State::Middle)
   {
      _dir = (_state_previous == State::Left) ? 1 : -1;
      reached = (_offset == row_center);
   };

   if (reached)
   {
      return;
   }

   _offset += _dir;

   updateSprite();
}


//-----------------------------------------------------------------------------
void Lever::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
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

   updateReceivers();
}


//-----------------------------------------------------------------------------
void Lever::setCallbacks(const std::vector<Callback>& callbacks)
{
   _callbacks = callbacks;
}


//-----------------------------------------------------------------------------
void Lever::addSearchRect(TmxObject* rect)
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
   const std::vector<std::shared_ptr<GameMechanism>>& spike_blocks
)
{
   for (auto rect : __rectangles)
   {
      sf::Rect<int32_t> search_rect;
      search_rect.left = static_cast<int32_t>(rect->_x_px);
      search_rect.top = static_cast<int32_t>(rect->_y_px);
      search_rect.width = static_cast<int32_t>(rect->_width_px);
      search_rect.height = static_cast<int32_t>(rect->_height_px);

      bool enabled = true;
      if (rect->_properties)
      {
         auto enabledIt = rect->_properties->_map.find("enabled");
         if (enabledIt != rect->_properties->_map.end())
         {
            enabled = enabledIt->second->_value_bool.value();
         }
      }

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
               auto laser = std::dynamic_pointer_cast<Laser>(l);

               if (laser->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([laser](int32_t state) {
                        laser->setEnabled(state == -1 ? false : true);
                     }
                  );
               }
            }

            for (auto& b : belts)
            {
               auto belt = std::dynamic_pointer_cast<ConveyorBelt>(b);

               if (belt->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([belt](int32_t state) {
                        belt->setEnabled(state == -1 ? false : true);
                     }
                  );
               }
            }

            for (auto& f : fans)
            {
               auto fan = std::dynamic_pointer_cast<Fan>(f);

               if (fan->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([fan](int32_t state) {
                        fan->setEnabled(state == -1 ? false : true);
                     }
                  );
               }
            }

            for (auto& p : platforms)
            {
               auto platform = std::dynamic_pointer_cast<MovingPlatform>(p);

               const auto& pixel_path = platform->getPixelPath();

               for (const auto& pixel : pixel_path)
               {
                  if (search_rect.contains(static_cast<int32_t>(pixel.x), static_cast<int32_t>(pixel.y)))
                  {
                     callbacks.push_back([platform](int32_t state) {
                           platform->setEnabled(state == -1 ? false : true);
                        }
                     );

                     break;
                  }
               }
            }

            for (auto& s : spikes)
            {
               auto spikes = std::dynamic_pointer_cast<Spikes>(s);

               if (spikes->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([spikes](int32_t state) {
                        spikes->setEnabled(state == -1 ? false : true);
                     }
                  );
               }
            }

            for (auto& s : spike_blocks)
            {
               auto spike_block = std::dynamic_pointer_cast<SpikeBlock>(s);

               if (spike_block->getPixelRect().intersects(search_rect))
               {
                  callbacks.push_back([spike_block](int32_t state) {
                        spike_block->setEnabled(state == -1 ? false : true);
                     }
                  );
               }
            }

            lever->setEnabled(enabled);
            lever->setCallbacks(callbacks);
            lever->updateReceivers();

            break;
         }
      }
   }

   __rectangles.clear();
}


