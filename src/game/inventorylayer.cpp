#include "inventorylayer.h"

#include "extramanager.h"
#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "gameconfiguration.h"
#include "gamecontrollerintegration.h"
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
// static const auto quad_width = 38;
// static const auto quad_height = 38;
// static const auto dist = 10.2f;
// static const auto icon_quad_dist = (icon_width - quad_width);
// static const auto y_offset = 300.0f;
// static const auto item_count = 13;
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
GameControllerInfo InventoryLayer::getJoystickInfo() const
{
   return _joystick_info;
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::setJoystickInfo(const GameControllerInfo& joystickInfo)
{
   _joystick_info = joystickInfo;
}

//---------------------------------------------------------------------------------------------------------------------
InventoryLayer::InventoryLayer() : _inventory_texture(TexturePool::getInstance().get("data/game/inventory.png"))
{
   _cursor_sprite.setTexture(*_inventory_texture);
   _cursor_sprite.setTextureRect({0, 512 - 48, 48, 48});
   // addDemoInventory();

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/inventory.psd");

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
   // add layer: profile_panel: 44, 125 (119 x 188)
   // add layer: heart_upgrade_1: 82, 247 (21 x 18)
   // add layer: heart_upgrade_2: 103, 247 (21 x 18)
   // add layer: heart_upgrade_3: 84, 265 (19 x 16)
   // add layer: heart_upgrade_4: 103, 265 (19 x 16)
   // add layer: inventory_panel: 160, 105 (316 x 231)
   // add layer: item_filter_next_0: 387, 124 (19 x 13)
   // add layer: item_filter_next_1: 387, 124 (19 x 13)
   // add layer: item_filter_previous_0: 235, 124 (19 x 13)
   // add layer: item_filter_previous_1: 235, 124 (19 x 13)
   // add layer: scrollbar_body: 453, 148 (8 x 140)
   // add layer: scrollbar_head: 451, 142 (9 x 152)
   // add layer: item_filter_various: 364, 123 (19 x 15)
   // add layer: item_filter_items: 340, 123 (19 x 15)
   // add layer: item_filter_consumables: 317, 123 (18 x 15)
   // add layer: item_filter_weapons: 293, 123 (19 x 15)
   // add layer: item_filter_all: 258, 123 (30 x 15)
   // add layer: item_description_panel: 479, 119 (112 x 198)
   // add layer: navigator: 181, 81 (278 x 17)
   // add layer: next_menu_0: 465, 83 (24 x 13)
   // add layer: next_menu_1: 465, 83 (24 x 13)
   // add layer: previous_menu_0: 151, 83 (24 x 13)
   // add layer: previous_menu_1: 151, 83 (24 x 13)
   // add layer: separator: 18, 57 (611 x 15)

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      //      std::cout << "add layer: " << layer.getName() << ": " << layer.getLeft() << ", " << layer.getTop() << " (" << layer.getWidth()
      //                << " x " << layer.getHeight() << ")" << std::endl;

      // make all layers visible per default, don't trust the PSD :)
      auto tmp = std::make_shared<Layer>();
      tmp->_visible = true;

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layers[layer.getName()] = tmp;
      _layer_stack.push_back(tmp);
   }

   _filter_map[Filter::Weapons] = _layers["item_filter_weapons"];
   _filter_map[Filter::Consumables] = _layers["item_filter_consumables"];
   _filter_map[Filter::Items] = _layers["item_filter_items"];
   _filter_map[Filter::Various] = _layers["item_filter_various"];
   _filter_map[Filter::All] = _layers["item_filter_all"];

   _filters = {Filter::All, Filter::Weapons, Filter::Consumables, Filter::Items, Filter::Various};

   updateFilterLayers();

   // store original panel positions
   _layer_profile_panel = _layers["profile_panel"];
   _layer_inventory_panel = _layers["inventory_panel"];
   _layer_item_description_panel = _layers["item_description_panel"];

   _profile_panel_px = _layer_profile_panel->_sprite->getPosition();
   _inventory_panel_px = _layer_inventory_panel->_sprite->getPosition();
   _item_description_panel_px = _layer_item_description_panel->_sprite->getPosition();
}

//---------------------------------------------------------------------------------------------------------------------
Inventory& InventoryLayer::getInventory()
{
   return SaveState::getPlayerInfo()._inventory;
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::addItem(int32_t x, int32_t y, ItemType type)
{
   sf::Sprite sprite;
   sprite.setTexture(*_inventory_texture);
   sprite.setTextureRect({x * icon_width, y * icon_height, icon_width, icon_height});
   _sprites[type].mSprite = sprite;

   getInventory().add(type);
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::addDemoInventory()
{
   addItem(0, 0, ItemType::KeyRed);
   addItem(1, 0, ItemType::KeyOrange);
   addItem(2, 0, ItemType::KeyBlue);
   addItem(3, 0, ItemType::KeyGreen);
   addItem(4, 0, ItemType::KeyYellow);
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window.setView(view);

   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }

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
bool InventoryLayer::isControllerActionSkipped() const
{
   auto skipped = false;
   auto now = GlobalClock::getInstance().getElapsedTimeInS();

   if (now - _joystick_update_time < 0.3f)
   {
      skipped = true;
   }

   return skipped;
}

//---------------------------------------------------------------------------------------------------------------------
std::shared_ptr<Layer> InventoryLayer::getFilterLayer(Filter filter) const
{
   return _filter_map.at(filter);
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::selectNextFilter()
{
   std::rotate(_filters.begin(), _filters.begin() + 1, _filters.end());
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::selectPreviousFilter()
{
   std::rotate(_filters.rbegin(), _filters.rbegin() + 1, _filters.rend());
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::updateFilterLayers()
{
   std::for_each(_filters.begin() + 1, _filters.end(), [this](auto filter) { getFilterLayer(filter)->hide(); });
   getFilterLayer(_filters.front())->show();
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::updateControllerActions()
{
   auto& gci = GameControllerIntegration::getInstance();

   if (!gci.isControllerConnected())
   {
      return;
   }

   const auto axis_values = _joystick_info.getAxisValues();
   const auto axis_left_x = gci.getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   const auto hat_values = _joystick_info.getHatValues().at(0);
   const auto dpad_left_pressed = hat_values & SDL_HAT_LEFT;
   const auto dpad_right_pressed = hat_values & SDL_HAT_RIGHT;
   auto xl = axis_values[axis_left_x] / 32767.0f;

   if (dpad_left_pressed)
   {
      xl = -1.0f;
   }
   else if (dpad_right_pressed)
   {
      xl = 1.0f;
   }

   if (fabs(xl) > 0.3f)
   {
      if (xl < 0.0f)
      {
         if (!isControllerActionSkipped())
         {
            _joystick_update_time = GlobalClock::getInstance().getElapsedTimeInS();
            left();
         }
      }
      else
      {
         if (!isControllerActionSkipped())
         {
            _joystick_update_time = GlobalClock::getInstance().getElapsedTimeInS();
            right();
         }
      }
   }
   else
   {
      _joystick_update_time = 0.0f;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::update(const sf::Time& /*dt*/)
{
   if (!_active)
   {
      return;
   }

   // _cursor_position.x = dist * 0.5f + _selected_item * (quad_width + dist) - 0.5f;
   updateControllerActions();

   const auto now = std::chrono::high_resolution_clock::now();

   const FloatSeconds duration_since_show_s = now - _time_show;
   const FloatSeconds duration_since_hide_s = now - _time_hide;

   constexpr auto duration_show_s = 1.0f;
   constexpr auto duration_hide_s = 1.0f;

   sf::Vector2f inventory_panel_offset_px;

   // animate show event
   if (duration_since_show_s.count() < duration_show_s)
   {
      // move profile_panel in from the left
      // move item description panel in from the right
      // move inventory_panel in from the bottom
      // fade in the top in the meantime

      //                0s                      1s
      // profile_panel: -profile_panel.width .. _profile_panel_x_px + profile_panel.width
      //                -120                 .. 40 + 120 (160)
      //
      // profile_panel:           44, 125 (119 x 188)
      // inventory_panel:        160, 105 (316 x 231)
      // item_description_panel: 479, 119 (112 x 198)

      const auto elapsed_s_normalized = duration_since_show_s.count() / duration_show_s;
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f;
      inventory_panel_offset_px.y = 500 * val;

      // std::cout << elapsed_s_normalized << " " << val << " " << inventory_panel_offset_px.y << std::endl;
   }

   // animate hide event
   if (duration_since_hide_s.count() < duration_hide_s)
   {
   }

   const auto inventory_panel_pos_y_px = _inventory_panel_px.y + inventory_panel_offset_px.y;
   _layer_inventory_panel->_sprite->setPosition(_inventory_panel_px.x, inventory_panel_pos_y_px);
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
void InventoryLayer::left()
{
   if (!_active)
   {
      return;
   }

   if (_selected_item > 0)
   {
      _selected_item--;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::right()
{
   if (!_active)
   {
      return;
   }

   if (_selected_item < static_cast<int32_t>(getInventory().getItems().size()) - 1)
   {
      _selected_item++;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::show()
{
   _active = true;
   _time_show = std::chrono::high_resolution_clock::now();
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::hide()
{
   _time_hide = std::chrono::high_resolution_clock::now();
   _active = false;
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::confirm()
{
   if (!_active)
   {
      return;
   }

   hide();
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::cancel()
{
   if (!_active)
   {
      return;
   }

   hide();
}
