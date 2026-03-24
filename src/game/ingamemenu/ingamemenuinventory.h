#ifndef INGAMEMENUINVENTORY_H
#define INGAMEMENUINVENTORY_H

#include "framework/joystick/gamecontrollerinfo.h"
#include "game/image/layerdata.h"
#include "game/ingamemenu/ingamemenupage.h"
#include "game/player/inventory.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/// \brief manages inventory menu rendering, selection, filtering ui, and item slot assignment.
class InGameMenuInventory : public InGameMenuPage
{
public:
   /// \brief stores the sprite used to render one inventory item icon.
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

   /// \brief stores localized text shown for one inventory item.
   struct ItemText
   {
      std::string _title;
      std::string _description;
      std::string _description_wrapped;
   };

   /// \brief loads inventory menu layers, fonts, and cached item presentation data.
   InGameMenuInventory();

   /// \brief draws menu layers and then item icons and text overlays.
   /// \param window render target that receives inventory page rendering.
   /// \param states render states used for drawing.
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;

   /// \brief advances animation state and recalculates item, slot, and text positions.
   /// \param dt elapsed frame time, currently unused by this page.
   void update(const sf::Time& dt) override;

   /// \brief moves the selected inventory index one column to the left.
   void left() override;

   /// \brief moves the selected inventory index one column to the right.
   void right() override;

   /// \brief moves the selected inventory index one row up when possible.
   void up() override;

   /// \brief moves the selected inventory index one row down when possible.
   void down() override;

   /// \brief registers temporary keyboard and controller callbacks, then starts show animation.
   void show() override;

   /// \brief unregisters temporary input callbacks, then starts hide animation.
   void hide() override;

   /// \brief returns the latest cached joystick state used by this page.
   /// \return copy of the current joystick snapshot.
   GameControllerInfo getJoystickInfo() const;

   /// \brief stores joystick state sampled by the menu controller integration.
   /// \param joystickInfo controller state to cache.
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

   /// \brief clamps selected index to the fixed 6x3 inventory grid range.
   void clampIndex();

   /// \brief handles keyboard shortcuts that assign the selected item to quick slots.
   /// \param key pressed keyboard key to interpret.
   void keyboardKeyPressed(sf::Keyboard::Key key);

private:
   /// \brief builds sprites and wrapped descriptions for all known inventory item definitions.
   void loadInventoryItems();

   /// \brief returns the persistent player inventory from save state.
   /// \return mutable reference to the player inventory.
   Inventory& getInventory();

   /// \brief returns the layer used to display one filter label.
   /// \param filter filter whose visual layer is requested.
   /// \return shared layer pointer bound to the filter value.
   std::shared_ptr<Layer> getFilterLayer(Filter filter) const;

   /// \brief rotates filter order so the next filter becomes active.
   void selectNextFilter();

   /// \brief rotates filter order so the previous filter becomes active.
   void selectPreviousFilter();

   /// \brief shows only the active filter layer and hides all others.
   void updateFilterLayers();

   /// \brief animates panel offsets and alpha during show and hide transitions.
   void updateShowHide();

   /// \brief animates horizontal slide transitions when switching submenus.
   void updateMove();

   /// \brief applies static button prompt visibility for the current input presentation.
   void updateButtons();

   /// \brief recalculates icon, slot overlay, frame, and text positions from selection state.
   void updateInventoryItems();

   /// \brief resets the selected inventory grid index to the first slot.
   void resetIndex();

   /// \brief draws item icons in the grid and assigned quick-slot icons.
   /// \param window render target that receives inventory item sprites.
   /// \param states render states used for drawing.
   void drawInventoryItems(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

   /// \brief draws selected item title and wrapped description text.
   /// \param window render target that receives inventory text overlays.
   /// \param states render states used for drawing.
   void drawInventoryTexts(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

   /// \brief returns the currently selected item key when selection points to a valid entry.
   /// \return selected inventory item id, or std::nullopt when selection is out of range.
   std::optional<std::string> getSelectedItem() const;

   /// \brief assigns an item id to one quick-access slot in the player inventory.
   /// \param item inventory item id to bind.
   /// \param slot quick slot index to update.
   void assign(const std::string& item, int32_t slot);

   /// \brief assigns the currently selected item to the requested quick-access slot.
   /// \param slot quick slot index to update.
   void assignSelectedItemToSlot(int32_t slot);

   /// \brief computes grid-aligned frame position including active submenu move offsets.
   /// \param layer_data frame layer baseline position data.
   /// \param index item index within the 6x3 inventory grid.
   /// \return destination position in menu pixel space.
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
