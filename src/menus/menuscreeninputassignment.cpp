#include "menuscreeninputassignment.h"

#include "menu.h"
#include "menuaudio.h"

#include "framework/joystick/gamecontroller.h"
#include "game/config/inputconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"

#include <SDL3/SDL.h>

namespace
{

constexpr float column_action_x = 72.0f;
constexpr float column_keyboard_x = 240.0f;
constexpr float column_controller_x = 410.0f;
constexpr float row_start_y = 72.0f;
constexpr float row_height = 18.0f;
constexpr float title_y = 22.0f;
constexpr float header_y = 52.0f;
constexpr float status_y = 258.0f;
constexpr float hint_y = 280.0f;
constexpr float hint_2_y = 298.0f;

const sf::Color color_title{220, 200, 255};
const sf::Color color_header{130, 120, 150};
const sf::Color color_row_normal{200, 185, 220};
const sf::Color color_row_selected{255, 255, 255};
const sf::Color color_row_reset_normal{220, 175, 140};
const sf::Color color_row_reset_selected{255, 205, 165};
const sf::Color color_waiting{255, 220, 80};
const sf::Color color_hint{110, 100, 130};

}  // namespace

MenuScreenInputAssignment::MenuScreenInputAssignment()
{
   _font.openFromFile("data/fonts/deceptum.ttf");
   const_cast<sf::Texture&>(_font.getTexture(12)).setSmooth(false);

   _text = std::make_unique<sf::Text>(_font);
   _text->setFont(_font);
   _text->setCharacterSize(12);

   _background.setSize({640.0f, 360.0f});
   _background.setPosition({0.0f, 0.0f});
   _background.setFillColor(sf::Color{10, 5, 20, 230});

   _cursor_highlight.setSize({580.0f, row_height - 1.0f});
   _cursor_highlight.setPosition({30.0f, row_start_y});
   _cursor_highlight.setFillColor(sf::Color{80, 60, 100, 110});
}

void MenuScreenInputAssignment::showEvent()
{
   _assignment_state = AssignmentState::Idle;
   _selected_row_index = 0;
   _previous_controller_button_values.clear();
}

void MenuScreenInputAssignment::up()
{
   if (_selected_row_index > 0)
   {
      _selected_row_index--;
   }
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenInputAssignment::down()
{
   const auto row_count = static_cast<int32_t>(InputConfiguration::actionList().size());
   if (_selected_row_index < row_count)
   {
      _selected_row_index++;
   }
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenInputAssignment::select()
{
   const auto row_count = static_cast<int32_t>(InputConfiguration::actionList().size());
   if (_selected_row_index == row_count)
   {
      resetDefaults();
      return;
   }
   _pending_action = InputConfiguration::actionList()[static_cast<size_t>(_selected_row_index)];
   _assignment_state = AssignmentState::WaitingForKey;
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenInputAssignment::back()
{
   InputConfiguration::getInstance().serializeToFile();
   Menu::getInstance()->show(Menu::MenuType::Controls);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenInputAssignment::resetDefaults()
{
   auto& active_config = InputConfiguration::getInstance();
   const auto& default_config = InputConfiguration::getDefaults();
   active_config._action_to_key = default_config._action_to_key;
   active_config._key_to_action = default_config._key_to_action;
   active_config._action_to_controller_button = default_config._action_to_controller_button;
   active_config.serializeToFile();
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenInputAssignment::completeKeyAssignment(sf::Keyboard::Key key)
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

void MenuScreenInputAssignment::completeButtonAssignment(int32_t sdl_button)
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

void MenuScreenInputAssignment::keyboardKeyPressed(sf::Keyboard::Key key)
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

   // Idle state navigation
   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }
   else if (key == sf::Keyboard::Key::Down)
   {
      down();
   }
   else if (key == sf::Keyboard::Key::Enter)
   {
      select();
   }
   else if (key == sf::Keyboard::Key::Escape)
   {
      back();
   }
}

void MenuScreenInputAssignment::controllerButtonY()
{
   if (_assignment_state != AssignmentState::Idle)
   {
      return;
   }

   const auto row_count = static_cast<int32_t>(InputConfiguration::actionList().size());
   if (_selected_row_index >= row_count)
   {
      return;
   }

   _pending_action = InputConfiguration::actionList()[static_cast<size_t>(_selected_row_index)];
   _assignment_state = AssignmentState::WaitingForButton;
   _previous_controller_button_values.clear();
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenInputAssignment::update(const sf::Time& dt)
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

void MenuScreenInputAssignment::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   window.draw(_background, states);

   const auto& actions = InputConfiguration::actionList();
   const auto row_count = static_cast<int32_t>(actions.size());

   // cursor highlight
   const auto cursor_row_y = row_start_y + static_cast<float>(_selected_row_index) * row_height;
   _cursor_highlight.setPosition({30.0f, cursor_row_y});
   window.draw(_cursor_highlight, states);

   // title
   _text->setCharacterSize(14);
   _text->setString("Input Assignment");
   _text->setFillColor(color_title);
   const auto title_bounds = _text->getLocalBounds();
   _text->setPosition({(640.0f - title_bounds.size.x) / 2.0f, title_y});
   window.draw(*_text, states);

   _text->setCharacterSize(12);

   // column headers
   _text->setFillColor(color_header);
   _text->setString("Action");
   _text->setPosition({column_action_x, header_y});
   window.draw(*_text, states);

   _text->setString("Keyboard");
   _text->setPosition({column_keyboard_x, header_y});
   window.draw(*_text, states);

   _text->setString("Controller");
   _text->setPosition({column_controller_x, header_y});
   window.draw(*_text, states);

   // action rows
   const auto& active_config = InputConfiguration::getInstance();

   for (auto row_index = 0; row_index < row_count; row_index++)
   {
      const auto action = actions[static_cast<size_t>(row_index)];
      const auto row_y = row_start_y + static_cast<float>(row_index) * row_height;
      const auto selected = (row_index == _selected_row_index);
      const auto row_color = selected ? color_row_selected : color_row_normal;

      _text->setFillColor(row_color);

      _text->setString(InputConfiguration::actionDisplayName(action));
      _text->setPosition({column_action_x, row_y});
      window.draw(*_text, states);

      std::string keyboard_name = "--";
      const auto key_entry = active_config._action_to_key.find(action);
      if (key_entry != active_config._action_to_key.end())
      {
         keyboard_name = InputConfiguration::keyName(key_entry->second);
      }
      _text->setString(keyboard_name);
      _text->setPosition({column_keyboard_x, row_y});
      window.draw(*_text, states);

      std::string button_name = "--";
      const auto button_entry = active_config._action_to_controller_button.find(action);
      if (button_entry != active_config._action_to_controller_button.end())
      {
         button_name = InputConfiguration::buttonName(button_entry->second);
      }
      _text->setString(button_name);
      _text->setPosition({column_controller_x, row_y});
      window.draw(*_text, states);
   }

   // Reset Defaults row
   {
      const auto reset_row_y = row_start_y + static_cast<float>(row_count) * row_height;
      const auto reset_selected = (_selected_row_index == row_count);
      _text->setFillColor(reset_selected ? color_row_reset_selected : color_row_reset_normal);
      _text->setString("Reset to Defaults");
      _text->setPosition({column_action_x, reset_row_y});
      window.draw(*_text, states);
   }

   // status text (shown while waiting for input)
   if (_assignment_state == AssignmentState::WaitingForKey)
   {
      _text->setFillColor(color_waiting);
      _text->setString("Press a key to assign  (Esc to cancel)");
      _text->setPosition({column_action_x, status_y});
      window.draw(*_text, states);
   }
   else if (_assignment_state == AssignmentState::WaitingForButton)
   {
      _text->setFillColor(color_waiting);
      _text->setString("Press a face or shoulder button  (Esc to cancel)");
      _text->setPosition({column_action_x, status_y});
      window.draw(*_text, states);
   }

   // hint lines
   _text->setFillColor(color_hint);
   _text->setString("Enter: assign keyboard key    Y button: assign controller button");
   _text->setPosition({column_action_x, hint_y});
   window.draw(*_text, states);

   _text->setString("Esc: save and return");
   _text->setPosition({column_action_x, hint_2_y});
   window.draw(*_text, states);
}
