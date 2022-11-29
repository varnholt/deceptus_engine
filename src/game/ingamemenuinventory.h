#ifndef INGAMEMENUINVENTORY_H
#define INGAMEMENUINVENTORY_H

#include "constants.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "ingamemenupage.h"
#include "inventory.h"
#include "layerdata.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <memory>
#include <vector>

class InGameMenuInventory : public InGameMenuPage
{
public:

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

   InGameMenuInventory();

   void addDemoInventory();

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;
   void update(const sf::Time& dt) override;
   void show() override;
   void hide() override;

   void left();
   void right();

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

private:
   void addItem(int32_t x, int32_t y, ItemType type);
   Inventory& getInventory();

   std::shared_ptr<Layer> getFilterLayer(Filter filter) const;
   void selectNextFilter();
   void selectPreviousFilter();
   void updateFilterLayers();
   void updateShowHide();
   void updateMove();
   void updateButtons();
   void fullyHidden();

   std::unordered_map<Filter, std::shared_ptr<Layer>> _filter_map;
   std::array<Filter, 5> _filters;

   sf::Sprite _cursor_sprite;
   sf::Vector2f _cursor_position;
   std::shared_ptr<sf::Texture> _inventory_texture;

   std::map<ItemType, ItemSprite> _sprites;
   int32_t _selected_item = 0;
   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   std::vector<LayerData> _top_area;
   std::vector<LayerData> _profile_panel;
   std::vector<LayerData> _inventory_panel;
   std::vector<LayerData> _item_description_panel;
};

#endif  // INGAMEMENUINVENTORY_H
