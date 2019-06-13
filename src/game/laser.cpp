#include "laser.h"

// game
#include "constants.h"
#include "player.h"
#include "fixturenode.h"
#include "sfmlmath.h"

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
   for (const auto& sprite : mSprites)
   {
      window.draw(sprite);
   }
}


//-----------------------------------------------------------------------------
void Laser::update(float /*dt*/)
{
}


//-----------------------------------------------------------------------------
void Laser::addSprite(const sf::Sprite& sprite)
{
   mSprites.push_back(sprite);
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
            laser->mTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

            if (layer->mProperties != nullptr)
            {
               laser->setZ(layer->mProperties->mMap["z"]->mValueInt);
            }

            const auto tu = (tileNumber - firstId) % (laser->mTexture.getSize().x / tilesize.x);
            const auto tv = (tileNumber - firstId) / (laser->mTexture.getSize().x / tilesize.x);

            sf::Sprite sprite;
            sprite.setTexture(laser->mTexture);
            sprite.setTextureRect(
               sf::IntRect(
                  tu * TILE_WIDTH,
                  tv * TILE_HEIGHT,
                  TILE_WIDTH,
                  TILE_HEIGHT
               )
            );

            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * TILE_WIDTH),
                  static_cast<float>(j * TILE_HEIGHT)
               )
            );

            laser->addSprite(sprite);
         }
      }
   }

   return lasers;
}


