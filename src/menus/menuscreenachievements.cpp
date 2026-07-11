#include "menuscreenachievements.h"

#include "framework/tools/sfmlstring.h"
#include "menu.h"

MenuScreenAchievements::MenuScreenAchievements()
{
   setFilename("data/menus/achievements.psd");
}

void MenuScreenAchievements::up()
{
}

void MenuScreenAchievements::down()
{
}

void MenuScreenAchievements::select()
{
}

void MenuScreenAchievements::back()
{
   Menu::getInstance()->show(Menu::MenuType::Options);
}

void MenuScreenAchievements::keyboardKeyPressed(sf::Keyboard::Key key)
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

void MenuScreenAchievements::loadingFinished()
{
#ifdef __EMSCRIPTEN__
   _text_back_button = std::make_unique<sf::Text>(_font, sf::Text::Data{});
#else
   _text_back_button = std::make_unique<sf::Text>(_font);
#endif
   _text_back_button->setCharacterSize(12);
   _text_back_button->setFillColor(color_label_normal);

   updateLayers();
}

void MenuScreenAchievements::updateLayers()
{
   _layers["back_xbox_0"]->_visible = isControllerUsed();
   _layers["back_xbox_1"]->_visible = false;

   _layers["back_pc_0"]->_visible = !isControllerUsed();
   _layers["back_pc_1"]->_visible = false;

   if (!_text_back_button)
   {
      return;
   }

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());
}

void MenuScreenAchievements::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   if (!_text_back_button)
   {
      return;
   }

   window.draw(*_text_back_button, states);
}

/*
data/menus/achievements.psd
    bg_tmp
    back_xbox_0
    back_xbox_1
    back_pc_0
    back_pc_1
    title
*/
