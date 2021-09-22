#include "menuscreenpause.h"

#include "game/messagebox.h"
#include "game/gamestate.h"
#include "menu.h"


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
   switch (mSelection)
   {
      case Selection::Resume:
         mSelection = Selection::Quit;
         break;
      case Selection::Options:
         mSelection = Selection::Resume;
         break;
      case Selection::Quit:
         mSelection = Selection::Options;
         break;
   }

   updateLayers();
}


void MenuScreenPause::down()
{
   switch (mSelection)
   {
      case Selection::Resume:
         mSelection = Selection::Options;
         break;
      case Selection::Options:
         mSelection = Selection::Quit;
         break;
      case Selection::Quit:
         mSelection = Selection::Resume;
         break;
   }

   updateLayers();
}


void MenuScreenPause::resume()
{
   Menu::getInstance()->hide();
   GameState::getInstance().enqueueResume();
}


void MenuScreenPause::select()
{
   switch (mSelection)
   {
      case Selection::Resume:
         resume();
         break;
      case Selection::Options:
         Menu::getInstance()->show(Menu::MenuType::Options);
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
   mSelection = Selection::Resume;
   updateLayers();
}


void MenuScreenPause::updateLayers()
{
   mLayers["resume_0"]->_visible = (mSelection != Selection::Resume);
   mLayers["resume_1"]->_visible = (mSelection == Selection::Resume);
   mLayers["options_0"]->_visible = (mSelection != Selection::Options);
   mLayers["options_1"]->_visible = (mSelection == Selection::Options);
   mLayers["quit_game_0"]->_visible = (mSelection != Selection::Quit);
   mLayers["quit_game_1"]->_visible = (mSelection == Selection::Quit);

   mLayers["back_xbox_0"]->_visible = isControllerUsed();
   mLayers["back_xbox_1"]->_visible = false;
   mLayers["accept_xbox_0"]->_visible = isControllerUsed();
   mLayers["accept_xbox_1"]->_visible = false;

   mLayers["back_pc_0"]->_visible = !isControllerUsed();
   mLayers["back_pc_1"]->_visible = false;
   mLayers["accept_pc_0"]->_visible = !isControllerUsed();
   mLayers["accept_pc_1"]->_visible = false;
}

