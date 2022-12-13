#include "menuscreenmain.h"

#include "game/gamestate.h"
#include "game/messagebox.h"
#include "game/savestate.h"
#include "menu.h"
#include "menuaudio.h"

#define DEV_SAVE_STATE 1

MenuScreenMain::MenuScreenMain()
{
   setFilename("data/menus/titlescreen.psd");
}

void MenuScreenMain::update(const sf::Time& /*dt*/)
{
}

void MenuScreenMain::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Up)
   {
      up();
   }

   else if (key == sf::Keyboard::Down)
   {
      down();
   }

   else if (key == sf::Keyboard::Return)
   {
      select();
   }
}

void MenuScreenMain::loadingFinished()
{
   SaveState::deserializeFromFile();
   updateLayers();
}

void MenuScreenMain::up()
{
   switch (_selection)
   {
      case Selection::Start:
      {
         _selection = Selection::Quit;
         break;
      }
      case Selection::Options:
      {
         _selection = Selection::Start;
         break;
      }
      case Selection::Quit:
      {
         _selection = Selection::Options;
         break;
      }
   }

   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenMain::down()
{
   switch (_selection)
   {
      case Selection::Start:
         _selection = Selection::Options;
         break;
      case Selection::Options:
         _selection = Selection::Quit;
         break;
      case Selection::Quit:
         _selection = Selection::Start;
         break;
   }

   updateLayers();

   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenMain::select()
{
   switch (_selection)
   {
      case Selection::Start:
      {
#ifdef DEV_SAVE_STATE
         Menu::getInstance()->show(Menu::MenuType::FileSelect);
#else
         Menu::getInstance()->hide();
         GameState::getInstance().enqueueResume();
#endif
         break;
      }
      case Selection::Options:
      {
         Menu::getInstance()->show(Menu::MenuType::Options);
         break;
      }
      case Selection::Quit:
      {
         MessageBox::question(
            "Are you sure you want to quit?",
            [this](MessageBox::Button button)
            {
               if (button == MessageBox::Button::Yes)
                  _exit_callback();
            }
         );
         break;
      }
   }

   MenuAudio::play(MenuAudio::SoundEffect::ItemSelect);
}

void MenuScreenMain::setExitCallback(MenuScreenMain::ExitCallback callback)
{
   _exit_callback = callback;
}

void MenuScreenMain::updateLayers()
{
   const auto canContinue = !SaveState::allEmpty();

   _layers["continue_0"]->_visible = canContinue && (_selection != Selection::Start);
   _layers["continue_1"]->_visible = canContinue && (_selection == Selection::Start);

   _layers["start_0"]->_visible = !canContinue && (_selection != Selection::Start);
   _layers["start_1"]->_visible = !canContinue && (_selection == Selection::Start);

   _layers["options_0"]->_visible = (_selection != Selection::Options);
   _layers["options_1"]->_visible = (_selection == Selection::Options);

   _layers["quit_0"]->_visible = (_selection != Selection::Quit);
   _layers["quit_1"]->_visible = (_selection == Selection::Quit);
}

/*
data/menus/titlescreen.psd
    bg_temp
    quit_0
    quit_1
    options_0
    options_1
    start_0
    start_1
    version
    credits
    logo
*/
