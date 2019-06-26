#include "extramanager.h"

#include "audio.h"
#include "constants.h"
#include "extraitem.h"
#include "extrahealth.h"
#include "extratable.h"
#include "player.h"
#include "tilemap.h"
#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxtile.h"
#include "tmxparser/tmxtileset.h"
#include "SFML/Graphics.hpp"



//-----------------------------------------------------------------------------
ExtraManager::ExtraManager()
{
}


//-----------------------------------------------------------------------------
void ExtraManager::load(
   TmxLayer* layer,
   TmxTileSet* tileSet
)
{
   auto tiles = layer->mData;
   auto width = layer->mWidth;
   auto height = layer->mHeight;
   auto firstId = tileSet->mFirstGid;
   auto tileMap = tileSet->mTileMap;
   auto tileIdAnimated = 0;
   auto tileIdStatic = 0;

   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         int tileNumber = tiles[i + j * width];
         if (tileNumber != 0)
         {
            std::shared_ptr<ExtraItem> item = std::make_shared<ExtraItem>();
            item->mSpriteOffset.x = i;
            item->mSpriteOffset.y = j;
            item->mPosition.x = static_cast<float>(i * TILE_WIDTH);
            item->mPosition.y = static_cast<float>(j * TILE_HEIGHT);
            item->mType = static_cast<ExtraItem::Type>(tileNumber - firstId);

            switch (item->mType)
            {
               case ExtraItem::Type::Coin:
                  // printf("extra coin at: %d, %d\n", i, j);
                  break;
               case ExtraItem::Type::Cherry:
                  // printf("extra cherry at: %d, %d\n", i, j);
                  break;
               case ExtraItem::Type::Banana:
                  // printf("extra banana at: %d, %d\n", i, j);
                  break;
               case ExtraItem::Type::Apple:
                  // printf("extra apple at: %d, %d\n", i, j);
                  break;
               default:
                  // printf("extra tile: %d\n", tileNumber - firstId);
                  break;
            }

            auto it = tileMap.find(tileNumber - firstId);
            if (it != tileMap.end())
            {
               TmxTile* tile = it->second;
               if (tile->mAnimation)
               {
                  item->mVertexOffset = tileIdAnimated++;
               }
            }
            else
            {
               item->mVertexOffset = tileIdStatic++;
            }

            mExtras.push_back(item);
         }
      }
   }
}


//-----------------------------------------------------------------------------
void ExtraManager::collide(const sf::Rect<int32_t>& playerRect)
{
   auto it =
      std::find_if(std::begin(mExtras), std::end(mExtras), [playerRect](std::shared_ptr<ExtraItem> item)
      {
         sf::Rect<int32_t> itemRect;
         itemRect.left = static_cast<int32_t>(item->mPosition.x);
         itemRect.top = static_cast<int32_t>(item->mPosition.y + TILE_HEIGHT);
         itemRect.width = TILE_WIDTH;
         itemRect.height = TILE_HEIGHT;

         return item->mActive && playerRect.intersects(itemRect);
      }
   );

   if (it != mExtras.end())
   {
      printf("player hit extra\n");
      (*it)->mActive = false;
      mTilemap->hideTile(
         (*it)->mSpriteOffset.x,
         (*it)->mSpriteOffset.y,
         (*it)->mVertexOffset
      );

      switch ((*it)->mType)
      {
         case ExtraItem::Type::Coin:
            Audio::getInstance()->playSample("coin.wav");
            break;
         case ExtraItem::Type::Cherry:
            Audio::getInstance()->playSample("healthup.wav");
            Player::getPlayer(0)->mExtraTable->mHealth->addHalth(20);
            break;
         case ExtraItem::Type::Banana:
            Audio::getInstance()->playSample("healthup.wav");
            Player::getPlayer(0)->mExtraTable->mHealth->addHalth(10);
            break;
         case ExtraItem::Type::Apple:
            Audio::getInstance()->playSample("powerup");
            break;
         default:
            break;
      }
   }
}
