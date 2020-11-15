#include "portal.h"

// game
#include "constants.h"
#include "player/player.h"
#include "fixturenode.h"
#include "math/sfmlmath.h"

#include "tmxparser/tmximage.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxobject.h"
#include "tmxparser/tmxpolyline.h"
#include "tmxparser/tmxproperty.h"
#include "tmxparser/tmxproperties.h"
#include "tmxparser/tmxtileset.h"

#include <iostream>


//-----------------------------------------------------------------------------
Portal::Portal(GameNode* parent)
 : GameNode(parent)
{
   setName(typeid(Portal).name());
}


//-----------------------------------------------------------------------------
void Portal::draw(sf::RenderTarget& window)
{
   for (const auto& sprite : mSprites)
   {
      window.draw(sprite);
   }
}


//-----------------------------------------------------------------------------
sf::Vector2f Portal::getPortalPosition()
{
   sf::Vector2f portalPos = mSprites.at(mSprites.size()-1).getPosition();
   return portalPos;
}



//-----------------------------------------------------------------------------
const sf::Vector2f& Portal::getTilePosition() const
{
   return mTilePosition;
}


//-----------------------------------------------------------------------------
std::shared_ptr<Portal> Portal::getDestination() const
{
   return mDestination;
}


//-----------------------------------------------------------------------------
void Portal::setDestination(const std::shared_ptr<Portal>& dst)
{
   mDestination = dst;
}


//-----------------------------------------------------------------------------
void Portal::update(const sf::Time& /*dt*/)
{
   sf::Vector2f playerPos = Player::getCurrent()->getPixelPositionf();
   sf::Vector2f PortalPos = getPortalPosition();

   sf::Vector2f a(playerPos.x, playerPos.y);
   sf::Vector2f b(PortalPos.x + PIXELS_PER_TILE * 0.5f, PortalPos.y);

   float distance = SfmlMath::length(a - b);
   bool atPortal = (distance < PIXELS_PER_TILE * 1.0f);

   setPlayerAtPortal(atPortal);

   int i = 0;
   for (auto& sprite : mSprites)
   {
      sprite.setColor(
         sf::Color(
            255,
            255, // atPortal ? 150 : 255,
            255  // atPortal ? 150 : 255
         )
      );

      int x = static_cast<int>(mTilePosition.x);
      int y = static_cast<int>(mTilePosition.y);

      sprite.setPosition(
         sf::Vector2f(
            static_cast<float>(x * PIXELS_PER_TILE),
            static_cast<float>((i + y) * PIXELS_PER_TILE)
         )
      );

      i++;
   }
}


//-----------------------------------------------------------------------------
void Portal::link(
   std::vector<std::shared_ptr<GameMechanism>>& portals,
   TmxObject* tmxObject
)
{
   auto srcdst = tmxObject->mPolyLine->mPolyLine;

   sf::Vector2f srcf = srcdst.at(0);
   sf::Vector2f dstf = srcdst.at(1);
   sf::Vector2i src(static_cast<int32_t>(floor(srcf.x)), static_cast<int32_t>(floor(srcf.y)));
   sf::Vector2i dst(static_cast<int32_t>(floor(dstf.x)), static_cast<int32_t>(floor(dstf.y)));

   const auto srcX = static_cast<int32_t>(src.x + tmxObject->mX) / PIXELS_PER_TILE;
   const auto srcY = static_cast<int32_t>(src.y + tmxObject->mY) / PIXELS_PER_TILE;
   const auto dstX = static_cast<int32_t>(dst.x + tmxObject->mX) / PIXELS_PER_TILE;
   const auto dstY = static_cast<int32_t>(dst.y + tmxObject->mY) / PIXELS_PER_TILE;

   std::shared_ptr<Portal> srcPortal;
   std::shared_ptr<Portal> dstPortal;

   for (auto p : portals)
   {
      auto portal = std::dynamic_pointer_cast<Portal>(p);
      sf::Vector2f portalPos = portal->getPortalPosition();

      const auto px = static_cast<int32_t>(portalPos.x / PIXELS_PER_TILE);
      const auto py = static_cast<int32_t>(portalPos.y / PIXELS_PER_TILE);

      // todo: go to py..(py + mHeight)
      if (px == srcX && (py == srcY || py + 1 == srcY))
      {
         srcPortal = portal;
      }

      if (px == dstX && (py == dstY || py + 1 == dstY))
      {
         dstPortal = portal;
      }

      if (srcPortal != nullptr && dstPortal != nullptr)
      {
         srcPortal->mDestination = dstPortal;
         break;
      }
   }

   // set the destination's destination to where we came from.
   // not sure if this is desired behavior. but for development purposes
   // it'll help :)
   if (!dstPortal)
   {
      std::cerr << "please mark your dst portal correctly for id: " << tmxObject->mId << std::endl;
   }

   if (!srcPortal)
   {
      std::cerr << "please mark your src portal correctly for id: " << tmxObject->mId << std::endl;
   }

   dstPortal->mDestination = srcPortal;

   // std::cout << "src: " << srcPortal << " dst: " << dstPortal << " (" << tmxObject->mName << ")" << std::endl;
}


//-----------------------------------------------------------------------------
void Portal::addSprite(const sf::Sprite& sprite)
{
   mSprites.push_back(sprite);
}


//-----------------------------------------------------------------------------
bool Portal::isPlayerAtPortal() const
{
   return mPlayerAtPortal;
}



//-----------------------------------------------------------------------------
void Portal::setPlayerAtPortal(bool playerAtPortal)
{
   mPlayerAtPortal = playerAtPortal;
}


//-----------------------------------------------------------------------------
std::vector<std::shared_ptr<GameMechanism>> Portal::load(
   TmxLayer* layer,
   TmxTileSet* tileSet,
   const std::filesystem::path& basePath,
   const std::shared_ptr<b2World>&
)
{
   // std::cout << "load portal layer" << std::endl;

   if (!tileSet)
   {
      return {};
   }

   std::vector<std::shared_ptr<GameMechanism>> portals;

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
            // find matching Portal
            std::shared_ptr<Portal> portal = nullptr;
            for (auto& p : portals)
            {
               auto tmp = std::dynamic_pointer_cast<Portal>(p);
               if (
                     static_cast<uint32_t>(tmp->mTilePosition.x) == i
                  && static_cast<uint32_t>(tmp->mTilePosition.y) + 1 == j )
               {
                  portal = tmp;
                  break;
               }
            }

            if (portal == nullptr)
            {
               portal = std::make_shared<Portal>();
               portals.push_back(portal);
               portal->mTilePosition.x = static_cast<float>(i);
               portal->mTilePosition.y = static_cast<float>(j);
               portal->mTexture.loadFromFile((basePath / tileSet->mImage->mSource).string());

               if (layer->mProperties != nullptr)
               {
                  portal->setZ(layer->mProperties->mMap["z"]->mValueInt);
               }
            }

            portal->mHeight++;

            int tu = (tileNumber - firstId) % (portal->mTexture.getSize().x / tilesize.x);
            int tv = (tileNumber - firstId) / (portal->mTexture.getSize().x / tilesize.x);

            sf::Sprite sprite;
            sprite.setTexture(portal->mTexture);
            sprite.setTextureRect(
               sf::IntRect(
                  tu * PIXELS_PER_TILE,
                  tv * PIXELS_PER_TILE,
                  PIXELS_PER_TILE,
                  PIXELS_PER_TILE
               )
            );

            sprite.setPosition(
               sf::Vector2f(
                  static_cast<float>(i * PIXELS_PER_TILE),
                  static_cast<float>(j * PIXELS_PER_TILE)
               )
            );

            portal->addSprite(sprite);
         }
      }
   }

   return portals;
}


