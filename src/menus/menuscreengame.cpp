#include "menuscreengame.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "game/config/gameconfiguration.h"
#include "menu.h"
#include "menuaudio.h"

#include <algorithm>

namespace
{
constexpr std::string_view language_codes[] = {"en", "it", "ja"};
constexpr std::string_view language_display_keys[] = {"English", "Italian", "Japanese"};
constexpr auto language_count = 3;
}  // namespace

MenuScreenGame::MenuScreenGame()
{
   setFilename("data/menus/game.psd");
}

void MenuScreenGame::up()
{
   auto next = static_cast<int32_t>(_selection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenGame::down()
{
   auto next = static_cast<int32_t>(_selection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenGame::select()
{
   MenuAudio::play(MenuAudio::SoundEffect::Apply);
}

void MenuScreenGame::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenGame::set(int32_t x)
{
   auto& config = GameConfiguration::getInstance();
   switch (_selection)
   {
      case Selection::TextSpeed:
      {
         config._text_speed = std::clamp(config._text_speed + x, 0, 4);
         break;
      }
      case Selection::Rumble:
      {
         config._rumble_enabled = !config._rumble_enabled;
         break;
      }
      case Selection::AutomaticPause:
      {
         config._pause_mode = (config._pause_mode == GameConfiguration::PauseMode::AutomaticPause)
                                 ? GameConfiguration::PauseMode::ManualPause
                                 : GameConfiguration::PauseMode::AutomaticPause;
         break;
      }
      case Selection::Language:
      {
         auto current_index = 0;
         for (auto index = 0; index < language_count; index++)
         {
            if (language_codes[index] == config._language)
            {
               current_index = index;
               break;
            }
         }
         current_index = (current_index + x + language_count) % language_count;
         config._language = std::string{language_codes[current_index]};
         break;
      }
      default:
      {
         break;
      }
   }

   GameConfiguration::getInstance().serializeToFile();
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemTick);
}

void MenuScreenGame::keyboardKeyPressed(sf::Keyboard::Key key)
{
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
      set(-1);
   }
   else if (key == sf::Keyboard::Key::Right)
   {
      set(1);
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

void MenuScreenGame::loadingFinished()
{
   _row_label_base_rect = _layers["textSpeed_text_0"]->_sprite->getGlobalBounds();
   _row_help_base_rect = _layers["textSpeed_help"]->_sprite->getGlobalBounds();
   _row_value_base_rect = _layers["textSpeed_1"]->_sprite->getGlobalBounds();
   _row_stride = _layers["rumble_text_0"]->_sprite->getGlobalBounds().position.y - _row_label_base_rect.position.y;

   for (const auto& layer_name :
        {"textSpeed_text_0",
         "textSpeed_text_1",
         "textSpeed_help",
         "textSpeed_1",
         "textSpeed_2",
         "textSpeed_3",
         "textSpeed_4",
         "textSpeed_5",
         "rumble_text_0",
         "rumble_text_1",
         "rumble_help",
         "Off",
         "On",
         "autoPause_text_0",
         "autoPause_text_1",
         "autoPause_help",
         "autoPause_value_no",
         "autoPause_value_yes"})
   {
      _layers[layer_name]->_visible = false;
   }

   auto make_label = [this]() -> std::unique_ptr<sf::Text>
   {
      auto text = std::make_unique<sf::Text>(_font);
      text->setFont(_font);
      text->setCharacterSize(12);
      return text;
   };

   _textspeed_label = make_label();
   _textspeed_help_text = make_label();
   _textspeed_help_text->setFillColor(color_help_text);
   _textspeed_value_text = make_label();
   _textspeed_value_text->setFillColor(sf::Color::White);

   _rumble_label = make_label();
   _rumble_help_text = make_label();
   _rumble_help_text->setFillColor(color_help_text);
   _rumble_value_text = make_label();
   _rumble_value_text->setFillColor(sf::Color::White);

   _autopause_label = make_label();
   _autopause_help_text = make_label();
   _autopause_help_text->setFillColor(color_help_text);
   _autopause_value_text = make_label();
   _autopause_value_text->setFillColor(sf::Color::White);

   _language_label = make_label();
   _language_help_text = make_label();
   _language_help_text->setFillColor(color_help_text);
   _language_value_text = make_label();
   _language_value_text->setFillColor(sf::Color::White);

   _text_back_button = make_label();
   _text_back_button->setFillColor(color_label_normal);
   _text_defaults_button = make_label();
   _text_defaults_button->setFillColor(color_label_normal);

   updateLayers();
}

bool MenuScreenGame::isRumbleEnabled() const
{
   return GameConfiguration::getInstance()._rumble_enabled;
}

void MenuScreenGame::updateLayers()
{
   const auto autopause_selected = _selection == Selection::AutomaticPause;
   const auto textspeed_selected = _selection == Selection::TextSpeed;
   const auto rumble_selected = _selection == Selection::Rumble;
   const auto language_selected = _selection == Selection::Language;

   const auto auto_pause_mode = GameConfiguration::getInstance()._pause_mode;
   const auto text_speed_value = GameConfiguration::getInstance()._text_speed;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["autoPause_highlight"]->_visible = autopause_selected;
   _layers["autoPause_arrows"]->_visible = autopause_selected;

   _layers["textSpeed_highlight"]->_visible = textspeed_selected;
   _layers["textSpeed_arrows"]->_visible = textspeed_selected;

   _layers["rumble_arrows"]->_visible = rumble_selected;
   _layers["rumble_highlight"]->_visible = rumble_selected;

   if (!_textspeed_label)
   {
      return;
   }

   static const std::string text_speed_strings[] = {"Slowest", "Slow", "Normal", "Fast", "Fastest"};

   _textspeed_label->setString(sftr("Text Speed"));
   _textspeed_label->setFillColor(textspeed_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_textspeed_label, rowRect(_row_label_base_rect, 0));

   _textspeed_help_text->setString(sftr("Set how quickly the dialogue messages appear"));

   placeTextCentered(*_textspeed_help_text, _row_help_base_rect);

   _textspeed_value_text->setString(sftr(text_speed_strings[std::clamp(text_speed_value, 0, 4)]));
   placeTextLeft(*_textspeed_value_text, rowRect(_row_value_base_rect, 0));

   _rumble_label->setString(sftr("Rumble"));
   _rumble_label->setFillColor(rumble_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_rumble_label, rowRect(_row_label_base_rect, 1));

   _rumble_help_text->setString(sftr("Toggle Game Controller Vibration"));

   placeTextCentered(*_rumble_help_text, _row_help_base_rect);

   _rumble_value_text->setString(isRumbleEnabled() ? sftr("On") : sftr("Off"));
   placeTextLeft(*_rumble_value_text, rowRect(_row_value_base_rect, 1));

   _autopause_label->setString(sftr("Automatic Pause"));
   _autopause_label->setFillColor(autopause_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_autopause_label, rowRect(_row_label_base_rect, 2));

   _autopause_help_text->setString(sftr("Pause game on focus loss"));

   placeTextCentered(*_autopause_help_text, _row_help_base_rect);

   const auto autopause_on = (auto_pause_mode == GameConfiguration::PauseMode::AutomaticPause);
   _autopause_value_text->setString(autopause_on ? sftr("Yes") : sftr("No"));
   placeTextLeft(*_autopause_value_text, rowRect(_row_value_base_rect, 2));

   _language_label->setString(sftr("Language"));
   _language_label->setFillColor(language_selected ? color_label_selected : color_label_normal);
   placeTextLeft(*_language_label, rowRect(_row_label_base_rect, 3));

   _language_help_text->setString(sftr("Applied after restart"));
   placeTextCentered(*_language_help_text, _row_help_base_rect);

   auto language_display_index = 0;
   for (auto index = 0; index < language_count; index++)
   {
      if (language_codes[index] == GameConfiguration::getInstance()._language)
      {
         language_display_index = index;
         break;
      }
   }
   _language_value_text->setString(sftr(language_display_keys[language_display_index]));
   placeTextLeft(*_language_value_text, rowRect(_row_value_base_rect, 3));

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   const auto& defaults_layer = isControllerUsed() ? _layers["defaults_xbox_0"] : _layers["defaults_pc_0"];
   _text_defaults_button->setString(sftr("Defaults"));
   placeTextRightOf(*_text_defaults_button, defaults_layer->_sprite->getGlobalBounds());
}

void MenuScreenGame::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   if (!_textspeed_label)
   {
      return;
   }

   window.draw(*_textspeed_label, states);
   if (_selection == Selection::TextSpeed)
   {
      window.draw(*_textspeed_help_text, states);
   }
   window.draw(*_textspeed_value_text, states);

   window.draw(*_rumble_label, states);
   if (_selection == Selection::Rumble)
   {
      window.draw(*_rumble_help_text, states);
   }
   window.draw(*_rumble_value_text, states);

   window.draw(*_autopause_label, states);
   if (_selection == Selection::AutomaticPause)
   {
      window.draw(*_autopause_help_text, states);
   }
   window.draw(*_autopause_value_text, states);

   window.draw(*_language_label, states);
   if (_selection == Selection::Language)
   {
      window.draw(*_language_help_text, states);
   }
   window.draw(*_language_value_text, states);

   window.draw(*_text_back_button, states);
   window.draw(*_text_defaults_button, states);
}

/*
data/menus/game.psd
    bg_temp

    game-window-bg
    game_window-main

    header
*/
