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
            item->_sprite_offset.x = i;
            item->_sprite_offset.y = j;
            item->_position.x = static_cast<float>(i * PIXELS_PER_TILE);
            item->_position.y = static_cast<float>(j * PIXELS_PER_TILE);
            item->_type = static_cast<ExtraItem::ExtraSpriteIndex>(tileNumber - firstId);
            _extras.push_back(item);
         }
      }
   }
}


//-----------------------------------------------------------------------------
void ExtraManager::collide(const sf::Rect<int32_t>& playerRect)
{
   for (auto& extra : _extras)
   {
      if (!extra->_active)
      {
         continue;
      }

      sf::Rect<int32_t> itemRect;
      itemRect.left = static_cast<int32_t>(extra->_position.x);
      itemRect.top = static_cast<int32_t>(extra->_position.y);
      itemRect.width = PIXELS_PER_TILE;
      itemRect.height = PIXELS_PER_TILE;

      if (playerRect.intersects(itemRect))
      {
         extra->_active = false;

         _tilemap->hideTile(
            extra->_sprite_offset.x,
            extra->_sprite_offset.y
         );

         switch (extra->_type)
         {
            case ExtraItem::ExtraSpriteIndex::Coin:
               Audio::getInstance().playSample("coin.wav");
               break;
            case ExtraItem::ExtraSpriteIndex::Cherry:
               Audio::getInstance().playSample("healthup.wav");
               SaveState::getPlayerInfo()._extra_table._health.addHealth(4);
               break;
            case ExtraItem::ExtraSpriteIndex::Banana:
               Audio::getInstance().playSample("healthup.wav");
               SaveState::getPlayerInfo()._extra_table._health.addHealth(1);
               break;
            case ExtraItem::ExtraSpriteIndex::Apple:
               Audio::getInstance().playSample("powerup.wav");
               break;
            case ExtraItem::ExtraSpriteIndex::KeyRed:
            {
               Audio::getInstance().playSample("powerup.wav");
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyRed);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyOrange:
            {
               Audio::getInstance().playSample("powerup.wav");
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyOrange);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyBlue:
            {
               Audio::getInstance().playSample("powerup.wav");
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyBlue);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyGreen:
            {
               Audio::getInstance().playSample("powerup.wav");
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyGreen);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyYellow:
            {
               Audio::getInstance().playSample("powerup.wav");
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyYellow);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::Dash:
            {
               Audio::getInstance().playSample("powerup.wav");
               SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(ExtraSkill::Skill::Dash);
               break;
            }
         }
      }
   }
}


void ExtraManager::resetExtras()
{
   _extras.clear();
}




