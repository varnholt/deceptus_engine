#include "ingamemenuinventory.h"

#include "displaymode.h"
#include "extramanager.h"
#include "framework/easings/easings.h"
#include "gameconfiguration.h"
#include "gamestate.h"
#include "inventoryitem.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "savestate.h"
#include "texturepool.h"

#include <iostream>

namespace
{
static const auto icon_width = 40;
static const auto icon_height = 24;
}  // namespace

//: _inventory_texture(TexturePool::getInstance().get("data/game/inventory.png"))
// static const auto quad_width = 38;
// static const auto quad_height = 38;
// static const auto dist = 10.2f;
// static const auto icon_quad_dist = (icon_width - quad_width);
// static const auto y_offset = 300.0f;
// static const auto item_count = 13;

//---------------------------------------------------------------------------------------------------------------------
GameControllerInfo InGameMenuInventory::getJoystickInfo() const
{
   return _joystick_info;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::setJoystickInfo(const GameControllerInfo& joystickInfo)
{
   _joystick_info = joystickInfo;
}

//---------------------------------------------------------------------------------------------------------------------
InGameMenuInventory::InGameMenuInventory()
{
   _filename = "data/game/inventory.psd";

   load();

   // ---------------------------------------------------------------
   //               <LT>   MAP   INVENTORY   VAULT   <RT>
   // ---------------------------------------------------------------
   // +-------------+ +----+-----+---+---+---+---+---+ +-------------+
   // |             | |<LB>|#####|all|wpn|con|itm|var| |             |
   // |             | +----+-----+---+---+---+---+---+ |             |
   // |             | |                              | |             |
   // |             | |                              | |    item     |
   // |   profile   | |                              | | description |
   // |    panel    | |       inventory_panel        | |   panel     |
   // |             | |                              | |             |
   // |             | |                              | |             |
   // |             | |                              | |             |
   // +-------------+ +------------------------------+ +-------------+

   // add layer: background: 0, 0 (640 x 360)

   _filter_map[Filter::Weapons] = _layers["item_filter_weapons"];
   _filter_map[Filter::Consumables] = _layers["item_filter_consumables"];
   _filter_map[Filter::Items] = _layers["item_filter_items"];
   _filter_map[Filter::Various] = _layers["item_filter_various"];
   _filter_map[Filter::All] = _layers["item_filter_all"];

   _filters = {Filter::All, Filter::Weapons, Filter::Consumables, Filter::Items, Filter::Various};

   updateFilterLayers();

   _profile_panel = {
      _layers["profile_panel"],
      _layers["heart_upgrade_1"],
      _layers["heart_upgrade_2"],
      _layers["heart_upgrade_3"],
      _layers["heart_upgrade_4"],
   };

   _inventory_panel = {
      _layers["inventory_panel"],
      _layers["item_filter_next_0"],
      _layers["item_filter_next_1"],
      _layers["item_filter_previous_0"],
      _layers["item_filter_previous_1"],
      _layers["item_filter_various"],
      _layers["item_filter_items"],
      _layers["item_filter_consumables"],
      _layers["item_filter_weapons"],
      _layers["item_filter_all"],
      _layers["scrollbar_body"],
      _layers["scrollbar_head"],
   };

   _item_description_panel = {
      _layers["item_description_panel"],
   };

   _top_area = {
      _layers["header"],
      _layers["header_bg"],
      _layers["previous_menu_0"],
      _layers["previous_menu_1"],
      _layers["next_menu_0"],
      _layers["next_menu_1"],
      _layers["footer"],
      _layers["close_pc_0"],
      _layers["close_pc_1"],
      _layers["close_xbox_0"],
      _layers["close_xbox_1"],
      _layers["background"],  // background is faded in/out, too
   };

   // update button visibility
   updateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
Inventory& InGameMenuInventory::getInventory()
{
   return SaveState::getPlayerInfo()._inventory;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::addItem(int32_t x, int32_t y, ItemType type)
{
   sf::Sprite sprite;
   sprite.setTexture(*_inventory_texture);
   sprite.setTextureRect({x * icon_width, y * icon_height, icon_width, icon_height});
   _sprites[type].mSprite = sprite;

   getInventory().add(type);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::addDemoInventory()
{
   addItem(0, 0, ItemType::KeyRed);
   addItem(1, 0, ItemType::KeyOrange);
   addItem(2, 0, ItemType::KeyBlue);
   addItem(3, 0, ItemType::KeyGreen);
   addItem(4, 0, ItemType::KeyYellow);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);
}

//---------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Layer> InGameMenuInventory::getFilterLayer(Filter filter) const
{
   return _filter_map.at(filter);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::selectNextFilter()
{
   std::rotate(_filters.begin(), _filters.begin() + 1, _filters.end());
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::selectPreviousFilter()
{
   std::rotate(_filters.rbegin(), _filters.rbegin() + 1, _filters.rend());
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::updateFilterLayers()
{
   std::for_each(_filters.begin() + 1, _filters.end(), [this](auto filter) { getFilterLayer(filter)->hide(); });
   getFilterLayer(_filters.front())->show();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::fullyHidden()
{
   GameState::getInstance().enqueueResume();
   DisplayMode::getInstance().enqueueUnset(Display::IngameMenu);
   _hide_requested = false;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::updateAnimation()
{
   const auto now = std::chrono::high_resolution_clock::now();

   const FloatSeconds duration_since_show_s = now - _time_show;
   const FloatSeconds duration_since_hide_s = now - _time_hide;

   constexpr auto duration_show_s = 0.5f;
   constexpr auto duration_hide_s = 1.0f;

   sf::Vector2f profile_panel_offset_px;
   sf::Vector2f inventory_panel_offset_px;
   sf::Vector2f item_description_panel_offset_px;

   auto alpha = 1.0f;

   // animate show event
   if (_show_requested && duration_since_show_s.count() < duration_show_s)
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / duration_show_s;
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f;

      profile_panel_offset_px.x = -200 * val;
      inventory_panel_offset_px.y = 250 * val;
      item_description_panel_offset_px.x = 200 * val;

      alpha = Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      profile_panel_offset_px.x = 0;
      inventory_panel_offset_px.y = 0;
      item_description_panel_offset_px.x = 0;
      alpha = 1.0f;

      if (_show_requested)
      {
         _show_requested = false;
      }
   }

   // animate hide event
   if (_hide_requested && duration_since_hide_s.count() < duration_hide_s)
   {
      const auto elapsed_s_normalized = duration_since_hide_s.count() / duration_hide_s;
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f);

      profile_panel_offset_px.x = -200 * val;
      inventory_panel_offset_px.y = 250 * val;
      item_description_panel_offset_px.x = 200 * val;

      alpha = 1.0f - Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      if (_hide_requested)
      {
         fullyHidden();
      }
   }

   // move in x
   for (const auto& layer : _profile_panel)
   {
      const auto x = layer._pos.x + profile_panel_offset_px.x;
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   // move in y
   for (const auto& layer : _inventory_panel)
   {
      const auto y = layer._pos.y + inventory_panel_offset_px.y;
      layer._layer->_sprite->setPosition(layer._pos.x, y);
   }

   // move in x
   for (const auto& layer : _item_description_panel)
   {
      const auto x = layer._pos.x + item_description_panel_offset_px.x;
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   // top
   for (const auto& layer : _top_area)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::updateButtons()
{
   _layers["previous_menu_1"]->_visible = false;
   _layers["next_menu_1"]->_visible = false;
   _layers["item_filter_previous_1"]->_visible = false;
   _layers["item_filter_next_1"]->_visible = false;
   _layers["close_pc_1"]->_visible = false;
   _layers["close_pc_0"]->_visible = false;
   _layers["close_xbox_1"]->_visible = false;
   _layers["close_xbox_0"]->_visible = true;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::update(const sf::Time& /*dt*/)
{
   // _cursor_position.x = dist * 0.5f + _selected_item * (quad_width + dist) - 0.5f;
   updateAnimation();
}

// ---------------------------------------------------------------
//               <LT>   MAP   INVENTORY   VAULT   <RT>
// ---------------------------------------------------------------
// +-------------+ +----+-----+---+---+---+---+---+ +-------------+
// |             | |<LB>|#####|all|wpn|con|itm|var| |             |
// |             | +----+-----+---+---+---+---+---+ |             |
// |             | |                              | |             |
// |             | |                              | |    item     |
// |   profile   | |                              | | description |
// |    panel    | |       inventory_panel        | |   panel     |
// |             | |                              | |             |
// |             | |                              | |             |
// |             | |                              | |             |
// +-------------+ +------------------------------+ +-------------+

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::left()
{
   if (_selected_item > 0)
   {
      _selected_item--;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::right()
{
   if (_selected_item < static_cast<int32_t>(getInventory().getItems().size()) - 1)
   {
      _selected_item++;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::show()
{
   _show_requested = true;
   _time_show = std::chrono::high_resolution_clock::now();
   updateAnimation();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::hide()
{
   if (_hide_requested)
   {
      return;
   }

   _hide_requested = true;
   _time_hide = std::chrono::high_resolution_clock::now();
}

// _cursor_sprite.setTexture(*_inventory_texture);
// _cursor_sprite.setTextureRect({0, 512 - 48, 48, 48});

/*
   const sf::Color color = {50, 70, 100, 150};

    y = y_offset  + 15.0f;
    x = dist;

   for (auto item : SaveState::getPlayerInfo()._inventory.getItems())
   {
      auto visualization = _sprites[item._type];

       visualization.mSprite.setPosition(static_cast<float>(x), static_cast<float>(y));
       window.draw(visualization.mSprite);
       x += icon_width + dist - icon_quad_dist;
    }

   _cursor_position.y = y_offset;
   _cursor_sprite.setPosition(_cursor_position);
   window.draw(_cursor_sprite);
*/
