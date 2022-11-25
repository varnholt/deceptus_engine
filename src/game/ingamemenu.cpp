#include "ingamemenu.h"

#include "displaymode.h"
#include "extramanager.h"
#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "gameconfiguration.h"
#include "gamecontrollerintegration.h"
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
// static const auto quad_width = 38;
// static const auto quad_height = 38;
// static const auto dist = 10.2f;
// static const auto icon_quad_dist = (icon_width - quad_width);
// static const auto y_offset = 300.0f;
// static const auto item_count = 13;
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
GameControllerInfo InGameMenu::getJoystickInfo() const
{
   return _joystick_info;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::setJoystickInfo(const GameControllerInfo& joystickInfo)
{
   _joystick_info = joystickInfo;
}

//---------------------------------------------------------------------------------------------------------------------
InGameMenu::InGameMenu() : _inventory_texture(TexturePool::getInstance().get("data/game/inventory.png"))
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
      tmp->_name = layer.getName();

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
Inventory& InGameMenu::getInventory()
{
   return SaveState::getPlayerInfo()._inventory;
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::addItem(int32_t x, int32_t y, ItemType type)
{
   sf::Sprite sprite;
   sprite.setTexture(*_inventory_texture);
   sprite.setTextureRect({x * icon_width, y * icon_height, icon_width, icon_height});
   _sprites[type].mSprite = sprite;

   getInventory().add(type);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::addDemoInventory()
{
   addItem(0, 0, ItemType::KeyRed);
   addItem(1, 0, ItemType::KeyOrange);
   addItem(2, 0, ItemType::KeyBlue);
   addItem(3, 0, ItemType::KeyGreen);
   addItem(4, 0, ItemType::KeyYellow);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::draw(sf::RenderTarget& window, sf::RenderStates states)
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
bool InGameMenu::isControllerActionSkipped() const
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
std::shared_ptr<Layer> InGameMenu::getFilterLayer(Filter filter) const
{
   return _filter_map.at(filter);
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::selectNextFilter()
{
   std::rotate(_filters.begin(), _filters.begin() + 1, _filters.end());
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::selectPreviousFilter()
{
   std::rotate(_filters.rbegin(), _filters.rbegin() + 1, _filters.rend());
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::updateFilterLayers()
{
   std::for_each(_filters.begin() + 1, _filters.end(), [this](auto filter) { getFilterLayer(filter)->hide(); });
   getFilterLayer(_filters.front())->show();
}

void InGameMenu::updateAnimation()
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
         GameState::getInstance().enqueueResume();
         DisplayMode::getInstance().enqueueUnset(Display::IngameMenu);
         _hide_requested = false;
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
void InGameMenu::updateButtons()
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
void InGameMenu::updateControllerActions()
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
void InGameMenu::update(const sf::Time& /*dt*/)
{
   // _cursor_position.x = dist * 0.5f + _selected_item * (quad_width + dist) - 0.5f;
   updateControllerActions();
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
void InGameMenu::left()
{
   if (_selected_item > 0)
   {
      _selected_item--;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::right()
{
   if (_selected_item < static_cast<int32_t>(getInventory().getItems().size()) - 1)
   {
      _selected_item++;
   }
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::show()
{
   _show_requested = true;
   _time_show = std::chrono::high_resolution_clock::now();
   updateAnimation();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::hide()
{
   if (_hide_requested)
   {
      return;
   }

   _hide_requested = true;
   _time_hide = std::chrono::high_resolution_clock::now();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::confirm()
{
   hide();
}

//---------------------------------------------------------------------------------------------------------------------
void InGameMenu::cancel()
{
   hide();
}
