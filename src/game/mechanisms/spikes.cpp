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


void Spikes::draw(sf::RenderTarget& window)
{
   window.draw(mSprite);
}


void Spikes::updateInterval()
{
   auto wait = false;

   if (mTu == SPIKES_TILE_INDEX_UP)
   {
      mTriggered = false;

      if (mElapsedMs < upTime)
      {
         wait = true;
      }
   }

   if (mTu == SPIKES_PER_ROW - 1)
   {
      mTriggered = true;

      if (mElapsedMs < downTime)
      {
         wait = true;
      }
   }

   const auto updateTime = (mTriggered ? updateTimeUpMs : updateTimeDownMs);
   if (!wait && mElapsedMs > updateTime)
   {
      mElapsedMs = (mElapsedMs % updateTime);

      if (mTriggered)
      {
         // extract
         mTu -= 2;
         if (mTu < SPIKES_TILE_INDEX_UP)
         {
            mTu = SPIKES_TILE_INDEX_UP;
         }
      }
      else
      {
         // retract
         mTu++;
         if (mTu >= SPIKES_PER_ROW)
         {
            mTu = SPIKES_PER_ROW - 1;
         }
      }
   }

   mDeadly = (mTu < 10);
}


void Spikes::updateTrap()
{
   if (mTu == SPIKES_TILE_INDEX_UP)
   {
      mTriggered = false;

      if (mElapsedMs < upTime)
      {
         return;
      }
   }

   // trap trigger is done via intersection
   if (mTu == TRAP_START_TILE)
   {
      auto playerRect = Player::getCurrent()->getPlayerPixelRect();
      if (playerRect.intersects(mPixelRect))
      {
         // start counting from first intersection
         if (!mTriggered)
         {
            mElapsedMs = 0;
         }

         mTriggered = true;
      }

      // trap was activated
      if (mTriggered)
      {
         if (mElapsedMs < trapTime)
         {
            return;
         }
      }
      else
      {
         return;
      }
   }

   const auto updateTime = (mTriggered ? updateTimeUpMs : updateTimeDownMs);
   if (mElapsedMs > updateTime)
   {
      mElapsedMs = (mElapsedMs % updateTime);

      if (mTriggered)
      {
         // extract
         mTu-=2;
         if (mTu <= SPIKES_TILE_INDEX_UP)
         {
            mTu = SPIKES_TILE_INDEX_UP;
         }
      }
      else
      {
         // retract
         mTu++;
         if (mTu >= SPIKES_PER_ROW)
         {
            mTu = SPIKES_PER_ROW - 1;
         }
      }
   }

   mDeadly = (mTu < 10);
}


void Spikes::updateToggled()
{
   if (isEnabled())
   {
      // initially mTu is in some wonky state due to that weird sprite set
      if (mTu > SPIKES_TILE_INDEX_UP)
      {
         mTu--;
      }
      else if (mTu < SPIKES_TILE_INDEX_UP)
      {
         mTu++;
      }
   }
   else
   {
      if (mTu < SPIKES_PER_ROW)
      {
         mTu++;
      }
   }

   mDeadly = (mTu < 10);
}


Spikes::Mode Spikes::getMode() const
{
   return mMode;
}


void Spikes::setMode(Mode mode)
{
   mMode = mode;
}


const sf::IntRect& Spikes::getPixelRect() const
{
   return mPixelRect;
}


void Spikes::updateSpriteRect()
{
   mSprite.setTextureRect({mTu * PIXELS_PER_TILE, mTv * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE});
}


void Spikes::update(const sf::Time& dt)
{
   mElapsedMs += dt.asMilliseconds();

   switch (mMode)
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

   if (mDeadly)
   {
      // check for intersection with player
      auto playerRect = Player::getCurrent()->getPlayerPixelRect();
      if (playerRect.intersects(mPixelRect))
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

   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

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
            spikes->mTexture = texture;

            allSpikes.push_back(spikes);

            spikes->mTilePosition.x = static_cast<float>(i);
            spikes->mTilePosition.y = static_cast<float>(j);

            // std::cout << "look up: " << id << std::endl;

            spikes->mTu = static_cast<int32_t>(id % tilesPerRow);
            spikes->mTv = static_cast<int32_t>(id / tilesPerRow);

            // spikes->mMode = Mode::Interval;
            spikes->mMode = mode;

            if (mode == Mode::Trap)
            {
               spikes->mTu = TRAP_START_TILE;
            }

            if (layer->mProperties != nullptr)
            {
               spikes->setZ(layer->mProperties->mMap["z"]->mValueInt.value());
            }

            spikes->mPixelRect = {
               static_cast<int32_t>(i * PIXELS_PER_TILE) + TOLERANCE_PIXELS,
               static_cast<int32_t>(j * PIXELS_PER_TILE) + TOLERANCE_PIXELS,
               PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS),
               PIXELS_PER_TILE - (2 * TOLERANCE_PIXELS)
            };

            sf::Sprite sprite;
            sprite.setTexture(*spikes->mTexture);
            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            spikes->mSprite = sprite;
            spikes->updateSpriteRect();
         }
      }
   }

   return allSpikes;
}

