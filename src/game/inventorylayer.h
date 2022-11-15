#pragma once

#include "constants.h"
#include "framework/image/layer.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "inventory.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <memory>
#include <vector>

struct InventoryItem;

class InventoryLayer
{
public:
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

   struct ItemSprite
   {
      sf::Sprite mSprite;
   };

   enum class Filter
   {
      All = 0x0f,
      Weapons = 0x01,
      Consumables = 0x02,
      Items = 0x04,
      Various = 0x08
   };

   InventoryLayer();

   void addDemoInventory();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void update(const sf::Time& dt);

   void left();
   void right();
   void show();
   void hide();
   void confirm();
   void cancel();

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

private:
   void addItem(int32_t x, int32_t y, ItemType type);
   Inventory& getInventory();
   void updateControllerActions();
   bool isControllerActionSkipped() const;

   std::shared_ptr<Layer> getFilterLayer(Filter filter) const;
   void selectNextFilter();
   void selectPreviousFilter();
   void updateFilterLayers();

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;
   std::unordered_map<Filter, std::shared_ptr<Layer>> _filter_map;
   std::array<Filter, 5> _filters;

   sf::Font _font;
   sf::Text _text;

   sf::Sprite _cursor_sprite;
   sf::Vector2f _cursor_position;
   std::shared_ptr<sf::Texture> _inventory_texture;

   std::map<ItemType, ItemSprite> _sprites;
   int32_t _selected_item = 0;
   bool _active = false;
   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   // animation
   HighResTimePoint _time_show;
   HighResTimePoint _time_hide;
   sf::Vector2f _profile_panel_px;
   sf::Vector2f _item_description_panel_px;
   sf::Vector2f _inventory_panel_px;
   std::shared_ptr<Layer> _layer_profile_panel;
   std::shared_ptr<Layer> _layer_inventory_panel;
   std::shared_ptr<Layer> _layer_item_description_panel;
};
