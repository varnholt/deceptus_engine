#include "spikes.h"

#include "constants.h"
#include "player/player.h"
#include "texturepool.h"

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
static const auto updateTimeUpMs = 5;
static const auto updateTimeDownMs = 30;
static const auto downTime = 2000;
static const auto upTime = 2000;
static const auto trapTime = 250;
}


Spikes::Spikes(GameNode* parent)
 : GameNode(parent)
{
   setName(typeid(Spikes).name());
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

      if (_elapsed_ms < upTime)
      {
         wait = true;
      }
   }

   if (_tu == SPIKES_PER_ROW - 1)
   {
      _triggered = true;

      if (_elapsed_ms < downTime)
      {
         wait = true;
      }
   }

   const auto updateTime = (_triggered ? updateTimeUpMs : updateTimeDownMs);
   if (!wait && _elapsed_ms > updateTime)
   {
      _elapsed_ms = (_elapsed_ms % updateTime);

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

      if (_elapsed_ms < upTime)
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
         if (_elapsed_ms < trapTime)
         {
            return;
         }
      }
      else
      {
         return;
      }
   }

   const auto updateTime = (_triggered ? updateTimeUpMs : updateTimeDownMs);
   if (_elapsed_ms > updateTime)
   {
      _elapsed_ms = (_elapsed_ms % updateTime);

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
   _sprite.setTextureRect({_tu * PIXELS_PER_TILE, _tv * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE});
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
      auto playerRect = Player::getCurrent()->getPlayerPixelRect();
      if (playerRect.intersects(_pixel_rect))
      {
         Player::getCurrent()->damage(100);
      }
   }
}


std::vector<std::shared_ptr<Spikes> > Spikes::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   Mode mode
)
{
   auto texture = TexturePool::getInstance().get(basePath / "tilesets" / "spikes.png");

   std::vector<std::shared_ptr<Spikes>> allSpikes;

   const auto tiles    = layer->_data;
   const auto width    = layer->_width_px;
   const auto height   = layer->_height_px;
   const auto firstId  = tileSet->_first_gid;

   const int32_t tilesPerRow = texture->getSize().x / PIXELS_PER_TILE;

   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         auto tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto id = (tileNumber - firstId);
            auto spikes = std::make_shared<Spikes>();
            spikes->_texture = texture;

            allSpikes.push_back(spikes);

            spikes->_tile_position.x = static_cast<float>(i);
            spikes->_tile_position.y = static_cast<float>(j);

            // std::cout << "look up: " << id << std::endl;

            spikes->_tu = static_cast<int32_t>(id % tilesPerRow);
            spikes->_tv = static_cast<int32_t>(id / tilesPerRow);

            // spikes->mMode = Mode::Interval;
            spikes->_mode = mode;

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

   return allSpikes;
}

