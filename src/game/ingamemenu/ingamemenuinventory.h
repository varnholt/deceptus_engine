#ifndef INGAMEMENUINVENTORY_H
#define INGAMEMENUINVENTORY_H

#include "framework/joystick/gamecontrollerinfo.h"
#include "game/image/layerdata.h"
#include "game/ingamemenu/ingamemenupage.h"
#include "game/player/inventory.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class InGameMenuInventory : public InGameMenuPage
{
public:
   struct ItemSprite
   {
      std::unique_ptr<sf::Sprite> _sprite;
   };

   enum class Filter
   {
      All = 0x0f,
      Weapons = 0x01,
      Consumables = 0x02,
      Items = 0x04,
      Various = 0x08
   };

   struct ItemText
   {
      std::string _title;
      std::string _description;
      std::string _description_wrapped;
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
   void drawInventoryItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void drawInventoryTexts(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

   std::optional<std::string> getSelectedItem() const;
   void assign(const std::string& item, int32_t slot);
   void assignSelectedItemToSlot(int32_t slot);
   sf::Vector2f getFramePosition(LayerData* layer_data, int32_t index) const;

   std::unordered_map<Filter, std::shared_ptr<Layer>> _filter_map;
   std::array<Filter, 5> _filters;

   std::unique_ptr<sf::Sprite> _cursor_sprite;
   sf::Vector2f _cursor_position;
   std::shared_ptr<sf::Texture> _inventory_texture;
   std::unique_ptr<LayerData> _frame_selection;
   std::unique_ptr<LayerData> _frame_slot_0;
   std::unique_ptr<LayerData> _frame_slot_1;

   std::map<std::string, ItemSprite> _sprites;
   std::map<std::string, ItemText> _texts;
   std::array<ItemSprite, 2> _slot_sprites;

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

   sf::Font _font_title;
   sf::Font _font_description;
   std::unique_ptr<sf::Text> _text_title;
   std::unique_ptr<sf::Text> _text_description;

   std::function<void(void)> _controller_button_x_pressed_callback;
   std::function<void(void)> _controller_button_y_pressed_callback;

   using EventCallback = std::function<void(const sf::Event&)>;
   EventCallback _keyboard_event_handler;
   std::optional<int32_t> _keyboard_event_handler_id;
};

#endif  // INGAMEMENUINVENTORY_H
