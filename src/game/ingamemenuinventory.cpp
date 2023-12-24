#include "ingamemenuinventory.h"

#include "framework/easings/easings.h"
#include "game/eventdistributor.h"
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

// inventory
//
// 1 page is 6 x 3
//
// x x x x x x
// x x x x x x
// x x x x x x
//
// left:  index--
// right: index++
// up:    index -= 6
// down:  index += 6

namespace
{
constexpr auto COLUMNS = 6;
constexpr auto ROWS = 3;
constexpr auto icon_width = 38;
constexpr auto icon_height = 38;
constexpr auto frame_width = 44;
constexpr auto frame_height = 52;
}  // namespace

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
      _layers["frame"],
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

   _frames = {
      _layers["frm_red"],
      _layers["frm_blue"],
      _layers["frm_purple"],
      _layers["frm_green"],
   };

   _frame = std::make_unique<LayerData>(_layers["frame"]);

   _panel_background = {
      _layers["background"],
   };

   // frames (no need for the colored ones for now)
   for (auto& frame : _frames)
   {
      frame._layer->hide();
   }

   // update button visibility
   updateButtons();

   MenuConfig config;
   _duration_hide = config._duration_hide;
   _duration_show = config._duration_show;

   loadInventoryItems();

   EventDistributor::registerEvent(sf::Event::KeyPressed, [this](const sf::Event& event) { keyboardKeyPressed(event.key.code); });
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::loadInventoryItems()
{
   const auto images = InventoryImages::readImages();
   const auto& texture = TexturePool::getInstance().get("data/sprites/inventory_items.png");

   std::ranges::for_each(
      images,
      [this, texture](const auto& image)
      {
         sf::Sprite sprite;
         sprite.setTexture(*texture);
         sprite.setTextureRect({image._x_px, image._y_px, icon_width, icon_height});
         _sprites[image._name]._sprite = sprite;
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
   drawInventoryItems(window, states);
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
void InGameMenuInventory::drawInventoryItems(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto& inventory = getInventory();
   for (const auto& item_key : inventory._items)
   {
      window.draw(_sprites[item_key]._sprite, states);
   }
}

//---------------------------------------------------------------------------------------------------------------------
std::string InGameMenuInventory::getSelectedItem() const
{
   std::string item;
   const auto& inventory = SaveState::getPlayerInfo()._inventory;

   if (_selected_index < static_cast<int32_t>(inventory._items.size()))
   {
      // apply filter
      // ...

      item = inventory._items[_selected_index];
   }

   return item;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::assign(const std::string& item, int32_t slot)
{
   auto& inventory = getInventory();
   inventory._slots[slot] = item;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::updateInventoryItems()
{
   const auto& inventory = getInventory();

   int32_t index{0};
   for (const auto& item_key : inventory._items)
   {
      constexpr auto offset_x_px = 190;
      constexpr auto offset_y_px = 126;

      const auto x_px = static_cast<float>(offset_x_px + (index % COLUMNS) * frame_width);
      const auto y_px = static_cast<float>(offset_y_px + (index / COLUMNS) * frame_height);

      _sprites[item_key]._sprite.setPosition(x_px, y_px);
      index++;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::update(const sf::Time& /*dt*/)
{
   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft || _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }

   updateInventoryItems();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::clampIndex()
{
   _selected_index = std::clamp(_selected_index, 0, (ROWS * COLUMNS) - 1);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::left()
{
   _selected_index--;
   clampIndex();
   updateFrame();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::right()
{
   _selected_index++;
   clampIndex();
   updateFrame();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::up()
{
   const auto next_index = _selected_index - COLUMNS;
   if (next_index >= 0)
   {
      _selected_index = next_index;
      updateFrame();
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::down()
{
   const auto next_index = _selected_index + COLUMNS;
   if (next_index <= (ROWS * COLUMNS) - 1)
   {
      _selected_index = next_index;
      updateFrame();
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::resetIndex()
{
   _selected_index = 0;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::updateFrame()
{
   const auto x = _selected_index % COLUMNS;
   const auto y = _selected_index / COLUMNS;
   const auto pos = _frame->_pos + sf::Vector2f{static_cast<float>(x * frame_width), static_cast<float>(y * frame_height)};
   _frame->_layer->_sprite->setPosition(pos);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenuInventory::keyboardKeyPressed(sf::Keyboard::Key key)
{
   int32_t slot = 0;
   if (key == sf::Keyboard::Z)
   {
      slot = 0;
   }
   else if (key == sf::Keyboard::X)
   {
      slot = 1;
   }

   const auto item = getSelectedItem();
   assign(item, slot);
}
