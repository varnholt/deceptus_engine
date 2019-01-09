#include "menuscreenoptions.h"

#include "menu.h"


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
   Menu::getInstance().show(Menu::MenuType::Main);
}


void MenuScreenOptions::loadingFinished()
{
   updateLayers();
}


void MenuScreenOptions::up()
{
   auto next = static_cast<int32_t>(mSelection);
   next--;
   if (next < 0)
   {
      next = static_cast<int32_t>(Selection::Count) - 1;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenOptions::down()
{
   auto next = static_cast<int32_t>(mSelection);
   next++;
   if (next == static_cast<int32_t>(Selection::Count))
   {
      next = 0;
   }

   mSelection = static_cast<Selection>(next);
   updateLayers();
}


void MenuScreenOptions::select()
{
   switch (mSelection)
   {
      case Selection::Controls:
         Menu::getInstance().show(Menu::MenuType::Controls);
         break;
      case Selection::Video:
         Menu::getInstance().show(Menu::MenuType::Video);
         break;
      case Selection::Audio:
         Menu::getInstance().show(Menu::MenuType::Audio);
         break;
      case Selection::Game:
         Menu::getInstance().show(Menu::MenuType::Game);
         break;
      case Selection::Achievements:
         Menu::getInstance().show(Menu::MenuType::Achievements);
         break;
      case Selection::Credits:
         Menu::getInstance().show(Menu::MenuType::Credits);
         break;
      case Selection::Count:
         break;
   }
}


void MenuScreenOptions::updateLayers()
{
   mLayers["back_xbox_0"]->mVisible = false;
   mLayers["back_xbox_1"]->mVisible = false;
   mLayers["accept_xbox_0"]->mVisible = false;
   mLayers["accept_xbox_1"]->mVisible = false;

   mLayers["back_pc_0"]->mVisible = true;
   mLayers["back_pc_1"]->mVisible = false;
   mLayers["accept_pc_0"]->mVisible = true;
   mLayers["accept_pc_1"]->mVisible = false;

   mLayers["credits_0"]->mVisible = (mSelection != Selection::Credits);
   mLayers["credits_1"]->mVisible = (mSelection == Selection::Credits);
   mLayers["achievements_0"]->mVisible = (mSelection != Selection::Achievements);
   mLayers["achievements_1"]->mVisible = (mSelection == Selection::Achievements);
   mLayers["game_0"]->mVisible = (mSelection != Selection::Game);
   mLayers["game_1"]->mVisible = (mSelection == Selection::Game);
   mLayers["audio_0"]->mVisible = (mSelection != Selection::Audio);
   mLayers["audio_1"]->mVisible = (mSelection == Selection::Audio);
   mLayers["video_0"]->mVisible = (mSelection != Selection::Video);
   mLayers["video_1"]->mVisible = (mSelection == Selection::Video);
   mLayers["controls_0"]->mVisible = (mSelection != Selection::Controls);
   mLayers["controls_1"]->mVisible = (mSelection == Selection::Controls);
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

