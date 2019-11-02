#include "spikes.h"

#include "constants.h"
#include "texturepool.h"

#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxtileset.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"

#include <iostream>


sf::Texture Spikes::mTexture;
#define SPIKES_PER_ROW 22


namespace
{
static const auto updateTimeUpMs = 5;
static const auto updateTimeDownMs = 30;
static const auto downTime = 2.0f;
static const auto upTime = 2.0f;
}


void Spikes::draw(sf::RenderTarget& window)
{
   window.draw(mSprite);
}


void Spikes::updateInterval()
{
   auto wait = false;

   if (mTu == 0)
   {
      mTriggered = false;

      if (mElapsed.asSeconds() < upTime)
      {
         wait = true;
      }
      else
      {
         // skip the first few frames on the way back..
         mTu = 7;
      }
   }

   if (mTu == SPIKES_PER_ROW - 1)
   {
      mTriggered = true;

      if (mElapsed.asSeconds() < downTime)
      {
         wait = true;
      }
   }

   if (!wait && mElapsed.asMilliseconds() > (mTriggered ? updateTimeUpMs : updateTimeDownMs))
   {
      mElapsed = {};

      if (mTriggered)
      {
         mTu--;
         if (mTu < 0)
         {
            mTu = SPIKES_PER_ROW - 1;
         }
      }
      else
      {
         // retract
         mTu++;
         if (mTu >= SPIKES_PER_ROW)
         {
            mTu = 0;
         }
      }
   }
}


void Spikes::update(const sf::Time& dt)
{
   mElapsed += dt;

   switch (mMode)
   {
      case Mode::Trap:
      {
         break;
      }
      case Mode::Interval:
      {
         updateInterval();
         break;
      }
      case Mode::Toggled:
      {
         break;
      }
      case Mode::Invalid:
      {
         break;
      }
   }

   mSprite.setTextureRect({mTu * PIXELS_PER_TILE, mTv * PIXELS_PER_TILE, PIXELS_PER_TILE, PIXELS_PER_TILE});

   if (mDeadly)
   {
      // check for intersection with player
   }
}


std::vector<std::shared_ptr<Spikes> > Spikes::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>& /*world*/
)
{
   mTexture = *TexturePool::getInstance().get(basePath / "tilesets" / "spikes.png");

   std::vector<std::shared_ptr<Spikes>> allSpikes;

   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

   const auto tilesPerRow = mTexture.getSize().x / PIXELS_PER_TILE;

   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         auto tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto id = (tileNumber - firstId);
            auto spikes = std::make_shared<Spikes>();
            spikes->mMode = Mode::Interval;
            allSpikes.push_back(spikes);

            spikes->mTilePosition.x = static_cast<float>(i);
            spikes->mTilePosition.y = static_cast<float>(j);

            spikes->mPixelPosition.x = spikes->mTilePosition.x * PIXELS_PER_TILE;
            spikes->mPixelPosition.y = spikes->mTilePosition.y * PIXELS_PER_TILE;

            // std::cout << "look up: " << id << std::endl;

            spikes->mTu = id % tilesPerRow;
            spikes->mTv = id / tilesPerRow;

            if (layer->mProperties != nullptr)
            {
               spikes->setZ(layer->mProperties->mMap["z"]->mValueInt);
            }

            sf::Sprite sprite;
            sprite.setTexture(mTexture);
            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            spikes->mSprite = sprite;
         }
      }
   }

   return allSpikes;
}


int32_t Spikes::getZ() const
{
   return mZ;
}


void Spikes::setZ(const int32_t& z)
{
   mZ = z;
}

