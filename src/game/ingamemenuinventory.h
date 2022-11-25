#ifndef INGAMEMENUINVENTORY_H
#define INGAMEMENUINVENTORY_H

#include "constants.h"
#include "framework/image/layer.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "inventory.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <memory>
#include <vector>

class InGameMenuInventory
{
public:
   using HighResTimePoint = std::chrono::high_resolution_clock::time_point;
   using FloatSeconds = std::chrono::duration<float>;

   struct ItemSprite
   {
      sf::Sprite mSprite;
   };

   struct LayerData
   {
      LayerData(const std::shared_ptr<Layer>& layer)
          : _layer(layer), _pos(layer->_sprite->getPosition()), _alpha(layer->_sprite->getColor().a / 255.0f)
      {
      }

      std::shared_ptr<Layer> _layer;
      sf::Vector2f _pos;
      float _alpha{1.0f};
   };

   enum class Filter
   {
      All = 0x0f,
      Weapons = 0x01,
      Consumables = 0x02,
      Items = 0x04,
      Various = 0x08
   };

   InGameMenuInventory();

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

   std::shared_ptr<Layer> getFilterLayer(Filter filter) const;
   void selectNextFilter();
   void selectPreviousFilter();
   void updateFilterLayers();
   void updateAnimation();
   void updateButtons();

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
   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   // animation
   HighResTimePoint _time_show;
   HighResTimePoint _time_hide;
   bool _show_requested = false;
   bool _hide_requested = false;

   std::vector<LayerData> _top_area;
   std::vector<LayerData> _profile_panel;
   std::vector<LayerData> _inventory_panel;
   std::vector<LayerData> _item_description_panel;
};

#endif  // INGAMEMENUINVENTORY_H
