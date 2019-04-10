#include "menuscreenpause.h"

#include "game/messagebox.h"
#include "game/gamestate.h"
#include "menu.h"


MenuScreenPause::MenuScreenPause()
{
   setFilename("data/menus/pause.psd");
}


void MenuScreenPause::update(float /*dt*/)
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

   if (key == sf::Keyboard::Escape)
   {
      // resume();
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
            "Are you sure you want to quit?",
            [this](MessageBox::Button button) {if (button == MessageBox::Button::Yes) mExitCallback();}
         );
         break;
   }
}


void MenuScreenPause::setExitCallback(MenuScreenPause::ExitCallback callback)
{
    mExitCallback = callback;
}


void MenuScreenPause::updateLayers()
{
   mLayers["resume_0"]->mVisible = (mSelection != Selection::Resume);
   mLayers["resume_1"]->mVisible = (mSelection == Selection::Resume);
   mLayers["options_0"]->mVisible = (mSelection != Selection::Options);
   mLayers["options_1"]->mVisible = (mSelection == Selection::Options);
   mLayers["quit_game_0"]->mVisible = (mSelection != Selection::Quit);
   mLayers["quit_game_1"]->mVisible = (mSelection == Selection::Quit);

   mLayers["back_xbox_0"]->mVisible = isControllerUsed();
   mLayers["back_xbox_1"]->mVisible = false;
   mLayers["accept_xbox_0"]->mVisible = isControllerUsed();
   mLayers["accept_xbox_1"]->mVisible = false;

   mLayers["back_pc_0"]->mVisible = !isControllerUsed();
   mLayers["back_pc_1"]->mVisible = false;
   mLayers["accept_pc_0"]->mVisible = !isControllerUsed();
   mLayers["accept_pc_1"]->mVisible = false;
}

