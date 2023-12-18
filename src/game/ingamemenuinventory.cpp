#include "ingamemenuinventory.h"

#include "framework/easings/easings.h"
#include "game/gameconfiguration.h"
#include "game/inventoryimages.h"
#include "game/mechanisms/extra.h"
#include "game/menuconfig.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/savestate.h"
#include "game/texturepool.h"

#include <iostream>
#include <ranges>

namespace
{
constexpr auto icon_width = 38;
constexpr auto icon_height = 38;
}  // namespace

//: _inventory_texture(TexturePool::getInstance().get("data/game/inventory.png"))
// constexpr auto quad_width = 38;
// constexpr auto quad_height = 38;
// constexpr auto dist = 10.2f;
// constexpr auto icon_quad_dist = (icon_width - quad_width);
// constexpr auto y_offset = 300.0f;
// constexpr auto item_count = 13;

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

   _panel_left = {
      _layers["profile_panel"],
      _layers["heart_upgrade_1"],
      _layers["heart_upgrade_2"],
      _layers["heart_upgrade_3"],
      _layers["heart_upgrade_4"],
   };

   _panel_center = {
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

   _panel_right = {
      _layers["item_description_panel"],
   };

   _panel_header = {
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
   };

   _panel_background = {
      _layers["background"],
   };

   // update button visibility
   updateButtons();

   MenuConfig config;
   _duration_hide = config._duration_hide;
   _duration_show = config._duration_show;

   loadInventoryItems();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::loadInventoryItems()
{
   auto images = InventoryImages::readImages();
   auto texture = TexturePool::getInstance().get("data/sprites/inventory_items.png");

   std::ranges::for_each(
      images,
      [this, texture](const auto& image)
      {
         sf::Sprite sprite;
         sprite.setTexture(*texture);
         sprite.setTextureRect({image._x_px * icon_width, image._y_px * icon_height, icon_width, icon_height});
         _sprites[image._name].mSprite = sprite;
      }
   );
}

//---------------------------------------------------------------------------------------------------------------------
Inventory& InGameMenuInventory::getInventory()
{
   return SaveState::getPlayerInfo()._inventory;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);

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
void InGameMenuInventory::updateShowHide()
{
   const auto now = std::chrono::high_resolution_clock::now();

   const FloatSeconds duration_since_show_s = now - _time_show;
   const FloatSeconds duration_since_hide_s = now - _time_hide;

   sf::Vector2f panel_left_offset_px;
   sf::Vector2f panel_center_offset_px;
   sf::Vector2f panel_right_offset_px;

   auto alpha = 1.0f;

   // animate show event
   if (_animation == Animation::Show && duration_since_show_s.count() < _duration_show.count())
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / _duration_show.count();
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f;

      panel_left_offset_px.x = -200 * val;
      panel_center_offset_px.y = 250 * val;
      panel_right_offset_px.x = 200 * val;

      alpha = Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      panel_left_offset_px.x = 0;
      panel_center_offset_px.y = 0;
      panel_right_offset_px.x = 0;
      alpha = 1.0f;

      if (_animation == Animation::Show)
      {
         _animation.reset();
      }
   }

   // animate hide event
   if (_animation == Animation::Hide && duration_since_hide_s.count() < _duration_hide.count())
   {
      const auto elapsed_s_normalized = duration_since_hide_s.count() / _duration_hide.count();
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f);

      panel_left_offset_px.x = -200 * val;
      panel_center_offset_px.y = 250 * val;
      panel_right_offset_px.x = 200 * val;

      alpha = 1.0f - Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      if (_animation == Animation::Hide)
      {
         fullyHidden();
      }
   }

   // move in x
   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + panel_left_offset_px.x;
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   // move in y
   for (const auto& layer : _panel_center)
   {
      const auto y = layer._pos.y + panel_center_offset_px.y;
      layer._layer->_sprite->setPosition(layer._pos.x, y);
   }

   // move in x
   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + panel_right_offset_px.x;
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   // fade in/out
   for (const auto& layer : _panel_header)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }

   for (const auto& layer : _panel_background)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::updateMove()
{
   const auto move_offset = getMoveOffset();

   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_center)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_background)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   if (!move_offset.has_value())
   {
      _animation.reset();
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

   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft || _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }
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

void InGameMenuInventory::up()
{
}

void InGameMenuInventory::down()
{
}
