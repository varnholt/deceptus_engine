#include "menuscreenmain.h"

#include <chrono>
#include "framework/easings/easings.h"
#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "game/ui/messagebox.h"
#include "menu.h"
#include "menuaudio.h"

#define DEV_SAVE_STATE 1

namespace
{

constexpr float build_text_x_offset = 4.0f;

std::string getBuildNumber()
{
   return std::format("{}", BUILD_NUMBER);
}

}  // namespace

MenuScreenMain::MenuScreenMain()
{
   setFilename("data/menus/titlescreen.psd");

   _text_build = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_build->setFont(_font);
   _text_build->setString((tr("Build Number") + ": " + std::string{getBuildNumber()}).c_str());
   _text_build->setCharacterSize(12);
   _text_build->position = {build_text_x_offset, 341};
   _text_build->setFillColor(sf::Color{50, 50, 50});

   const auto current_year =
      static_cast<int32_t>(std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())}.year());

   _text_copyright = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_copyright->setFont(_font);
   const auto copyright_string = std::vformat(tr("© {} Matthias Varnholt & dstar"), std::make_format_args(current_year));
   _text_copyright->setString(copyright_string.c_str());
   _text_copyright->setCharacterSize(12);
   _text_copyright->setFillColor(sf::Color{127, 171, 253});

   const auto copyright_bounds = _text_copyright->getLocalBounds();
   const auto copyright_x = static_cast<int32_t>((640.0f - copyright_bounds.size.x) / 2.0f - copyright_bounds.position.x);
   _text_copyright->position = {static_cast<float>(copyright_x), 341.0f};
}

void MenuScreenMain::update(const sf::Time& /*dt*/)
{
   // only do fade-in the first time the menu is shown
   if (_first_time_shown)
   {
      _first_time_shown = false;
      _fade_in_active = true;
      _fade_in_clock.restart();
   }

   // check if fade-in is still active and should be completed
   if (_fade_in_active)
   {
      constexpr float fade_in_duration_ms = 1000.0f;
      const auto elapsed = _fade_in_clock.getElapsedTime().asMilliseconds();
      const auto ratio_normalized = std::min(elapsed / fade_in_duration_ms, 1.0f);
      const auto eased_ratio = Easings::easeInOutQuad(ratio_normalized);
      _fade_alpha = static_cast<std::uint8_t>(255 * eased_ratio);

      if (_fade_in_clock.getElapsedTime().asMilliseconds() >= fade_in_duration_ms)
      {
         _fade_in_active = false;
      }
   }
}

void MenuScreenMain::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   const auto can_continue = !SaveState::allEmpty();

   auto draw_all_text = [&](sf::RenderTarget& target)
   {
      target.draw(*_text_build);
      target.draw(*_text_copyright);
      if (can_continue)
      {
         target.draw(*_text_continue_item);
      }
      else
      {
         target.draw(*_text_new_game_item);
      }
      target.draw(*_text_options_item);
      target.draw(*_text_quit_item);
   };

   // fade-in
   if (_fade_in_active)
   {
      auto temp_texture = *sf::RenderTexture::create(sf::Vector2u{window.getSize()});
      temp_texture.clear(sf::Color::Transparent);

      // draw the base menu content to the temporary texture
      MenuScreen::draw(temp_texture, states);
      if (_text_continue_item)
      {
         draw_all_text(temp_texture);
      }
      temp_texture.display();

      // create a sprite and apply alpha
      const sf::Texture& temp_fade_texture = temp_texture.getTexture();
      sf::Sprite temp_sprite;
      temp_sprite.textureRect =
         sf::FloatRect{{0.f, 0.f}, {static_cast<float>(temp_fade_texture.getSize().x), static_cast<float>(temp_fade_texture.getSize().y)}};
      temp_sprite.color = sf::Color(255, 255, 255, _fade_alpha);

      // draw the faded sprite to the main window
      sf::RenderStates faded_states = states;
      faded_states.texture = &temp_fade_texture;
      window.draw(temp_sprite, faded_states);
   }
   else
   {
      // normal drawing without fade
      MenuScreen::draw(window, states);
      if (_text_continue_item)
      {
         draw_all_text(window);
      }
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

void MenuScreenMain::loadingFinished()
{
   _row_label_base_rect = _layers["continue_0"]->_sprite->getGlobalBounds();
   _row_stride = _layers["options_0"]->_sprite->getGlobalBounds().position.y - _row_label_base_rect.position.y;

   _layers["deco_l"]->_visible = false;
   _layers["deco_r"]->_visible = false;

   for (const auto& layer_name : {"continue_0", "continue_1", "start_0", "start_1", "options_0", "options_1", "quit_0", "quit_1"})
   {
      _layers[layer_name]->_visible = false;
   }

   if (_layers.contains("build"))
   {
      _layers["build"]->_visible = false;
   }

   if (_layers.contains("credits"))
   {
      _layers["credits"]->_visible = false;
   }

   _text_continue_item = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_continue_item->setFont(_font);
   _text_continue_item->setCharacterSize(12);

   _text_new_game_item = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_new_game_item->setFont(_font);
   _text_new_game_item->setCharacterSize(12);

   _text_options_item = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_options_item->setFont(_font);
   _text_options_item->setCharacterSize(12);

   _text_quit_item = std::make_unique<sf::Text>(_font, sf::Text::Data{});
   _text_quit_item->setFont(_font);
   _text_quit_item->setCharacterSize(12);

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
            tr("Are you sure you want to quit?"),
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
   if (!_text_continue_item)
   {
      return;
   }

   const auto start_color = (_selection == Selection::Start) ? color_label_selected : color_label_normal;
   const auto options_color = (_selection == Selection::Options) ? color_label_selected : color_label_normal;
   const auto quit_color = (_selection == Selection::Quit) ? color_label_selected : color_label_normal;

   _text_continue_item->setString(sftr("Continue"));
   _text_continue_item->setFillColor(start_color);
   placeTextCentered(*_text_continue_item, rowRect(_row_label_base_rect, 0));

   _text_new_game_item->setString(sftr("New Game"));
   _text_new_game_item->setFillColor(start_color);
   placeTextCentered(*_text_new_game_item, rowRect(_row_label_base_rect, 0));

   _text_options_item->setString(sftr("Options"));
   _text_options_item->setFillColor(options_color);
   placeTextCentered(*_text_options_item, rowRect(_row_label_base_rect, 1));

   _text_quit_item->setString(sftr("Quit"));
   _text_quit_item->setFillColor(quit_color);
   placeTextCentered(*_text_quit_item, rowRect(_row_label_base_rect, 2));

   const auto can_continue = !SaveState::allEmpty();

   sf::FloatRect active_text_bounds;
   switch (_selection)
   {
      case Selection::Start:
         active_text_bounds = can_continue ? _text_continue_item->getGlobalBounds() : _text_new_game_item->getGlobalBounds();
         break;
      case Selection::Options:
         active_text_bounds = _text_options_item->getGlobalBounds();
         break;
      case Selection::Quit:
         active_text_bounds = _text_quit_item->getGlobalBounds();
         break;
   }

   placeDecorators(active_text_bounds);
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
