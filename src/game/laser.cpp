#include "laser.h"

// game
#include "constants.h"
#include "player.h"
#include "fixturenode.h"
#include "sfmlmath.h"
#include "texturepool.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


//-----------------------------------------------------------------------------
void Laser::draw(sf::RenderTarget& window)
{
   mSprite.setTextureRect(
      sf::IntRect(
         mTu * TILE_WIDTH + mTileIndex * TILE_WIDTH,
         mTv * TILE_HEIGHT,
         TILE_WIDTH,
         TILE_HEIGHT
      )
   );

   window.draw(mSprite);
}


//-----------------------------------------------------------------------------
void Laser::update(float dt)
{
   mTime += static_cast<uint32_t>(dt * 1000.0f);

   const auto& sig = mSignalPlot.at(mSignalIndex);

   // elapsed time exceeded signal duration
   if (mTime > sig.mDurationMs)
   {
      mOn = !mOn;
      mTime = 0;

      // reset signal index after 1 loop
      mSignalIndex++;
      if (mSignalIndex >= mSignalPlot.size())
      {
         mSignalIndex = 0;
      }
   }

   if ( (mOn && mTileIndex > 0) || (!mOn && mTileIndex < 6) )
   {
      // off sprite is rightmost, on sprite is leftmost
      auto dir = mOn ? -1 : 1;

      mTileAnimation += (dt * 10.0f * dir);
      mTileIndex = static_cast<int32_t>(mTileAnimation);

      // clamp tile index
      if (mTileIndex < 0)
      {
         mTileIndex = 0;
      }
      if (mTileIndex > 6)
      {
         mTileIndex = 6;
      }
   }
}


//-----------------------------------------------------------------------------
int Laser::getZ() const
{
   return mZ;
}


//-----------------------------------------------------------------------------
void Laser::setZ(int z)
{
   mZ = z;
}


//-----------------------------------------------------------------------------
std::vector<Laser *> Laser::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   std::vector<Laser*> lasers;

   sf::Vector2u tilesize = sf::Vector2u(tileSet->mTileWidth, tileSet->mTileHeight);
   const auto tiles    = layer->mData;
   const auto width    = layer->mWidth;
   const auto height   = layer->mHeight;
   const auto firstId  = tileSet->mFirstGid;

   // populate the vertex array, with one quad per tile
   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         // get the current tile number
         int tileNumber = tiles[i + j * width];

         if (tileNumber != 0)
         {
            auto laser = new Laser();
            lasers.push_back(laser);

            laser->mTilePosition.x = static_cast<float>(i);
            laser->mTilePosition.y = static_cast<float>(j);
            laser->mTexture = TexturePool::getInstance().get(basePath / tileSet->mImage->mSource);
            laser->mTu = (tileNumber - firstId) % (laser->mTexture->getSize().x / tilesize.x);
            laser->mTv = (tileNumber - firstId) / (laser->mTexture->getSize().x / tilesize.x);

            if (layer->mProperties != nullptr)
            {
               laser->setZ(layer->mProperties->mMap["z"]->mValueInt);
            }

            sf::Sprite sprite;
            sprite.setTexture(*laser->mTexture);
            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * TILE_WIDTH),
                  static_cast<float>(j * TILE_HEIGHT)
               )
            );

            laser->mSprite = sprite;
            laser->mSignalPlot = { {3000, true}, {3000, false} };
         }
      }
   }

   return lasers;
}


