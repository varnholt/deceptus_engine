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
      sf::Sprite _sprite;
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

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;
   void update(const sf::Time& dt) override;

   void left() override;
   void right() override;
   void up() override;
   void down() override;

   void show() override;
   void hide() override;

   GameControllerInfo getJoystickInfo() const;
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

   void clampIndex();

   void keyboardKeyPressed(sf::Keyboard::Key key);

private:
   void loadInventoryItems();
   Inventory& getInventory();

   std::shared_ptr<Layer> getFilterLayer(Filter filter) const;
   void selectNextFilter();
   void selectPreviousFilter();
   void updateFilterLayers();
   void updateShowHide();
   void updateMove();
   void updateButtons();
   void updateInventoryItems();

   void resetIndex();
   void updateFrame();
   void drawInventoryItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

   std::optional<std::string> getSelectedItem() const;
   void assign(const std::string& item, int32_t slot);

   std::unordered_map<Filter, std::shared_ptr<Layer>> _filter_map;
   std::array<Filter, 5> _filters;

   sf::Sprite _cursor_sprite;
   sf::Vector2f _cursor_position;
   std::shared_ptr<sf::Texture> _inventory_texture;
   std::unique_ptr<LayerData> _frame_selection;
   std::unique_ptr<LayerData> _frame_slot1;
   std::unique_ptr<LayerData> _frame_slot2;

   std::map<std::string, ItemSprite> _sprites;
   int32_t _selected_index = 0;
   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   std::vector<LayerData> _panel_header;
   std::vector<LayerData> _panel_left;
   std::vector<LayerData> _panel_center;
   std::vector<LayerData> _panel_right;
   std::vector<LayerData> _panel_background;
   std::vector<LayerData> _frames;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;

   sf::Vector2f _panel_left_offset_px;
   sf::Vector2f _panel_center_offset_px;
   sf::Vector2f _panel_right_offset_px;

   using EventCallback = std::function<void(const sf::Event&)>;
   EventCallback _keyboard_event_handler;
};

#endif  // INGAMEMENUINVENTORY_H
