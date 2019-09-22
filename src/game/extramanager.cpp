#include "extramanager.h"

#include "audio.h"
#include "constants.h"
#include "extraitem.h"
#include "extratable.h"
#include "inventoryitem.h"
#include "player.h"
#include "playerinfo.h"
#include "tilemap.h"
#include "savestate.h"

#include "tmxparser/tmxlayer.h"
#include "tmxparser/tmxtile.h"
#include "tmxparser/tmxtileset.h"

#include "SFML/Graphics.hpp"


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
            item->mPosition.x = static_cast<float>(i * PIXELS_PER_TILE);
            item->mPosition.y = static_cast<float>(j * PIXELS_PER_TILE);
            item->mType = static_cast<ExtraItem::ExtraSpriteIndex>(tileNumber - firstId);

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
   for (auto& extra : mExtras)
   {
      if (!extra->mActive)
      {
         continue;
      }

      sf::Rect<int32_t> itemRect;
      itemRect.left = static_cast<int32_t>(extra->mPosition.x);
      itemRect.top = static_cast<int32_t>(extra->mPosition.y);
      itemRect.width = PIXELS_PER_TILE;
      itemRect.height = PIXELS_PER_TILE;

      if (playerRect.intersects(itemRect))
      {
         // printf("player hit extra\n");
         extra->mActive = false;
         mTilemap->hideTile(
            extra->mSpriteOffset.x,
            extra->mSpriteOffset.y,
            extra->mVertexOffset
         );

         switch (extra->mType)
         {
            case ExtraItem::ExtraSpriteIndex::Coin:
               Audio::getInstance()->playSample("coin.wav");
               break;
            case ExtraItem::ExtraSpriteIndex::Cherry:
               Audio::getInstance()->playSample("healthup.wav");
               SaveState::getPlayerInfo().mExtraTable.mHealth->addHealth(20);
               break;
            case ExtraItem::ExtraSpriteIndex::Banana:
               Audio::getInstance()->playSample("healthup.wav");
               SaveState::getPlayerInfo().mExtraTable.mHealth->addHealth(10);
               break;
            case ExtraItem::ExtraSpriteIndex::Apple:
               Audio::getInstance()->playSample("powerup.wav");
               break;
            case ExtraItem::ExtraSpriteIndex::KeyRed:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mInventory.add(ItemType::KeyRed);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyYellow:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mInventory.add(ItemType::KeyYellow);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyBlue:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mInventory.add(ItemType::KeyBlue);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyGreen:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mInventory.add(ItemType::KeyGreen);
               break;
            }
         }
      }
   }
}


void ExtraManager::resetExtras()
{
   mExtras.clear();
}




