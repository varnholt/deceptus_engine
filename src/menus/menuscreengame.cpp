#include "menuscreengame.h"

#include "gameconfiguration.h"
#include "menu.h"
#include "menuaudio.h"

#include <algorithm>

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
      case Selection::AutomaticPause:
      {
         config._pause_mode = static_cast<GameConfiguration::PauseMode>(std::clamp(static_cast<int32_t>(config._pause_mode) + x, 0, 1));
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
   if (key == sf::Keyboard::Up)
   {
      up();
   }
   else if (key == sf::Keyboard::Down)
   {
      down();
   }
   else if (key == sf::Keyboard::Left)
   {
      set(-1);
   }
   else if (key == sf::Keyboard::Right)
   {
      set(1);
   }
   else if (key == sf::Keyboard::Return)
   {
      select();
   }
   else if (key == sf::Keyboard::Escape)
   {
      back();
   }
}

void MenuScreenGame::loadingFinished()
{
   updateLayers();
}

void MenuScreenGame::updateLayers()
{
   auto auto_pause = _selection == Selection::AutomaticPause;
   auto text_speed = _selection == Selection::TextSpeed;

   auto auto_pause_selection = GameConfiguration::getInstance()._pause_mode;
   auto text_speed_selection = GameConfiguration::getInstance()._text_speed;

   _layers["defaults_xbox_0"]->_visible = isControllerUsed();
   _layers["defaults_xbox_1"]->_visible = false;
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["defaults_pc_0"]->_visible = !isControllerUsed();
   _layers["defaults_pc_1"]->_visible = false;
   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["autoPause_text_0"]->_visible = !auto_pause;
   _layers["autoPause_text_1"]->_visible = auto_pause;
   _layers["autoPause_highlight"]->_visible = auto_pause;
   _layers["autoPause_help"]->_visible = auto_pause;
   _layers["autoPause_arrows"]->_visible = auto_pause;
   _layers["autoPause_value_no"]->_visible = (auto_pause_selection == GameConfiguration::PauseMode::ManualPause);
   _layers["autoPause_value_yes"]->_visible = (auto_pause_selection == GameConfiguration::PauseMode::AutomaticPause);

   _layers["textSpeed_text_0"]->_visible = !text_speed;
   _layers["textSpeed_text_1"]->_visible = text_speed;
   _layers["textSpeed_highlight"]->_visible = text_speed;
   _layers["textSpeed_help"]->_visible = text_speed;
   _layers["textSpeed_arrows"]->_visible = text_speed;
   _layers["textSpeed_1"]->_visible = (text_speed_selection == 0);
   _layers["textSpeed_2"]->_visible = (text_speed_selection == 1);
   _layers["textSpeed_3"]->_visible = (text_speed_selection == 2);
   _layers["textSpeed_4"]->_visible = (text_speed_selection == 3);
   _layers["textSpeed_5"]->_visible = (text_speed_selection == 4);
}

/*
data/menus/game.psd
    bg_temp

    game-window-bg
    game_window-main

    header
*/
