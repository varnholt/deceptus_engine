#include "menuscreenmain.h"

#include <chrono>
#include <cstdint>
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "game/ui/messagebox.h"
#include "menu.h"
#include "menuaudio.h"

#define DEV_SAVE_STATE 1

namespace
{

std::string getBuildNumber()
{
   return std::format("{}", BUILD_NUMBER);
}

}  // namespace

MenuScreenMain::MenuScreenMain()
{
   setFilename("data/menus/titlescreen.psd");

   _font.openFromFile("data/fonts/deceptum.ttf");
   const_cast<sf::Texture&>(_font.getTexture(12)).setSmooth(false);

   _text_build = std::make_unique<sf::Text>(_font);

   _text_build->setFont(_font);
   _text_build->setString(getBuildNumber());
   _text_build->setCharacterSize(12);
   _text_build->setPosition({70, 341});
   _text_build->setFillColor(sf::Color{50, 50, 50});

   const auto current_year =
      static_cast<int32_t>(std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())}.year());

   _text_year = std::make_unique<sf::Text>(_font);

   _text_year->setFont(_font);
   _text_year->setString(std::to_string(current_year));
   _text_year->setCharacterSize(12);
   _text_year->setPosition({344, 341});
   _text_year->setFillColor(sf::Color{127, 171, 253});
}

void MenuScreenMain::update(const sf::Time& /*dt*/)
{
   // check if fade-in is still active and should be completed
   if (_fade_in_active && _fade_in_clock.getElapsedTime().asMilliseconds() >= _fade_in_duration)
   {
      _fade_in_active = false;
   }
}

void MenuScreenMain::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   // fade-in
   if (_fade_in_active)
   {
      const auto elapsed = _fade_in_clock.getElapsedTime().asMilliseconds();
      const auto alpha_ratio = std::min(elapsed / _fade_in_duration, 1.0f);
      const auto alpha = static_cast<std::uint8_t>(255 * alpha_ratio);

      // create a temporary render texture to apply alpha to the entire menu
      sf::RenderTexture temp_texture(sf::Vector2u{window.getSize()});
      temp_texture.clear(sf::Color::Transparent);

      // draw the base menu content to the temporary texture
      MenuScreen::draw(temp_texture, states);
      temp_texture.draw(*_text_build);
      temp_texture.draw(*_text_year);
      temp_texture.display();

      // create a sprite and apply alpha
      sf::Sprite temp_sprite(temp_texture.getTexture());
      temp_sprite.setColor(sf::Color(255, 255, 255, alpha));

      // draw the faded sprite to the main window
      window.draw(temp_sprite, states);
   }
   else
   {
      // normal drawing without fade
      MenuScreen::draw(window, states);
      window.draw(*_text_build);
      window.draw(*_text_year);
   }
}

void MenuScreenMain::keyboardKeyPressed(sf::Keyboard::Key key)
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
}

void MenuScreenMain::showEvent()
{
   // Only do fade-in the first time the menu is shown
   if (_first_time_shown)
   {
      _first_time_shown = false;
      _fade_in_active = true;
      _fade_in_clock.restart();
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
               {
                  _exit_callback();
               }
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
