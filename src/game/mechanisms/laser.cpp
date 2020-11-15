#include "laser.h"

// game
#include "constants.h"
#include "player/player.h"
#include "fixturenode.h"
#include "math/sfmlmath.h"
#include "texturepool.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


namespace
{
   static constexpr std::pair<int32_t, int32_t> rangeDisabled{0, 1};
   static constexpr std::pair<int32_t, int32_t> rangeEnabling{2, 9};
   static constexpr std::pair<int32_t, int32_t> rangeEnabled{10, 16};
   static constexpr std::pair<int32_t, int32_t> rangeDisabling{17, 20};

   static constexpr auto rangeDisabledDelta  = rangeDisabled.second  - rangeDisabled.first;
   static constexpr auto rangeEnabledDelta   = rangeEnabled.second   - rangeEnabled.first;
}


//-----------------------------------------------------------------------------
std::vector<TmxObject*> Laser::mObjects;
std::vector<std::shared_ptr<Laser>> Laser::mLasers;
std::vector<std::array<int32_t, 9>> Laser::mTilesVersion1;
std::vector<std::array<int32_t, 9>> Laser::mTilesVersion2;


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
void Laser::setEnabled(bool enabled)
{
   GameMechanism::setEnabled(enabled);
}


//-----------------------------------------------------------------------------
const sf::Rect<int32_t>& Laser::getPixelRect() const
{
   return mPixelRect;
}


//-----------------------------------------------------------------------------
void Laser::update(const sf::Time& dt)
{
   mTime += dt.asMilliseconds();

   if (mEnabled)
   {
      if (mSignalPlot.empty())
      {
         mOn = true;
      }
      else
      {
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
      }
   }
   else
   {
      mOn = false;
   }

   const auto previousTileIndex = mTileIndex;

   if (mVersion == MechanismVersion::Version1)
   {
      // shift tile index in right direction depending on the on/off state
      //
      // if the laser is switched on, move the tile index to the left
      // if the laser is switched off, move the tile index to the right
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
         else if (mTileIndex > 6)
         {
            mTileIndex = 6;
         }
      }
   }
   else if (mVersion == MechanismVersion::Version2)
   {
      //   +---------+-----------+
      //   | frame   | state     |
      //   +---------+-----------+
      //   |  0 -  1 | disabled  |
      //   |  2 -  9 | enabling  |
      //   | 10 - 16 | enabled   |
      //   | 17 - 21 | disabling |
      //   +---------+-----------+

      // disabled (!mOn and mTileIndex inside 0..1)
      // loop 0..1
      if (!mOn && mTileIndex >= rangeDisabled.first && mTileIndex <= rangeDisabled.second)
      {
         mTileAnimation += dt.asSeconds();
         mTileIndex = rangeDisabled.first + static_cast<int32_t>(mTileAnimation + mAnimationOffset) % (rangeDisabledDelta + 1);
      }

      // enabled (mOn and mTileIndex inside 10..16)
      // loop 10..16
      else if (mOn && mTileIndex >= rangeEnabled.first && mTileIndex <= rangeEnabled.second)
      {
         mTileAnimation += dt.asSeconds() * 10.0f;
         mTileIndex = rangeEnabled.first + static_cast<int32_t>(mTileAnimation + mAnimationOffset) % (rangeEnabledDelta + 1);
      }

      // enabling (mOn and mTileIndex outside 10..16)
      // go from 2..9, when 10 go to rangeEnabled
      else if (mOn)
      {
         mTileAnimation += dt.asSeconds() * 10.0f;

         if (mTileIndex < rangeEnabling.first || mTileIndex > rangeEnabling.second)
         {
            mTileAnimation = 0;
         }

         mTileIndex = rangeEnabling.first + static_cast<int32_t>(mTileAnimation);
      }

      // disabling (!mOn and mTileIndex outside 0..1)
      // go from 17..21, when 22 to to rangeDisabled
      else if (!mOn)
      {
         mTileAnimation += dt.asSeconds() * 10.0f;

         if (mTileIndex < rangeDisabling.first || mTileIndex > rangeDisabling.second)
         {
            mTileAnimation = 0;
         }

         mTileIndex = rangeDisabling.first + static_cast<int32_t>(mTileAnimation);

         // jumped out of range
         if (mTileIndex == rangeDisabling.second + 1)
         {
            mTileIndex = rangeDisabled.first;
         }
      }
   }

   // if (mVersion == MechanismVersion::Version2)
   // {
   //    if (mGroupId == 2)
   //    {
   //       if (previousTileIndex != mTileIndex)
   //       {
   //          std::cout << mGroupId << ": " << mTileIndex << std::endl;
   //       }
   //    }
   // }
}


//-----------------------------------------------------------------------------
void Laser::reset()
{
   mOn = true;
   mTileIndex = 0;
   mTileAnimation = 0.0f;
   mSignalIndex = 0;
   mTime = 0u;
}


//-----------------------------------------------------------------------------
void Laser::resetAll()
{
   mObjects.clear();
   mLasers.clear();
   mTilesVersion1.clear();
   mTilesVersion2.clear();
}


//-----------------------------------------------------------------------------
const sf::Vector2f& Laser::getTilePosition() const
{
   return mTilePosition;
}


//-----------------------------------------------------------------------------
const sf::Vector2f& Laser::getPixelPosition() const
{
   return mPixelPosition;
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Laser::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   const auto version = (layer->mName == "lasers") ? MechanismVersion::Version1 : MechanismVersion::Version2;

   resetAll();

   if (version == MechanismVersion::Version1)
   {
      addTilesVersion1();
   }
   else if (version == MechanismVersion::Version2)
   {
      addTilesVersion2();
   }

   std::vector<std::shared_ptr<GameMechanism>> lasers;

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
            auto laser = std::make_shared<Laser>();
            lasers.push_back(laser);

            laser->mVersion = version;

            laser->mTilePosition.x = static_cast<float>(i);
            laser->mTilePosition.y = static_cast<float>(j);

            laser->mPixelPosition.x = laser->mTilePosition.x * PIXELS_PER_TILE;
            laser->mPixelPosition.y = laser->mTilePosition.y * PIXELS_PER_TILE;

            laser->mPixelRect.left = static_cast<int32_t>(laser->mPixelPosition.x);
            laser->mPixelRect.top  = static_cast<int32_t>(laser->mPixelPosition.y);

            laser->mPixelRect.width  = PIXELS_PER_TILE;
            laser->mPixelRect.height = PIXELS_PER_TILE;

            laser->mTexture = TexturePool::getInstance().get(basePath / tileSet->mImage->mSource);

            laser->mTu = (tileNumber - firstId) % (laser->mTexture->getSize().x / tilesize.x);
            laser->mTv = (tileNumber - firstId) / (laser->mTexture->getSize().x / tilesize.x);

            if (version == MechanismVersion::Version2)
            {
               laser->mTu = 0;
            }

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


void Laser::addTilesVersion1()
{
   // each tile is split up into 3 rows x 3 columns (3 x 8 pixels)
   mTilesVersion1.push_back(
      {0,0,0,
       1,1,1,
       0,0,0}
   );

   mTilesVersion1.push_back({0,1,0,0,1,0,0,1,0});
   mTilesVersion1.push_back({0,1,0,0,1,0,0,1,0});
   mTilesVersion1.push_back({0,1,0,0,1,0,0,1,0});
   mTilesVersion1.push_back({0,0,0,1,1,1,0,0,0});
   mTilesVersion1.push_back({0,0,0,1,1,1,0,0,0});
   mTilesVersion1.push_back({0,1,0,1,1,1,0,1,0});
   mTilesVersion1.push_back({0,0,0,1,1,1,0,1,0});
   mTilesVersion1.push_back({0,1,0,1,1,0,0,1,0});
   mTilesVersion1.push_back({0,1,0,1,1,1,0,0,0});
   mTilesVersion1.push_back({0,1,0,0,1,1,0,1,0});
   mTilesVersion1.push_back({0,0,0,0,1,1,0,1,0});
   mTilesVersion1.push_back({0,0,0,1,1,0,0,1,0});
   mTilesVersion1.push_back({0,1,0,0,1,1,0,0,0});
   mTilesVersion1.push_back({0,1,0,1,1,0,0,0,0});
   mTilesVersion1.push_back({0,1,0,0,1,0,0,0,0});
   mTilesVersion1.push_back({0,0,0,0,1,0,0,1,0});
   mTilesVersion1.push_back({0,0,0,0,1,1,0,0,0});
   mTilesVersion1.push_back({0,0,0,1,1,0,0,0,0});
}


void Laser::addTilesVersion2()
{
   mTilesVersion2.push_back({0,1,0,0,1,0,0,0,0});
   mTilesVersion2.push_back({0,0,0,0,1,0,0,1,0});
   mTilesVersion2.push_back({0,0,0,0,1,1,0,0,0});
   mTilesVersion2.push_back({0,0,0,1,1,0,0,0,0});
   mTilesVersion2.push_back({0,1,0,0,1,0,0,1,0});
   mTilesVersion2.push_back({0,0,0,1,1,1,0,0,0});
   mTilesVersion2.push_back({0,0,0,0,1,1,0,1,0});
   mTilesVersion2.push_back({0,0,0,1,1,0,0,1,0});
   mTilesVersion2.push_back({0,1,0,0,1,1,0,0,0});
   mTilesVersion2.push_back({0,1,0,1,1,0,0,0,0});
   mTilesVersion2.push_back({0,1,0,0,1,0,0,0,0});
   mTilesVersion2.push_back({0,0,0,0,1,0,0,1,0});
   mTilesVersion2.push_back({0,0,0,0,1,1,0,0,0});
   mTilesVersion2.push_back({0,0,0,1,1,0,0,0,0});
}


void Laser::collide(const sf::Rect<int32_t>& playerRect)
{
   const auto it =
      std::find_if(std::begin(mLasers), std::end(mLasers), [playerRect](auto laser) {

            const auto roughIntersection = playerRect.intersects(laser->mPixelRect);

            auto active = false;

            if (laser->mVersion == MechanismVersion::Version1)
            {
               active = (laser->mTileIndex == 0);
            }
            else if (laser->mVersion == MechanismVersion::Version2)
            {
               active = (laser->mTileIndex >= rangeEnabled.first) && (laser->mTileIndex <= rangeEnabled.second);
            }

            // tileindex at 0 is an active laser
            if (active && roughIntersection)
            {
               const auto tileId = static_cast<uint32_t>(laser->mTv);

               const auto tile =
                  (laser->mVersion == MechanismVersion::Version1)
                     ? mTilesVersion1[tileId]
                     : mTilesVersion2[tileId];

               auto x = 0u;
               auto y = 0u;

               for (auto i = 0u; i < 9; i++)
               {
                  if (tile[i] == 1)
                  {
                     sf::Rect<int32_t> rect;

                     rect.left = static_cast<int32_t>(laser->mPixelPosition.x + (x * PIXELS_PER_PHYSICS_TILE));
                     rect.top  = static_cast<int32_t>(laser->mPixelPosition.y + (y * PIXELS_PER_PHYSICS_TILE));

                     rect.width  = PIXELS_PER_PHYSICS_TILE;
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
      Player::getCurrent()->damage(100);
   }
}


void Laser::merge()
{
   int32_t groupId = 0;

   for (auto object : mObjects)
   {
      const auto x = static_cast<int32_t>(object->mX      / PIXELS_PER_TILE );
      const auto y = static_cast<int32_t>(object->mY      / PIXELS_PER_TILE);
      const auto w = static_cast<int32_t>(object->mWidth  / PIXELS_PER_TILE );
      const auto h = static_cast<int32_t>(object->mHeight / PIXELS_PER_TILE);

      const auto animationOffset = std::rand() % 100;

      groupId++;

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

                  laser->mAnimationOffset = animationOffset;
                  laser->mGroupId = groupId;
               }
            }
         }
      }
   }
}


