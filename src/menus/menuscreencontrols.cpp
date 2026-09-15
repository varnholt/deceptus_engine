#include "menuscreencontrols.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlcompat.h"
#include "framework/tools/sfmlstring.h"
#include "menu.h"
#include "menuaudio.h"

#include "framework/joystick/gamecontroller.h"
#include "framework/tools/log.h"
#include "game/config/inputconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"

#include <SDL3/SDL.h>

#include <array>
#include <cmath>
#include <string_view>

namespace
{

// rows of the psd title layer the word occupies; the ornament below it is kept where it was drawn
constexpr int32_t title_band_height_px = 40;

// every action is listed at once, so the rows sit tighter than the 26px the other option screens
// space theirs at. all ten of them at this stride fill the key window the psd marked out
constexpr float row_stride_px = 14.0f;

// x offsets of the two columns, measured from the left edge of the key window
constexpr float column_action_offset_px = 12.0f;
constexpr float column_binding_offset_px = 110.0f;

// width the binding column is centered as, wide enough for the longest label it shows
constexpr float column_binding_width_px = 56.0f;

constexpr float screen_width_px = 640.0f;

const sf::Color color_label_disabled{100, 90, 115};

// taken from the artwork they replace
const sf::Color color_device_name{165, 170, 237};
const sf::Color color_prompt{186, 26, 83};
const sf::Color color_caption{127, 171, 253};

// every button the controller artwork can light up, so they can all be cleared before one is shown
constexpr std::array<std::string_view, 16> controller_button_layer_names{
   "button_A",
   "button_B",
   "button_X",
   "button_Y",
   "button_MENU",
   "button_VIEW",
   "button_LEFTSTICK",
   "button_RIGHTSTICK",
   "button_DPAD_UP",
   "button_DPAD_DOWN",
   "button_DPAD_LEFT",
   "button_DPAD_RIGHT",
   "button_LB",
   "button_RB",
   "button_LT",
   "button_RT"
};

bool isReadOnlyControllerAction(KeyPressed action)
{
   return action == KeyPressedUp || action == KeyPressedDown || action == KeyPressedLeft || action == KeyPressedRight ||
          action == KeyPressedLook;
}

std::string_view controllerReadOnlyLabel(KeyPressed action)
{
   if (action == KeyPressedLook)
   {
      return "Right Stick";
   }
   return "Analog / DPad";
}

// the layer lighting up the given button in the controller artwork. the triggers are axes rather
// than buttons, so button_LT and button_RT have no sdl button that reaches them
std::string_view controllerButtonLayerName(int32_t sdl_button)
{
   switch (static_cast<SDL_GamepadButton>(sdl_button))
   {
      case SDL_GAMEPAD_BUTTON_SOUTH:
      {
         return "button_A";
      }
      case SDL_GAMEPAD_BUTTON_EAST:
      {
         return "button_B";
      }
      case SDL_GAMEPAD_BUTTON_WEST:
      {
         return "button_X";
      }
      case SDL_GAMEPAD_BUTTON_NORTH:
      {
         return "button_Y";
      }
      case SDL_GAMEPAD_BUTTON_BACK:
      {
         return "button_VIEW";
      }
      case SDL_GAMEPAD_BUTTON_START:
      {
         return "button_MENU";
      }
      case SDL_GAMEPAD_BUTTON_LEFT_STICK:
      {
         return "button_LEFTSTICK";
      }
      case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
      {
         return "button_RIGHTSTICK";
      }
      case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
      {
         return "button_LB";
      }
      case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
      {
         return "button_RB";
      }
      case SDL_GAMEPAD_BUTTON_DPAD_UP:
      {
         return "button_DPAD_UP";
      }
      case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
      {
         return "button_DPAD_DOWN";
      }
      case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
      {
         return "button_DPAD_LEFT";
      }
      case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
      {
         return "button_DPAD_RIGHT";
      }
      default:
      {
         return {};
      }
   }
}

#ifdef DECEPTUS_VRSFML
sf::Utf8String sfstr(const std::string& text)
{
   return sf::Utf8String(text.c_str());
}
#else
sf::String sfstr(const std::string& text)
{
   return sf::String::fromUtf8(text.begin(), text.end());
}
#endif

}  // namespace

MenuScreenControls::MenuScreenControls()
{
   setFilename("data/menus/controls.psd");

#ifdef DECEPTUS_VRSFML
   _text = std::make_unique<sf::Text>(_font, sf::Text::Data{});
#else
   _text = std::make_unique<sf::Text>(_font);
#endif
   _text->setFont(_font);
   _text->setCharacterSize(12);

   _cursor_highlight.setFillColor(sf::Color{80, 60, 100, 110});

   GameControllerIntegration::getInstance().addDeviceRemovedCallback(
      [this](int32_t removed_joystick_id)
      {
         if (_device_mode != DeviceMode::Controller)
         {
            return;
         }

         if (_device_row_index >= static_cast<int32_t>(_device_entries.size()))
         {
            return;
         }

         if (_device_entries[static_cast<size_t>(_device_row_index)].joystick_id != removed_joystick_id)
         {
            return;
         }

         auto& input_config = InputConfiguration::getInstance();
         input_config.saveControllerBindingsToFile(input_config.getCurrentFilename());
         rebuildDeviceList();
         _device_row_index = 0;
         loadDevice(_device_row_index);
         _assignment_state = AssignmentState::Idle;
         _previous_controller_button_values.clear();
      }
   );
}

void MenuScreenControls::rebuildDeviceList()
{
   _device_entries.clear();

   DeviceEntry keyboard_entry;
   keyboard_entry.display_name = tr("Keyboard");
   _device_entries.push_back(keyboard_entry);

   const auto& gci = GameControllerIntegration::getInstance();
   for (const auto joystick_id : gci.getControllerIds())
   {
      const auto& controller = gci.getController(joystick_id);
      if (!controller)
      {
         continue;
      }

      DeviceEntry controller_entry;
      controller_entry.display_name = controller->getName(joystick_id);
      controller_entry.guid = gci.getControllerGuid(joystick_id);
      controller_entry.joystick_id = joystick_id;
      _device_entries.push_back(controller_entry);
   }

   _device_row_index = std::min(_device_row_index, static_cast<int32_t>(_device_entries.size()) - 1);
   if (_device_row_index < 0)
   {
      _device_row_index = 0;
   }
}

void MenuScreenControls::loadDevice(int32_t index)
{
   const auto& entry = _device_entries[static_cast<size_t>(index)];
   auto& input_config = InputConfiguration::getInstance();

   if (entry.guid.empty())
   {
      input_config.setCurrentFilename(InputConfiguration::keyboardFilename());
      input_config.deserializeFromFile();
      _device_mode = DeviceMode::Keyboard;
   }
   else
   {
      const auto controller_filename = InputConfiguration::controllerFilename(entry.guid);
      input_config.setCurrentFilename(controller_filename);
      input_config.mergeControllerBindingsFromFile(controller_filename);
      _device_mode = DeviceMode::Controller;
   }

   _device_name = entry.display_name;
   _assignment_state = AssignmentState::Idle;
   _previous_controller_button_values.clear();

   if (_device_mode == DeviceMode::Controller)
   {
      const auto& actions = InputConfiguration::actionList();
      _action_row_index = 0;
      for (auto action_index = 0; action_index < static_cast<int32_t>(actions.size()); action_index++)
      {
         if (!isReadOnlyControllerAction(actions[static_cast<size_t>(action_index)]))
         {
            _action_row_index = action_index;
            break;
         }
      }
   }
   else
   {
      _action_row_index = 0;
   }
}

void MenuScreenControls::showEvent()
{
   _device_row_index = 0;
   rebuildDeviceList();
   if (!_device_entries.empty())
   {
      loadDevice(_device_row_index);
   }
}

void MenuScreenControls::loadingFinished()
{
   // for (const auto& layer_entry : _layers)
   // {
   //    Log::Info() << "controls psd layer: " << layer_entry.first;
   // }

   for (const auto& layer_entry : _layers)
   {
      layer_entry.second->_visible = false;
   }

   setTitle("header", "Controls", title_band_height_px);
   setCaption("audio_window-main", "Controls", color_caption);

   // content_mask marks out the area the psd reserved for the action list
   _row_list_rect = _layers["content_mask"]->_sprite->getGlobalBounds();
   const auto& list_rect = _row_list_rect;
   const auto action_count = static_cast<float>(InputConfiguration::actionList().size());

   _row_stride = row_stride_px;
   const auto list_top = list_rect.position.y + std::floor((list_rect.size.y - action_count * _row_stride) / 2.0f);
   _row_label_base_rect = {{list_rect.position.x + column_action_offset_px, list_top}, {list_rect.size.x, _row_stride}};
   _row_binding_base_rect = {{list_rect.position.x + column_binding_offset_px, list_top}, {list_rect.size.x, _row_stride}};

   // the two columns do not fill the window the psd drew, so the highlight follows them rather than
   // it, with the indent of the action column left over on the far side of the binding column
   _cursor_highlight_x = list_rect.position.x;
   _cursor_highlight.setSize({column_action_offset_px + column_binding_offset_px + column_binding_width_px, _row_stride});

   auto make_label = [this]() -> std::unique_ptr<sf::Text>
   {
#ifdef DECEPTUS_VRSFML
      auto text = std::make_unique<sf::Text>(_font, sf::Text::Data{});
#else
      auto text = std::make_unique<sf::Text>(_font);
#endif
      text->setFont(_font);
      text->setCharacterSize(12);
      return text;
   };

   _text_setkey_button = make_label();
   _text_setkey_button->setFillColor(color_label_normal);

   _text_defaults_button = make_label();
   _text_defaults_button->setFillColor(color_label_normal);

   _text_back_button = make_label();
   _text_back_button->setFillColor(color_label_normal);

   _text_device_name = make_label();
   _text_device_name->setFillColor(color_device_name);

   _text_prompt = make_label();

   updateLayers();
}

void MenuScreenControls::updateLayers()
{
   _layers["bg_temp"]->_visible = true;
   _layers["header"]->_visible = true;
   _layers["audio_window-main"]->_visible = true;

   // the whole left pane is about a controller, so with none plugged in there is nothing to draw
   // in it and the action list takes the screen on its own
   const auto controller_connected = isControllerUsed();
   const auto controller_selected = (_device_mode == DeviceMode::Controller);

   _layers["controller"]->_visible = controller_connected;
   _layers["controller_window_0"]->_visible = controller_connected && !controller_selected;
   _layers["controller_window_1"]->_visible = controller_connected && controller_selected;

   for (const auto& button_layer_name : controller_button_layer_names)
   {
      _layers[std::string{button_layer_name}]->_visible = false;
   }

   // the artwork lights up whichever button the action under the cursor is bound to. that is worth
   // showing while the keyboard is the device too, since Y assigns a controller button from there
   const auto& actions = InputConfiguration::actionList();
   if (controller_connected && _action_row_index < static_cast<int32_t>(actions.size()))
   {
      const auto& button_bindings = InputConfiguration::getInstance()._action_to_controller_button;
      const auto button_binding = button_bindings.find(actions[static_cast<size_t>(_action_row_index)]);
      if (button_binding != button_bindings.end())
      {
         const auto button_layer_name = controllerButtonLayerName(button_binding->second);
         if (!button_layer_name.empty())
         {
            _layers[std::string{button_layer_name}]->_visible = true;
         }
      }
   }

   // with the pane gone the list is the only thing left, so it moves to the middle of the screen.
   // the two columns sit in the left part of the window the psd drew, so it is them rather than the
   // window that has to end up centered
   const auto columns_center_x =
      _row_list_rect.position.x + (column_action_offset_px + column_binding_offset_px + column_binding_width_px) / 2.0f;
   _row_x_offset = controller_connected ? 0.0f : std::floor((screen_width_px / 2.0f) - columns_center_x);

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;

   _layers["setKey_xbox_0"]->_visible = isControllerUsed();
   _layers["setKey_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;

   _layers["setKey_pc_0"]->_visible = !isControllerUsed();
   _layers["setKey_pc_1"]->_visible = false;

   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   if (!_text_back_button)
   {
      return;
   }

   const auto& setkey_layer = isControllerUsed() ? _layers["setKey_xbox_0"] : _layers["setKey_pc_0"];
   _text_setkey_button->setString(sftr("Set Key"));
   placeTextRightOf(*_text_setkey_button, setkey_layer->_sprite->getGlobalBounds());

   const auto& defaults_layer = isControllerUsed() ? _layers["defaults_xbox_0"] : _layers["defaults_pc_0"];
   _text_defaults_button->setString(sftr("Defaults"));
   placeTextRightOf(*_text_defaults_button, defaults_layer->_sprite->getGlobalBounds());

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   _text_device_name->setString(sfstr(_device_name));
   placeTextCentered(*_text_device_name, _layers["deviceName_text"]->_sprite->getGlobalBounds());

   if (_assignment_state == AssignmentState::WaitingForKey)
   {
      _text_prompt->setString(sftr("[ Press a key ]"));
      _text_prompt->setFillColor(color_prompt);
   }
   else if (_assignment_state == AssignmentState::WaitingForButton)
   {
      _text_prompt->setString(sftr("[ Press a button ]"));
      _text_prompt->setFillColor(color_prompt);
   }
   else if (_device_entries.size() > 1)
   {
      _text_prompt->setString(sftr("Left / Right: change device"));
      _text_prompt->setFillColor(color_help_text);
   }
   else
   {
      // the keyboard is the only device there is, so there is nothing to change to
      _text_prompt->setString(sftr(""));
   }
   placeTextCentered(*_text_prompt, _layers["[ Press a Key ]"]->_sprite->getGlobalBounds());
}

void MenuScreenControls::up()
{
   const auto& actions = InputConfiguration::actionList();
   auto candidate = _action_row_index - 1;
   while (candidate > 0 && _device_mode == DeviceMode::Controller && isReadOnlyControllerAction(actions[static_cast<size_t>(candidate)]))
   {
      candidate--;
   }
   if (candidate >= 0 && !(_device_mode == DeviceMode::Controller && isReadOnlyControllerAction(actions[static_cast<size_t>(candidate)])))
   {
      _action_row_index = candidate;
      MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
   }
}

void MenuScreenControls::down()
{
   const auto& actions = InputConfiguration::actionList();
   const auto row_count = static_cast<int32_t>(actions.size());
   auto candidate = _action_row_index + 1;
   while (candidate < row_count && _device_mode == DeviceMode::Controller &&
          isReadOnlyControllerAction(actions[static_cast<size_t>(candidate)]))
   {
      candidate++;
   }
   if (candidate < row_count)
   {
      _action_row_index = candidate;
      MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
   }
}

void MenuScreenControls::select()
{
   _pending_action = InputConfiguration::actionList()[static_cast<size_t>(_action_row_index)];
   if (_device_mode == DeviceMode::Controller)
   {
      if (isReadOnlyControllerAction(_pending_action))
      {
         return;
      }
      _assignment_state = AssignmentState::WaitingForButton;
      _previous_controller_button_values.clear();
   }
   else
   {
      _assignment_state = AssignmentState::WaitingForKey;
   }
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenControls::back()
{
   auto& input_config = InputConfiguration::getInstance();
   if (_device_mode == DeviceMode::Controller)
   {
      input_config.saveControllerBindingsToFile(input_config.getCurrentFilename());
   }
   else
   {
      input_config.serializeToFile();
   }
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenControls::resetDefaults()
{
   auto& active_config = InputConfiguration::getInstance();
   const auto& default_config = InputConfiguration::getDefaults();

   if (_device_mode == DeviceMode::Controller)
   {
      active_config._action_to_controller_button = default_config._action_to_controller_button;
      active_config.saveControllerBindingsToFile(active_config.getCurrentFilename());
   }
   else
   {
      active_config._action_to_key = default_config._action_to_key;
      active_config._key_to_action = default_config._key_to_action;
      active_config._action_to_controller_button = default_config._action_to_controller_button;
      active_config.serializeToFile();
   }
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenControls::completeKeyAssignment(sf::Keyboard::Key key)
{
   auto& active_config = InputConfiguration::getInstance();

   // remove any existing binding for this key to avoid duplicates
   const auto existing_action = active_config._key_to_action.find(key);
   if (existing_action != active_config._key_to_action.end())
   {
      active_config._action_to_key.erase(existing_action->second);
      active_config._key_to_action.erase(existing_action);
   }

   // remove previous key bound to this action
   const auto previous_key = active_config._action_to_key.find(_pending_action);
   if (previous_key != active_config._action_to_key.end())
   {
      active_config._key_to_action.erase(previous_key->second);
   }

   active_config._action_to_key[_pending_action] = key;
   active_config._key_to_action[key] = _pending_action;

   _assignment_state = AssignmentState::Idle;
}

void MenuScreenControls::completeButtonAssignment(int32_t sdl_button)
{
   auto& active_config = InputConfiguration::getInstance();

   // remove any existing binding for this button to avoid duplicates
   for (auto map_entry = active_config._action_to_controller_button.begin(); map_entry != active_config._action_to_controller_button.end();)
   {
      if (map_entry->second == sdl_button && map_entry->first != _pending_action)
      {
         map_entry = active_config._action_to_controller_button.erase(map_entry);
      }
      else
      {
         ++map_entry;
      }
   }

   active_config._action_to_controller_button[_pending_action] = sdl_button;

   _assignment_state = AssignmentState::Idle;
   _previous_controller_button_values.clear();
}

std::string MenuScreenControls::bindingName(KeyPressed action) const
{
   const auto& active_config = InputConfiguration::getInstance();

   if (_device_mode == DeviceMode::Controller)
   {
      if (isReadOnlyControllerAction(action))
      {
         return std::string{controllerReadOnlyLabel(action)};
      }

      const auto button_entry = active_config._action_to_controller_button.find(action);
      if (button_entry != active_config._action_to_controller_button.end())
      {
         return InputConfiguration::buttonName(button_entry->second);
      }
      return "--";
   }

   const auto key_entry = active_config._action_to_key.find(action);
   if (key_entry != active_config._action_to_key.end())
   {
      return InputConfiguration::keyName(key_entry->second);
   }
   return "--";
}

void MenuScreenControls::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (_assignment_state == AssignmentState::WaitingForKey)
   {
      if (key == sf::Keyboard::Key::Escape)
      {
         _assignment_state = AssignmentState::Idle;
         return;
      }
      completeKeyAssignment(key);
      return;
   }

   if (_assignment_state == AssignmentState::WaitingForButton)
   {
      if (key == sf::Keyboard::Key::Escape)
      {
         _assignment_state = AssignmentState::Idle;
         _previous_controller_button_values.clear();
      }
      // all other keys are ignored while waiting for a controller button
      return;
   }

   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }
   else if (key == sf::Keyboard::Key::Down)
   {
      down();
   }
   else if (key == sf::Keyboard::Key::Left)
   {
      cycleDevice(-1);
   }
   else if (key == sf::Keyboard::Key::Right)
   {
      cycleDevice(1);
   }
   else if (key == sf::Keyboard::Key::Enter)
   {
      select();
   }
   else if (key == sf::Keyboard::Key::D)
   {
      resetDefaults();
   }
   else if (key == sf::Keyboard::Key::Escape)
   {
      back();
   }
}

void MenuScreenControls::controllerButtonY()
{
   if (_assignment_state != AssignmentState::Idle)
   {
      return;
   }

   const auto row_count = static_cast<int32_t>(InputConfiguration::actionList().size());
   if (_action_row_index >= row_count)
   {
      return;
   }

   _pending_action = InputConfiguration::actionList()[static_cast<size_t>(_action_row_index)];
   if (isReadOnlyControllerAction(_pending_action))
   {
      return;
   }
   _assignment_state = AssignmentState::WaitingForButton;
   _previous_controller_button_values.clear();
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenControls::cycleDevice(int32_t direction)
{
   const auto new_index = std::clamp(_device_row_index + direction, 0, static_cast<int32_t>(_device_entries.size()) - 1);
   if (new_index == _device_row_index)
   {
      return;
   }
   _device_row_index = new_index;
   loadDevice(_device_row_index);
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenControls::update(const sf::Time& dt)
{
   MenuScreen::update(dt);

   if (_assignment_state != AssignmentState::WaitingForButton)
   {
      // keep previous values in sync so WaitingForButton starts from an accurate baseline
      if (GameControllerIntegration::getInstance().isControllerConnected())
      {
         const auto& controller = GameControllerIntegration::getInstance().getController();
         if (controller)
         {
            _previous_controller_button_values = controller->getInfo().getButtonValues();
         }
      }
      return;
   }

   if (!GameControllerIntegration::getInstance().isControllerConnected())
   {
      return;
   }

   const auto& controller = GameControllerIntegration::getInstance().getController();
   if (!controller)
   {
      return;
   }

   const auto& current_button_values = controller->getInfo().getButtonValues();

   if (_previous_controller_button_values.empty())
   {
      _previous_controller_button_values = current_button_values;
      return;
   }

   for (auto button_index = 0u; button_index < current_button_values.size(); button_index++)
   {
      if (button_index >= _previous_controller_button_values.size())
      {
         break;
      }

      const auto previously_pressed = _previous_controller_button_values[button_index];
      const auto currently_pressed = current_button_values[button_index];

      // skip dpad buttons — they are stored as false in the button values vector
      // and are instead handled via hat values; they cannot be captured by polling
      const auto sdl_button = static_cast<SDL_GamepadButton>(button_index);
      if (sdl_button == SDL_GAMEPAD_BUTTON_DPAD_UP || sdl_button == SDL_GAMEPAD_BUTTON_DPAD_DOWN ||
          sdl_button == SDL_GAMEPAD_BUTTON_DPAD_LEFT || sdl_button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
      {
         continue;
      }

      if (!previously_pressed && currently_pressed)
      {
         completeButtonAssignment(static_cast<int32_t>(button_index));
         break;
      }
   }

   _previous_controller_button_values = current_button_values;
}

void MenuScreenControls::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   updateLayers();
   MenuScreen::draw(window, states);

   if (!_text_prompt)
   {
      return;
   }

   const auto& actions = InputConfiguration::actionList();

   for (auto row_index = 0; row_index < static_cast<int32_t>(actions.size()); row_index++)
   {
      const auto action = actions[static_cast<size_t>(row_index)];
      const auto selected = (row_index == _action_row_index);
      const auto read_only = (_device_mode == DeviceMode::Controller && isReadOnlyControllerAction(action));
      auto label_rect = rowRect(_row_label_base_rect, row_index);
      label_rect.position.x += _row_x_offset;

      if (selected && !read_only)
      {
         sfcompat::setPosition(_cursor_highlight, {_cursor_highlight_x + _row_x_offset, label_rect.position.y});
         window.draw(_cursor_highlight, states);
      }

      _text->setFillColor(read_only ? color_label_disabled : (selected ? color_label_selected : color_label_normal));

      _text->setString(sfstr(InputConfiguration::actionDisplayName(action)));
      placeTextLeft(*_text, label_rect);
      window.draw(*_text, states);

      auto binding_rect = rowRect(_row_binding_base_rect, row_index);
      binding_rect.position.x += _row_x_offset;

      _text->setString(sfstr(bindingName(action)));
      placeTextLeft(*_text, binding_rect);
      window.draw(*_text, states);
   }

   if (isControllerUsed())
   {
      window.draw(*_text_device_name, states);
   }
   window.draw(*_text_prompt, states);
   window.draw(*_text_setkey_button, states);
   window.draw(*_text_defaults_button, states);
   window.draw(*_text_back_button, states);
}

/*
data/menus/controls.psd
    bg_temp
    video-window-bg
    video_window-main
    displayMode_arrows
    body
    scrollbar_body
    scrollbar_slider
    vibration_on
    body_header
    header
*/
