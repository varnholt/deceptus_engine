#include "menuscreenpause.h"

#include "game/gameaudio.h"
#include "game/gamestate.h"
#include "game/ui/messagebox.h"
#include "menu.h"
#include "menuaudio.h"

MenuScreenPause::MenuScreenPause()
{
   setFilename("data/menus/pause.psd");
}


void MenuScreenPause::update(const sf::Time& /*dt*/)
{

}


void MenuScreenPause::keyboardKeyPressed(sf::Keyboard::Key key)
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
   else if (key == sf::Keyboard::Escape)
   {
      resume();
   }
}


void MenuScreenPause::loadingFinished()
{
   updateLayers();
}


void MenuScreenPause::up()
{
   switch (_selection)
   {
      case Selection::Resume:
         _selection = Selection::Quit;
         break;
      case Selection::Options:
         _selection = Selection::Resume;
         break;
      case Selection::Quit:
         _selection = Selection::Options;
         break;
   }

   updateLayers();
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenPause::down()
{
   switch (_selection)
   {
      case Selection::Resume:
         _selection = Selection::Options;
         break;
      case Selection::Options:
         _selection = Selection::Quit;
         break;
      case Selection::Quit:
         _selection = Selection::Resume;
         break;
   }

   updateLayers();
   MenuAudio::play(MenuAudio::SoundEffect::ItemNavigate);
}

void MenuScreenPause::resume()
{
   Menu::getInstance()->hide();
   GameState::getInstance().enqueueResume();
   GameAudio::getInstance().play(GameAudio::SoundEffect::GameStateResume);
}


void MenuScreenPause::select()
{
   switch (_selection)
   {
      case Selection::Resume:
         resume();
         break;
      case Selection::Options:
         Menu::getInstance()->show(Menu::MenuType::Options);
         MenuAudio::play(MenuAudio::SoundEffect::Apply);
         break;
      case Selection::Quit:
         MessageBox::question(
            "Do you want to end the game?",
            [](MessageBox::Button button) {
               if (button == MessageBox::Button::Yes)
               {
                  GameState::getInstance().enqueuePause();
                  Menu::getInstance()->show(Menu::MenuType::Main);
               }
            }
         );
         break;
   }
}


void MenuScreenPause::showEvent()
{
   // initial selection after coming from pause state should always be 'resume'
   _selection = Selection::Resume;
   updateLayers();
}


void MenuScreenPause::updateLayers()
{
   _layers["resume_0"]->_visible = (_selection != Selection::Resume);
   _layers["resume_1"]->_visible = (_selection == Selection::Resume);
   _layers["options_0"]->_visible = (_selection != Selection::Options);
   _layers["options_1"]->_visible = (_selection == Selection::Options);
   _layers["quit_game_0"]->_visible = (_selection != Selection::Quit);
   _layers["quit_game_1"]->_visible = (_selection == Selection::Quit);

   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;
   _layers["accept_xbox_0"]->_visible = isControllerUsed();
   _layers["accept_xbox_1"]->_visible = false;

   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;
   _layers["accept_pc_0"]->_visible = !isControllerUsed();
   _layers["accept_pc_1"]->_visible = false;
}

