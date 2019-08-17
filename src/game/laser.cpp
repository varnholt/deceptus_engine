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
std::vector<TmxObject*> Laser::mObjects;
std::vector<Laser*> Laser::mLasers;
std::vector<std::array<int32_t, 9>> Laser::mTiles;


//-----------------------------------------------------------------------------
void Laser::draw(sf::RenderTarget& window)
{
   mSprite.setTextureRect(
      sf::IntRect(
         mTu * PIXELS_PER_TILE + mTileIndex * PIXELS_PER_TILE,
         mTv * PIXELS_PER_TILE,
         PIXELS_PER_TILE,
         PIXELS_PER_TILE
      )
   );

   window.draw(mSprite);
}


//-----------------------------------------------------------------------------
void Laser::update(const sf::Time& dt)
{
   mTime += dt.asMilliseconds();

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

      mTileAnimation += (dt.asSeconds() * 10.0f * dir);
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
std::vector<Laser*> Laser::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   addTiles();

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
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            laser->mSprite = sprite;
            mLasers.push_back(laser);
         }
      }
   }

   return lasers;
}


void Laser::addObject(TmxObject* object)
{
   mObjects.push_back(object);
}


void Laser::addTiles()
{
   mTiles.push_back({0,0,0,1,1,1,0,0,0});
   mTiles.push_back({0,1,0,0,1,0,0,1,0});
   mTiles.push_back({0,1,0,0,1,0,0,1,0});
   mTiles.push_back({0,1,0,0,1,0,0,1,0});
   mTiles.push_back({0,0,0,1,1,1,0,0,0});
   mTiles.push_back({0,0,0,1,1,1,0,0,0});
   mTiles.push_back({0,1,0,1,1,1,0,1,0});
   mTiles.push_back({0,0,0,1,1,1,0,1,0});
   mTiles.push_back({0,1,0,1,1,0,0,1,0});
   mTiles.push_back({0,1,0,1,1,1,0,0,0});
   mTiles.push_back({0,1,0,0,1,1,0,1,0});
   mTiles.push_back({0,0,0,0,1,1,0,1,0});
   mTiles.push_back({0,0,0,1,1,0,0,1,0});
   mTiles.push_back({0,1,0,0,1,1,0,0,0});
   mTiles.push_back({0,1,0,1,1,0,0,0,0});
   mTiles.push_back({0,1,0,0,1,0,0,0,0});
   mTiles.push_back({0,0,0,0,1,0,0,1,0});
   mTiles.push_back({0,0,0,0,1,1,0,0,0});
   mTiles.push_back({0,0,0,1,1,0,0,0,0});
}


void Laser::collide(const sf::Rect<int32_t>& playerRect)
{
   const auto it =
      std::find_if(std::begin(mLasers), std::end(mLasers), [playerRect](Laser* laser){
            sf::Rect<int32_t> itemRect;

            itemRect.left = static_cast<int32_t>(laser->mTilePosition.x * PIXELS_PER_TILE);
            itemRect.top = static_cast<int32_t>(laser->mTilePosition.y * PIXELS_PER_TILE);

            itemRect.width = PIXELS_PER_TILE;
            itemRect.height = PIXELS_PER_TILE;

            const auto roughIntersection = playerRect.intersects(itemRect);

            if (laser->mTileIndex == 0 && roughIntersection)
            {
               const auto tileId = static_cast<uint32_t>(laser->mTv);
               const auto tile = mTiles[tileId];

               auto x = 0u;
               auto y = 0u;
               for (auto i = 0u; i < 9; i++)
               {
                  if (tile[i] == 1)
                  {
                     sf::Rect<int32_t> rect;

                     rect.left = static_cast<int32_t>(laser->mTilePosition.x * PIXELS_PER_TILE) + (x * PIXELS_PER_PHYSICS_TILE);
                     rect.top = static_cast<int32_t>(laser->mTilePosition.y * PIXELS_PER_TILE) + (y * PIXELS_PER_PHYSICS_TILE);

                     rect.width = PIXELS_PER_PHYSICS_TILE;
                     rect.height = PIXELS_PER_PHYSICS_TILE;

                     const auto fineIntersection = playerRect.intersects(rect);

                     if (fineIntersection)
                     {
                        return true;
                     }
                  }

                  x++;

                  if (i > 0 && ( (i + 1) % 3 == 0))
                  {
                     x = 0;
                     y++;
                  }
               }

               return false;
            }

            return false;
         }
      );

   if (it != mLasers.end())
   {
      // player is dead
      Player::getPlayer(0)->damage(100);
   }
}


void Laser::merge()
{
   for (auto object : mObjects)
   {
      const auto x = static_cast<int32_t>(object->mX      / PIXELS_PER_TILE );
      const auto y = static_cast<int32_t>(object->mY      / PIXELS_PER_TILE);
      const auto w = static_cast<int32_t>(object->mWidth  / PIXELS_PER_TILE );
      const auto h = static_cast<int32_t>(object->mHeight / PIXELS_PER_TILE);

      for (auto yi = y; yi < y + h; yi++)
      {
         for (auto xi = x; xi < x + w; xi++)
         {
            for (auto laser : mLasers)
            {
               if (
                     static_cast<int32_t>(laser->mTilePosition.x) == xi
                  && static_cast<int32_t>(laser->mTilePosition.y) == yi
               )
               {
                   if (object->mProperties != nullptr)
                   {
                      auto it = object->mProperties->mMap.find("on_time");
                      if (it != object->mProperties->mMap.end())
                      {
                          laser->mSignalPlot.push_back(Signal{static_cast<uint32_t>(it->second->mValueInt), true});
                      }

                      it = object->mProperties->mMap.find("off_time");
                      if (it != object->mProperties->mMap.end())
                      {
                          laser->mSignalPlot.push_back(Signal{static_cast<uint32_t>(it->second->mValueInt), false});
                      }
                   }
               }
            }
         }
      }
   }
}


