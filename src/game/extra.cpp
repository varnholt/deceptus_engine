#include "extra.h"

#include "audio.h"
#include "constants.h"
#include "extratable.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxtile.h"
#include "framework/tmxparser/tmxtileset.h"
#include "framework/tools/log.h"
#include "gamedeserializedata.h"
#include "inventoryitem.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "savestate.h"
#include "tilemap.h"

#include <iostream>
#include "SFML/Graphics.hpp"

//----------------------------------------------------------------------------------------------------------------------
void Extra::load(const std::shared_ptr<TmxLayer>& layer, const std::shared_ptr<TmxTileSet>& tileset)
{
   resetExtras();

   if (!layer)
   {
      Log::Error() << "tmx layer is empty, please fix your level design";
      return;
   }

   if (!tileset)
   {
      Log::Error() << "tmx tileset is empty, please fix your level design";
      return;
   }

   auto tiles = layer->_data;
   auto width = layer->_width_tl;
   auto height = layer->_height_tl;
   auto first_id = tileset->_first_gid;

   for (auto i = 0u; i < width; ++i)
   {
      for (auto j = 0u; j < height; ++j)
      {
         const auto tile_number = tiles[i + j * width];
         if (tile_number != 0)
         {
            std::shared_ptr<ExtraItem> item = std::make_shared<ExtraItem>();
            item->_sprite_offset.x = i;
            item->_sprite_offset.y = j;
            item->_position.x = static_cast<float>(i * PIXELS_PER_TILE);
            item->_position.y = static_cast<float>(j * PIXELS_PER_TILE);
            item->_type = static_cast<ExtraItem::ExtraSpriteIndex>(tile_number - first_id);
            _extra_items.push_back(item);
         }
      }
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::deserialize(GameNode* /*parent*/, const GameDeserializeData& data)
{
   const auto pos_x_px = data._tmx_object->_x_px;
   const auto pos_y_px = data._tmx_object->_y_px;
   const auto width_px = data._tmx_object->_width_px;
   const auto height_px = data._tmx_object->_height_px;

   std::cout << "extra at: " << pos_x_px << ", " << pos_y_px << " (width: " << width_px << ", height: " << height_px << ")" << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::collide(const sf::FloatRect& player_rect)
{
   for (auto& extra : _extra_items)
   {
      if (!extra->_active)
      {
         continue;
      }

      sf::FloatRect item_rect;
      item_rect.left = extra->_position.x;
      item_rect.top = extra->_position.y;
      item_rect.width = PIXELS_PER_TILE;
      item_rect.height = PIXELS_PER_TILE;

      if (player_rect.intersects(item_rect))
      {
         extra->_active = false;

         _tilemap->hideTile(extra->_sprite_offset.x, extra->_sprite_offset.y);

         switch (extra->_type)
         {
            case ExtraItem::ExtraSpriteIndex::Coin:
               Audio::getInstance().playSample({"coin.wav"});
               break;
            case ExtraItem::ExtraSpriteIndex::Cherry:
               Audio::getInstance().playSample({"healthup.wav"});
               SaveState::getPlayerInfo()._extra_table._health.addHealth(4);
               break;
            case ExtraItem::ExtraSpriteIndex::Banana:
               Audio::getInstance().playSample({"healthup.wav"});
               SaveState::getPlayerInfo()._extra_table._health.addHealth(1);
               break;
            case ExtraItem::ExtraSpriteIndex::Apple:
               Audio::getInstance().playSample({"powerup.wav"});
               break;
            case ExtraItem::ExtraSpriteIndex::KeyRed:
            {
               Audio::getInstance().playSample({"powerup.wav"});
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyRed);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyOrange:
            {
               Audio::getInstance().playSample({"powerup.wav"});
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyOrange);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyBlue:
            {
               Audio::getInstance().playSample({"powerup.wav"});
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyBlue);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyGreen:
            {
               Audio::getInstance().playSample({"powerup.wav"});
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyGreen);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::KeyYellow:
            {
               Audio::getInstance().playSample({"powerup.wav"});
               SaveState::getPlayerInfo()._inventory.add(ItemType::KeyYellow);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::Dash:
            {
               Audio::getInstance().playSample({"powerup.wav"});
               SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Dash);
               break;
            }
            case ExtraItem::ExtraSpriteIndex::Invalid:
            {
               break;
            }
         }
      }
   }
}

//----------------------------------------------------------------------------------------------------------------------
void Extra::resetExtras()
{
   _extra_items.clear();
}

Extra::ExtraItem::ExtraItem(GameNode* /*parent*/)
{
   setClassName(typeid(ExtraItem).name());
}
