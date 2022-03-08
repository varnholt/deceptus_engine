#include "spikes.h"

#include "constants.h"
#include "player/player.h"
#include "texturepool.h"

#include "framework/tools/log.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"

#include <iostream>


#define SPIKES_PER_ROW 18
#define TOLERANCE_PIXELS 5
#define TRAP_START_TILE (SPIKES_PER_ROW - 4)
#define SPIKES_TILE_INDEX_UP 6
// -> 24 - 2 * 4 = 16px rect

namespace
{
constexpr auto update_time_up_ms = 5;
constexpr auto update_time_down_ms = 30;
constexpr auto down_time_ms = 2000;
constexpr auto up_tims_ms = 2000;
constexpr auto trap_time_ms = 250;
}


Spikes::Spikes(GameNode* parent)
 : GameNode(parent)
{
   setClassName(typeid(Spikes).name());
}


void Spikes::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(_sprite);
}


void Spikes::updateInterval()
{
   auto wait = false;

   if (_tu == SPIKES_TILE_INDEX_UP)
   {
      _triggered = false;

      if (_elapsed_ms < up_tims_ms)
      {
         wait = true;
      }
   }

   if (_tu == SPIKES_PER_ROW - 1)
   {
      _triggered = true;

      if (_elapsed_ms < down_time_ms)
      {
         wait = true;
      }
   }

   const auto update_time_ms = (_triggered ? update_time_up_ms : update_time_down_ms);
   if (!wait && _elapsed_ms > update_time_ms)
   {
      _elapsed_ms = (_elapsed_ms % update_time_ms);

      if (_triggered)
      {
         // extract
         _tu -= 2;
         if (_tu < SPIKES_TILE_INDEX_UP)
         {
            _tu = SPIKES_TILE_INDEX_UP;
         }
      }
      else
      {
         // retract
         _tu++;
         if (_tu >= SPIKES_PER_ROW)
         {
            _tu = SPIKES_PER_ROW - 1;
         }
      }
   }

   _deadly = (_tu < 10);
}


void Spikes::updateTrap()
{
   if (_tu == SPIKES_TILE_INDEX_UP)
   {
      _triggered = false;

      if (_elapsed_ms < up_tims_ms)
      {
         return;
      }
   }

   // trap trigger is done via intersection
   if (_tu == TRAP_START_TILE)
   {
      auto playerRect = Player::getCurrent()->getPlayerPixelRect();
      if (playerRect.intersects(_pixel_rect))
      {
         // start counting from first intersection
         if (!_triggered)
         {
            _elapsed_ms = 0;
         }

         _triggered = true;
      }

      // trap was activated
      if (_triggered)
      {
         if (_elapsed_ms < trap_time_ms)
         {
            return;
         }
      }
      else
      {
         return;
      }
   }

   const auto udate_time_ms = (_triggered ? update_time_up_ms : update_time_down_ms);
   if (_elapsed_ms > udate_time_ms)
   {
      _elapsed_ms = (_elapsed_ms % udate_time_ms);

      if (_triggered)
      {
         // extract
         _tu-=2;
         if (_tu <= SPIKES_TILE_INDEX_UP)
         {
            _tu = SPIKES_TILE_INDEX_UP;
         }
      }
      else
      {
         // retract
         _tu++;
         if (_tu >= SPIKES_PER_ROW)
         {
            _tu = SPIKES_PER_ROW - 1;
         }
      }
   }

   _deadly = (_tu < 10);
}


void Spikes::updateToggled()
{
   if (isEnabled())
   {
      // initially mTu is in some wonky state due to that weird sprite set
      if (_tu > SPIKES_TILE_INDEX_UP)
      {
         _tu--;
      }
      else if (_tu < SPIKES_TILE_INDEX_UP)
      {
         _tu++;
      }
   }
   else
   {
      if (_tu < SPIKES_PER_ROW)
      {
         _tu++;
      }
   }

   _deadly = (_tu < 10);
}


Spikes::Mode Spikes::getMode() const
{
   return _mode;
}


void Spikes::setMode(Mode mode)
{
   _mode = mode;
}


const sf::IntRect& Spikes::getPixelRect() const
{
   return _pixel_rect;
}


void Spikes::updateSpriteRect()
{
   _sprite.setTextureRect({
      _tu * PIXELS_PER_TILE,
      _tv * PIXELS_PER_TILE,
      PIXELS_PER_TILE,
      PIXELS_PER_TILE}
   );
}


void Spikes::update(const sf::Time& dt)
{
   _elapsed_ms += dt.asMilliseconds();

   switch (_mode)
   {
      case Mode::Trap:
      {
         updateTrap();
         break;
      }
      case Mode::Interval:
      {
         updateInterval();
         break;
      }
      case Mode::Toggled:
      {
         updateToggled();
         break;
      }
      case Mode::Invalid:
      {
         break;
      }
   }

   updateSpriteRect();

   if (_deadly)
   {
      // check for intersection with player
      const auto& player_rect = Player::getCurrent()->getPlayerPixelRect();
      if (player_rect.intersects(_pixel_rect))
      {
         Player::getCurrent()->damage(100);
      }
   }
}


std::vector<std::shared_ptr<Spikes>> Spikes::load(
   GameNode* parent,
   TmxLayer* layer,
   TmxTileSet* tileset,
   const std::filesystem::path& base_path,
   Mode mode
)
{
   if (!layer)
   {
      Log::Error() << "tmx layer is empty, please fix your level design";
      return {};
   }

   if (!tileset)
   {
      Log::Error() << "tmx tileset is empty, please fix your level design";
      return {};
   }

   auto texture = TexturePool::getInstance().get(base_path / "tilesets" / "spikes.png");

   std::vector<std::shared_ptr<Spikes>> all_spikes;

   const auto tiles    = layer->_data;
   const auto width    = layer->_width_px;
   const auto height   = layer->_height_px;
   const auto first_id = tileset->_first_gid;

   const int32_t tiles_per_row = texture->getSize().x / PIXELS_PER_TILE;

   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         auto tile_number = tiles[i + j * width];

         if (tile_number != 0)
         {
            const auto id = (tile_number - first_id);

            auto spikes = std::make_shared<Spikes>(parent);
            spikes->_texture = texture;
            spikes->_tile_position.x = static_cast<float>(i);
            spikes->_tile_position.y = static_cast<float>(j);
            spikes->_tu = static_cast<int32_t>(id % tiles_per_row);
            spikes->_tv = static_cast<int32_t>(id / tiles_per_row);
            spikes->_mode = mode;

            all_spikes.push_back(spikes);

            if (mode == Mode::Trap)
            {
               spikes->_tu = TRAP_START_TILE;
            }

            if (layer->_properties != nullptr)
            {
               spikes->setZ(layer->_properties->_map["z"]->_value_int.value());
            }

            spikes->_pixel_rect = {
               static_cast<int32_t>(i * PIXELS_PER_TILE) + TOLERANCE_PIXELS,
               static_cast<int32_t>(j * PIXELS_PER_TILE) + TOLERANCE_PIXELS,
               PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS),
               PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS)
            };

            sf::Sprite sprite;
            sprite.setTexture(*spikes->_texture);
            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            spikes->_sprite = sprite;
            spikes->updateSpriteRect();
         }
      }
   }

   return all_spikes;
}

