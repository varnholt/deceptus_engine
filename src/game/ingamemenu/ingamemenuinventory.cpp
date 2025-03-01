#include "ingamemenuinventory.h"

#include "framework/easings/easings.h"
#include "framework/joystick/gamecontroller.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/event/eventdistributor.h"
#include "game/ingamemenu/menuconfig.h"
#include "game/io/texturepool.h"
#include "game/player/playerinfo.h"
#include "game/state/savestate.h"

#include <numbers>

// ---------------------------------------------------------------
//               <LT>   MAP   INVENTORY   VAULT   <RT>
// ---------------------------------------------------------------
// +-------------|   +--+---+---+---+---+---+--+   +-------------+
// |   slot 1    |   |LB|all|wpn|con|itm|var|RB|   |    item     |
// |     (X)     |   +--+---+---+---+---+---+--+   | description |
// +-------------+ +----+----+----+----+----+----+ |    panel    |
// |   slot 2    | |    |    |    |    |    |    | |             |
// |     (Y)     | | 00 | 01 | 02 | 03 | 04 | 05 | |             |
// +-------------+ +----+----+----+----+----+----+ |             |
// |   profile   | |    |    |    |    |    |    | |             |
// |    panel    | | 06 | 07 | 08 | 09 | 10 | 11 | |             |
// |             | +----+----+----+----+----+----+ |             |
// |             | |    |    |    |    |    |    | |             |
// |             | | 12 | 13 | 14 | 15 | 16 | 17 | |             |
// +-------------+ +----+----+----+----+----+----+ +-------------+

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

constexpr auto count_columns = 6;
constexpr auto count_rows = 3;
constexpr auto icon_width = 38;
constexpr auto icon_height = 38;
constexpr auto frame_width = 44;
constexpr auto frame_height = 52;

constexpr auto description_rect_width = 100;
constexpr auto description_rect_height = 135;

constexpr auto inventory_text_font_size = 12;
constexpr auto inventory_title_font_size = 12;

std::string wrapTextWithinRect(const std::string& original_text, const sf::FloatRect& rect, const sf::Font& font, int32_t character_size)
{
   std::string wrapped_text;
   std::string line;
   sf::Text temp_text(font);
   temp_text.setCharacterSize(character_size);

   // get words from original text
   std::vector<std::string> words;
   std::istringstream iss(original_text);
   copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), back_inserter(words));

   for (const auto& word : words)
   {
      // check if the current line exceeds the right boundary
      std::string test_line = line + word + " ";
      temp_text.setString(test_line);

      if (temp_text.getLocalBounds().size.x <= rect.size.x)  // text fits into boundary
      {
         line = test_line;
      }
      else  // boundary is exceeded
      {
         wrapped_text = wrapped_text + line + "\n";
         line = word + " ";
      }
   }

   // add remaining text to the last line
   wrapped_text = wrapped_text + line;
   return wrapped_text;
}

float getHorizontallyCenteredX(const sf::Text& text, const sf::FloatRect& rect)
{
   const auto x_pos = rect.position.x + (rect.size.x - text.getLocalBounds().size.x) * 0.5f;
   return x_pos;
}

}  // namespace

// #define DEBUG_INVENTORY 1

GameControllerInfo InGameMenuInventory::getJoystickInfo() const
{
   return _joystick_info;
}

void InGameMenuInventory::setJoystickInfo(const GameControllerInfo& joystickInfo)
{
   _joystick_info = joystickInfo;
}

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
      _layers["filters"],
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
      _layers["equip"],
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

   _frame_selection = std::make_unique<LayerData>(_layers["frame"]);

   // init slot layers and hide them initially
   _frame_slot_0 = std::make_unique<LayerData>(_layers["frm_silver_0"]);
   _frame_slot_1 = std::make_unique<LayerData>(_layers["frm_silver_1"]);
   _frame_slot_0->_layer->hide();
   _frame_slot_1->_layer->hide();

   _panel_background = {
      _layers["background"],
   };

   // frames (no need for the colored ones for now)
   for (auto& frame : _frames)
   {
      frame._layer->hide();
   }

   // hide unused layers
   _layers["btn_Y"]->hide();
   _layers["btn_X"]->hide();
   _layers["item_desc_oldlever"]->hide();

   // update button visibility
   updateButtons();

   MenuConfig config;
   _duration_hide = config._duration_hide;
   _duration_show = config._duration_show;

   // load fonts
   if (_font_title.openFromFile("data/fonts/deceptum.ttf"))
   {
      const_cast<sf::Texture&>(_font_title.getTexture(inventory_title_font_size)).setSmooth(false);
      _text_title = std::make_unique<sf::Text>(_font_title);
      _text_title->setCharacterSize(inventory_title_font_size);
      _text_title->setFillColor(sf::Color{232, 219, 243});
   }

   if (_font_description.openFromFile("data/fonts/deceptum.ttf"))
   {
      const_cast<sf::Texture&>(_font_description.getTexture(inventory_text_font_size)).setSmooth(false);
      _text_description = std::make_unique<sf::Text>(_font_description);
      _text_description->setCharacterSize(inventory_text_font_size);
      _text_description->setFillColor(sf::Color{232, 219, 243});
   }

   loadInventoryItems();
}

void InGameMenuInventory::loadInventoryItems()
{
   const auto& inventory_item_descriptions = getInventory()._descriptions;
   _inventory_texture = TexturePool::getInstance().get("data/sprites/inventory_items.png");

   std::ranges::for_each(
      inventory_item_descriptions,
      [this](const auto& image)
      {
         // store sprites
         std::unique_ptr<sf::Sprite> sprite = std::make_unique<sf::Sprite>(*_inventory_texture);
         sprite->setTextureRect(sf::IntRect({image._x_px, image._y_px}, {icon_width, icon_height}));
         _sprites[image._name]._sprite = std::move(sprite);

         // store texts
         _texts[image._name]._title = image._title;
         _texts[image._name]._description = image._description;

         // wrap text
         sf::FloatRect rect{{0.0f, 0.0f}, {description_rect_width, description_rect_height}};
         const auto wrapped_text = wrapTextWithinRect(image._description, rect, _font_description, inventory_text_font_size);
         _texts[image._name]._description_wrapped = wrapped_text;
      }
   );
}

Inventory& InGameMenuInventory::getInventory()
{
   return SaveState::getPlayerInfo()._inventory;
}

void InGameMenuInventory::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);
   drawInventoryItems(window, states);
   drawInventoryTexts(window, states);
}

std::shared_ptr<Layer> InGameMenuInventory::getFilterLayer(Filter filter) const
{
   return _filter_map.at(filter);
}

void InGameMenuInventory::selectNextFilter()
{
   std::rotate(_filters.begin(), _filters.begin() + 1, _filters.end());
}

void InGameMenuInventory::selectPreviousFilter()
{
   std::rotate(_filters.rbegin(), _filters.rbegin() + 1, _filters.rend());
}

void InGameMenuInventory::updateFilterLayers()
{
   std::for_each(_filters.begin() + 1, _filters.end(), [this](auto filter) { getFilterLayer(filter)->hide(); });
   getFilterLayer(_filters.front())->show();
}

void InGameMenuInventory::updateShowHide()
{
   const auto now = std::chrono::high_resolution_clock::now();

   const FloatSeconds duration_since_show_s = now - _time_show;
   const FloatSeconds duration_since_hide_s = now - _time_hide;

   auto alpha = 1.0f;

   // animate show event
   if (_animation == Animation::Show && duration_since_show_s.count() < _duration_show.count())
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / _duration_show.count();
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * std::numbers::pi))) * 0.5f;

      _panel_left_offset_px.x = -200 * val;
      _panel_center_offset_px.y = 250 * val;
      _panel_right_offset_px.x = 200 * val;

      alpha = Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      _panel_left_offset_px.x = 0;
      _panel_center_offset_px.y = 0;
      _panel_right_offset_px.x = 0;
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
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * std::numbers::pi))) * 0.5f);

      _panel_left_offset_px.x = -200 * val;
      _panel_center_offset_px.y = 300 * val;
      _panel_right_offset_px.x = 200 * val;

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
      const auto x = layer._pos.x + _panel_left_offset_px.x;
      layer._layer->_sprite->setPosition({x, layer._pos.y});
   }

   // move in y
   for (const auto& layer : _panel_center)
   {
      const auto y = layer._pos.y + _panel_center_offset_px.y;
      layer._layer->_sprite->setPosition({layer._pos.x, y});
   }

   // move in x
   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + _panel_right_offset_px.x;
      layer._layer->_sprite->setPosition({x, layer._pos.y});
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

void InGameMenuInventory::updateMove()
{
   const auto move_offset = getMoveOffset();

   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition({x, layer._pos.y});
   }

   for (const auto& layer : _panel_center)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition({x, layer._pos.y});
   }

   for (const auto& layer : _panel_background)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition({x, layer._pos.y});
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition({x, layer._pos.y});
   }

   if (!move_offset.has_value())
   {
      _animation.reset();
   }
}

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

void InGameMenuInventory::drawInventoryItems(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto& inventory = getInventory();

   for (const auto& item_key : inventory._items)
   {
      window.draw(*_sprites[item_key]._sprite, states);
   }

   int32_t index = 0;
   for (const auto& slot : inventory._slots)
   {
      if (slot.empty())
      {
         index++;
         continue;
      }

      window.draw(*_slot_sprites[index]._sprite, states);
      index++;
   }
}

void InGameMenuInventory::drawInventoryTexts(sf::RenderTarget& window, sf::RenderStates states)
{
   window.draw(*_text_title, states);
   window.draw(*_text_description, states);
}

std::optional<std::string> InGameMenuInventory::getSelectedItem() const
{
   const auto& inventory = SaveState::getPlayerInfo()._inventory;

   if (_selected_index < static_cast<int32_t>(inventory._items.size()))
   {
      // apply filter
      // not implemented yet

      return inventory._items[_selected_index];
   }

   return std::nullopt;
}

void InGameMenuInventory::assign(const std::string& item, int32_t slot)
{
   auto& inventory = getInventory();
   inventory._slots[slot] = item;

   // clear slot if there's a duplicate assignment
   const auto other_slot_index = 1 - slot;
   if (inventory._slots[other_slot_index] == item)
   {
      inventory._slots[other_slot_index].clear();
   }

#ifdef DEBUG_INVENTORY
   std::cout << "assigning " << item << " to slot " << slot << std::endl;
#endif
}

void InGameMenuInventory::assignSelectedItemToSlot(int32_t slot)
{
   const auto item = getSelectedItem();

   if (!item.has_value())
   {
      return;
   }

   assign(item.value(), slot);
}

void InGameMenuInventory::updateInventoryItems()
{
   const auto& inventory = getInventory();
   const auto move_offset = getMoveOffset();

   constexpr auto item_grid_offset_x_px = 190;
   constexpr auto item_grid_offset_y_px = 107;

   std::optional<int32_t> slot_0_index;
   std::optional<int32_t> slot_1_index;

   // update grid of items
   int32_t index{0};
   for (const auto& item_key : inventory._items)
   {
      const auto offset_x_px = item_grid_offset_x_px + move_offset.value_or(0.0f);
      const auto offset_y_px = item_grid_offset_y_px + _panel_center_offset_px.y;

      const auto x_px = static_cast<float>(offset_x_px + (index % count_columns) * frame_width);
      const auto y_px = static_cast<float>(offset_y_px + (index / count_columns) * frame_height);

      _sprites[item_key]._sprite->setPosition({x_px, y_px});

      // also determine the indices for the selected slots
      if (item_key == inventory._slots[0])
      {
         slot_0_index = index;
      }

      if (item_key == inventory._slots[1])
      {
         slot_1_index = index;
      }

      index++;
   }

   // update items in slot
   index = 0;
   for (const auto& slot : inventory._slots)
   {
      if (slot.empty())
      {
         index++;
         continue;
      }

      const auto& reference_sprite = _sprites[slot]._sprite;

      auto& sprite = _slot_sprites[index];
      sprite._sprite->setTextureRect(reference_sprite->getTextureRect());
      sprite._sprite->setTexture(reference_sprite->getTexture());

      constexpr auto frame_width_slots = 47;
      const auto pos_x_px = 61 + _panel_left_offset_px.x + move_offset.value_or(0.0f) + index * frame_width_slots;
      constexpr auto pos_y_px = 110;
      sprite._sprite->setPosition({pos_x_px, pos_y_px});
      index++;
   };

   // update inventory texts
   const auto selected_item = getSelectedItem();
   if (!selected_item.has_value())
   {
      _text_description->setString("");
      _text_title->setString("");
   }
   else
   {
      constexpr auto text_description_x_offset_px = 486;
      constexpr auto text_description_y_offset_px = 128;
      constexpr auto text_title_x_offset_px = 482;
      constexpr auto text_title_y_offset_px = 95;
      constexpr auto text_title_width_px = 115;

      const auto& text = _texts[selected_item.value()];
      const sf::FloatRect rect{{text_title_x_offset_px, 0}, {text_title_width_px, 16}};
      const auto title_x_px = getHorizontallyCenteredX(*_text_title, rect);
      _text_description->setString(text._description_wrapped);
      _text_description->setPosition(
         {_panel_right_offset_px.x + text_description_x_offset_px + move_offset.value_or(0.0f), text_description_y_offset_px}
      );
      _text_title->setString(text._title);
      _text_title->setPosition({_panel_right_offset_px.x + title_x_px + move_offset.value_or(0.0f), text_title_y_offset_px});
   }

   // update frames
   const auto selected_frame_position = getFramePosition(_frame_selection.get(), _selected_index);
   _frame_selection->_layer->_sprite->setPosition(selected_frame_position);

   _frame_slot_0->_layer->_visible = slot_0_index.has_value();
   _frame_slot_1->_layer->_visible = slot_1_index.has_value();
   if (slot_0_index.has_value())
   {
      const auto slot_0_position = getFramePosition(_frame_slot_0.get(), slot_0_index.value());
      _frame_slot_0->_layer->_sprite->setPosition(slot_0_position);
   }

   if (slot_1_index.has_value())
   {
      const auto slot_1_position = getFramePosition(_frame_slot_1.get(), slot_1_index.value());
      _frame_slot_1->_layer->_sprite->setPosition(slot_1_position);
   }
}

void InGameMenuInventory::update(const sf::Time& /*dt*/)
{
   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft ||
            _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }

   updateInventoryItems();
}

void InGameMenuInventory::clampIndex()
{
   _selected_index = std::clamp(_selected_index, 0, (count_rows * count_columns) - 1);
}

void InGameMenuInventory::left()
{
   _selected_index--;
   clampIndex();
}

void InGameMenuInventory::right()
{
   _selected_index++;
   clampIndex();
}

void InGameMenuInventory::up()
{
   const auto next_index = _selected_index - count_columns;
   if (next_index >= 0)
   {
      _selected_index = next_index;
   }
}

void InGameMenuInventory::down()
{
   const auto next_index = _selected_index + count_columns;
   if (next_index <= (count_rows * count_columns) - 1)
   {
      _selected_index = next_index;
   }
}

void InGameMenuInventory::show()
{
#ifdef DEBUG_INVENTORY
   static bool added = false;
   if (!added)
   {
      added = true;
      getInventory().add("sword");
      getInventory().add("potion_red");
      getInventory().add("potion_green");
      getInventory().add("handle");
      getInventory().add("book");
      getInventory().add("key");
   }
#endif

   _keyboard_event_handler = [this](const sf::Event& event)
   {
      if (const auto* key_event = event.getIf<sf::Event::KeyPressed>())
      {
         keyboardKeyPressed(key_event->code);
      }
   };

   EventDistributor::registerEvent(_keyboard_event_handler);

   const auto& gji = GameControllerIntegration::getInstance();
   if (gji.isControllerConnected())
   {
      _controller_button_x_pressed_callback = [this]() { assignSelectedItemToSlot(0); };
      _controller_button_y_pressed_callback = [this]() { assignSelectedItemToSlot(1); };

      gji.getController()->addButtonPressedCallback(SDL_GAMEPAD_BUTTON_WEST, _controller_button_x_pressed_callback);
      gji.getController()->addButtonPressedCallback(SDL_GAMEPAD_BUTTON_NORTH, _controller_button_y_pressed_callback);
   }

   InGameMenuPage::show();
}

void InGameMenuInventory::hide()
{
   EventDistributor::unregisterEvent(_keyboard_event_handler);

   const auto& gji = GameControllerIntegration::getInstance();
   if (gji.isControllerConnected())
   {
      gji.getController()->removeButtonPressedCallback(SDL_GAMEPAD_BUTTON_WEST, _controller_button_x_pressed_callback);
      gji.getController()->removeButtonPressedCallback(SDL_GAMEPAD_BUTTON_NORTH, _controller_button_y_pressed_callback);
   }

   InGameMenuPage::hide();
}

void InGameMenuInventory::resetIndex()
{
   _selected_index = 0;
}

sf::Vector2f InGameMenuInventory::getFramePosition(LayerData* layer_data, int32_t index) const
{
   const auto move_offset = getMoveOffset();
   const auto x = index % count_columns;
   const auto y = index / count_columns;
   const auto pos = layer_data->_pos + sf::Vector2f{
                                          static_cast<float>(x * frame_width + move_offset.value_or(0.0f)),
                                          static_cast<float>(y * frame_height + _panel_center_offset_px.y)
                                       };

   return pos;
}

void InGameMenuInventory::keyboardKeyPressed(sf::Keyboard::Key key)
{
   std::optional<int32_t> slot;

   if (key == sf::Keyboard::Key::LControl)
   {
      slot = 0;
   }
   else if (key == sf::Keyboard::Key::LAlt)
   {
      slot = 1;
   }

   if (!slot.has_value())
   {
      return;
   }

   assignSelectedItemToSlot(slot.value());
}
