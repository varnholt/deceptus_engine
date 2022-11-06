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
static const auto quad_width = 38;
static const auto quad_height = 38;
static const auto dist = 10.2f;
static const auto icon_quad_dist = (icon_width - quad_width);
static const auto y_offset = 300.0f;
static const auto item_count = 13;
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

   // add layer: background
   // add layer: profile_panel
   // add layer: inventory_panel
   // add layer: inventory_next_page_0
   // add layer: inventory_next_page_1
   // add layer: inventory_previous_page_0
   // add layer: inventory_previous_page_1
   // add layer: item_description_panel
   // add layer: page_navigator
   // add layer: R_0
   // add layer: R_1
   // add layer: L_0
   // add layer: L_1
   // add layer: separator

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      // std::cout << "add layer: " << layer.getName() << std::endl;

      auto tmp = std::make_shared<Layer>();
      tmp->_visible = true;  // layer.isVisible();

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
      layer->draw(window, states);
   }

   /*
   const sf::Color color = {50, 70, 100, 150};

   auto x = dist;
   auto y = y_offset + 5.0f;

   for (int i = 0; i < item_count; i++)
   {
      sf::Vertex quad[] =
      {
         sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(y) ), color),
         sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(y) +
   static_cast<float>(quad_height)), color), sf::Vertex(sf::Vector2f(static_cast<float>(x) + static_cast<float>(quad_width),
   static_cast<float>(y) + static_cast<float>(quad_height)), color), sf::Vertex(sf::Vector2f(static_cast<float>(x) +
   static_cast<float>(quad_width), static_cast<float>(y)), color)
      };

      window.draw(quad, 4, sf::Quads);
      x += quad_width + dist;
   }

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
void InventoryLayer::updateControllerActions()
{
   auto& gci = GameControllerIntegration::getInstance();

   if (!gci.isControllerConnected())
   {
      return;
   }

   auto axis_values = _joystick_info.getAxisValues();
   auto axis_left_x = gci.getController()->getAxisIndex(SDL_CONTROLLER_AXIS_LEFTX);
   auto xl = axis_values[axis_left_x] / 32767.0f;
   auto hat_values = _joystick_info.getHatValues().at(0);
   auto dpad_left_pressed = hat_values & SDL_HAT_LEFT;
   auto dpad_right_pressed = hat_values & SDL_HAT_RIGHT;

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
   _cursor_position.x = dist * 0.5f + _selected_item * (quad_width + dist) - 0.5f;
   updateControllerActions();
}

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
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::hide()
{
}

//---------------------------------------------------------------------------------------------------------------------
void InventoryLayer::setActive(bool active)
{
   _active = active;
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
