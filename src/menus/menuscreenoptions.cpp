#include "menuscreenoptions.h"

#include "menu.h"
#include "menuaudio.h"

MenuScreenOptions::MenuScreenOptions()
{
   setFilename("data/menus/options.psd");
}

void MenuScreenOptions::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Up)
   {
      up();
   }

   if (key == sf::Keyboard::Down)
   {
      down();
   }

   if (key == sf::Keyboard::Return)
   {
      select();
   }

   if (key == sf::Keyboard::Escape)
   {
      back();
   }
}

void MenuScreenOptions::back()
{
   const auto& history = Menu::getInstance()->getHistory();

   // choose whatever has been used the last time to open up the options menu
   auto menu = Menu::MenuType::Main;
   for (auto it = history.crbegin(); it != history.crend(); ++it)
   {
      if ((*it) == Menu::MenuType::Main)
      {
         menu = Menu::MenuType::Main;
         break;
      }
      else if ((*it) == Menu::MenuType::Pause)
      {
         menu = Menu::MenuType::Pause;
         break;
      }
   }

   Menu::getInstance()->show(menu);

   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenOptions::loadingFinished()
{
   updateLayers();
}

void MenuScreenOptions::up()
{
   auto next = static_cast<int32_t>(_selection);
   next--;

   // achievements are skipped at the moment
   if (next == static_cast<int32_t>(Selection::Achievements))
   {
      next--;
   }

   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenOptions::down()
{
   auto next = static_cast<int32_t>(_selection);
   next++;

   // achievements are skipped at the moment
   if (next == static_cast<int32_t>(Selection::Achievements))
   {
      next++;
   }

   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   _selection = static_cast<Selection>(next);
   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenOptions::select()
{
   switch (_selection)
   {
      case Selection::Controls:
         Menu::getInstance()->show(Menu::MenuType::Controls);
         break;
      case Selection::Video:
         Menu::getInstance()->show(Menu::MenuType::Video);
         break;
      case Selection::Audio:
         Menu::getInstance()->show(Menu::MenuType::Audio);
         break;
      case Selection::Game:
         Menu::getInstance()->show(Menu::MenuType::Game);
         break;
      case Selection::Achievements:
         Menu::getInstance()->show(Menu::MenuType::Achievements);
         break;
      case Selection::Credits:
         Menu::getInstance()->show(Menu::MenuType::Credits);
         break;
      case Selection::Count:
         break;
   }

   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenOptions::updateLayers()
{
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["accept_xbox_0"]->_visible = isControllerUsed();
   _layers["accept_xbox_1"]->_visible = false;

   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   _layers["accept_pc_0"]->_visible = !isControllerUsed();
   _layers["accept_pc_1"]->_visible = false;

   _layers["credits_0"]->_visible = (_selection != Selection::Credits);
   _layers["credits_1"]->_visible = (_selection == Selection::Credits);
   _layers["achievements_0"]->_visible = (_selection != Selection::Achievements);
   _layers["achievements_1"]->_visible = (_selection == Selection::Achievements);
   _layers["game_0"]->_visible = (_selection != Selection::Game);
   _layers["game_1"]->_visible = (_selection == Selection::Game);
   _layers["audio_0"]->_visible = (_selection != Selection::Audio);
   _layers["audio_1"]->_visible = (_selection == Selection::Audio);
   _layers["video_0"]->_visible = (_selection != Selection::Video);
   _layers["video_1"]->_visible = (_selection == Selection::Video);
   _layers["controls_0"]->_visible = (_selection != Selection::Controls);
   _layers["controls_1"]->_visible = (_selection == Selection::Controls);
}

/*
data/menus/options.psd
    bg_temp
    back_xbox_0
    back_xbox_1
    back_pc_0
    back_pc_1
    accept_xbox_0
    accept_xbox_1
    accept_pc_0
    accept_pc_1
    credits_0
    credits_1
    achievements_0
    achievements_1
    game_0
    game_1
    audio_0
    audio_1
    video_0
    video_1
    controls_0
    controls_1
    header
*/
