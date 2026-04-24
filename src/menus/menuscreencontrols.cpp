#include "menuscreencontrols.h"

#include "menu.h"
#include "menuaudio.h"
#include "menuscreeninputassignment.h"

#include "game/config/inputconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"
#include "framework/joystick/gamecontroller.h"

namespace
{

constexpr float row_start_y = 100.0f;
constexpr float row_height = 22.0f;
constexpr float title_y = 30.0f;
constexpr float hint_y = 310.0f;
constexpr float entry_x = 220.0f;

const sf::Color color_title{220, 200, 255};
const sf::Color color_row_normal{200, 185, 220};
const sf::Color color_row_selected{255, 255, 255};
const sf::Color color_hint{110, 100, 130};

}  // namespace

MenuScreenControls::MenuScreenControls()
{
   _font.openFromFile("data/fonts/deceptum.ttf");
   const_cast<sf::Texture&>(_font.getTexture(12)).setSmooth(false);

   _text = std::make_unique<sf::Text>(_font);
   _text->setFont(_font);
   _text->setCharacterSize(12);

   _background.setSize({640.0f, 360.0f});
   _background.setPosition({0.0f, 0.0f});
   _background.setFillColor(sf::Color{10, 5, 20, 230});

   _cursor_highlight.setSize({300.0f, row_height - 1.0f});
   _cursor_highlight.setPosition({entry_x - 10.0f, row_start_y});
   _cursor_highlight.setFillColor(sf::Color{80, 60, 100, 110});
}

void MenuScreenControls::rebuildDeviceList()
{
   _device_entries.clear();

   DeviceEntry keyboard_entry;
   keyboard_entry.display_name = "Keyboard";
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

   _selected_row_index = std::min(_selected_row_index, static_cast<int32_t>(_device_entries.size()) - 1);
   if (_selected_row_index < 0)
   {
      _selected_row_index = 0;
   }
}

void MenuScreenControls::showEvent()
{
   _selected_row_index = 0;
   rebuildDeviceList();
}

void MenuScreenControls::up()
{
   if (_selected_row_index > 0)
   {
      _selected_row_index--;
   }
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenControls::down()
{
   if (_selected_row_index < static_cast<int32_t>(_device_entries.size()) - 1)
   {
      _selected_row_index++;
   }
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenControls::select()
{
   if (_device_entries.empty())
   {
      return;
   }

   const auto& entry = _device_entries[static_cast<size_t>(_selected_row_index)];
   auto& input_config = InputConfiguration::getInstance();

   if (entry.guid.empty())
   {
      // keyboard selected
      input_config.setCurrentFilename(InputConfiguration::keyboardFilename());
      input_config.deserializeFromFile();
      Menu::getInstance()->setInputAssignmentDeviceMode(MenuScreenInputAssignment::DeviceMode::Keyboard, "Keyboard");
   }
   else
   {
      // controller selected — load controller-specific bindings on top of current keyboard bindings
      const auto controller_filename = InputConfiguration::controllerFilename(entry.guid);
      input_config.setCurrentFilename(controller_filename);
      input_config.mergeControllerBindingsFromFile(controller_filename);
      Menu::getInstance()->setInputAssignmentDeviceMode(MenuScreenInputAssignment::DeviceMode::Controller, entry.display_name);
   }

   Menu::getInstance()->show(Menu::MenuType::InputAssignment);
   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenControls::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenControls::keyboardKeyPressed(sf::Keyboard::Key key)
{
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

void MenuScreenControls::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   window.draw(_background, states);

   // cursor highlight
   const auto cursor_row_y = row_start_y + static_cast<float>(_selected_row_index) * row_height;
   _cursor_highlight.setPosition({entry_x - 10.0f, cursor_row_y});
   window.draw(_cursor_highlight, states);

   // title
   _text->setCharacterSize(14);
   _text->setFillColor(color_title);
   _text->setString("Select Input Device");
   const auto title_bounds = _text->getLocalBounds();
   _text->setPosition({(640.0f - title_bounds.size.x) / 2.0f, title_y});
   window.draw(*_text, states);

   _text->setCharacterSize(12);

   // device entries
   for (auto row_index = 0; row_index < static_cast<int32_t>(_device_entries.size()); row_index++)
   {
      const auto row_y = row_start_y + static_cast<float>(row_index) * row_height;
      const auto selected = (row_index == _selected_row_index);

      _text->setFillColor(selected ? color_row_selected : color_row_normal);
      _text->setString(_device_entries[static_cast<size_t>(row_index)].display_name);
      _text->setPosition({entry_x, row_y});
      window.draw(*_text, states);
   }

   // hints
   _text->setFillColor(color_hint);
   _text->setString("Enter: configure bindings    Esc: back");
   _text->setPosition({entry_x - 10.0f, hint_y});
   window.draw(*_text, states);
}

