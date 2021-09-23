#include "extramanager.h"

#include "audio.h"
#include "constants.h"
#include "extraitem.h"
#include "extratable.h"
#include "inventoryitem.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "tilemap.h"
#include "savestate.h"

#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtile.h"
#include "framework/tmxparser/tmxtileset.h"

#include "SFML/Graphics.hpp"


//-----------------------------------------------------------------------------
void ExtraManager::load(
   TmxLayer* layer,
   TmxTileSet* tileSet
)
{
   resetExtras();

   auto tiles = layer->_data;
   auto width = layer->_width_px;
   auto height = layer->_height_px;
   auto firstId = tileSet->_first_gid;
   auto tileMap = tileSet->_tile_map;

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
            extra->mSpriteOffset.y
         );

         switch (extra->mType)
         {
            case ExtraItem::ExtraSpriteIndex::Coin:
               Audio::getInstance()->playSample("coin.wav");
               break;
            case ExtraItem::ExtraSpriteIndex::Cherry:
               Audio::getInstance()->playSample("healthup.wav");
               SaveState::getPlayerInfo().mExtraTable._health.addHealth(20);
               break;
            case ExtraItem::ExtraSpriteIndex::Banana:
               Audio::getInstance()->playSample("healthup.wav");
               SaveState::getPlayerInfo().mExtraTable._health.addHealth(10);
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
            case ExtraItem::ExtraSpriteIndex::KeyOrange:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mInventory.add(ItemType::KeyOrange);
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
            case ExtraItem::ExtraSpriteIndex::KeyYellow:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mInventory.add(ItemType::KeyYellow);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::Dash:
            {
               Audio::getInstance()->playSample("powerup.wav");
               SaveState::getPlayerInfo().mExtraTable._skills._skills |= ExtraSkill::SkillDash;
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




