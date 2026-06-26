#include "menuscreenoptions.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlstring.h"
#include "menu.h"
#include "menuaudio.h"

MenuScreenOptions::MenuScreenOptions()
{
   setFilename("data/menus/options.psd");
}

void MenuScreenOptions::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Key::Up)
   {
      up();
   }

   if (key == sf::Keyboard::Key::Down)
   {
      down();
   }

   if (key == sf::Keyboard::Key::Enter)
   {
      select();
   }

   if (key == sf::Keyboard::Key::Escape)
   {
      back();
   }
}

void MenuScreenOptions::showEvent()
{
   const auto previous = Menu::getInstance()->getPreviousType();
   if (previous == Menu::MenuType::Main || previous == Menu::MenuType::Pause)
   {
      _back_target = previous;
   }
   updateLayers();
}

void MenuScreenOptions::back()
{
   Menu::getInstance()->show(_back_target);
   MenuAudio::play(MenuAudio::SoundEffect::MenuBack);
}

void MenuScreenOptions::loadingFinished()
{
   _row_label_base_rect = _layers["controls_0"]->_sprite->getGlobalBounds();
   _row_stride = _layers["video_0"]->_sprite->getGlobalBounds().position.y - _row_label_base_rect.position.y;

   _layers["deco_l"]->_visible = false;
   _layers["deco_r"]->_visible = false;

   for (const auto& layer_name :
        {"controls_0",
         "controls_1",
         "video_0",
         "video_1",
         "audio_0",
         "audio_1",
         "game_0",
         "game_1",
         "achievements_0",
         "achievements_1",
         "credits_0",
         "credits_1"})
   {
      _layers[layer_name]->_visible = false;
   }

   auto make_item_text = [this]() -> std::unique_ptr<sf::Text>
   {
      auto text = std::make_unique<sf::Text>(_font);
      text->setFont(_font);
      text->setCharacterSize(12);
      return text;
   };

   _text_controls_item = make_item_text();
   _text_video_item = make_item_text();
   _text_audio_item = make_item_text();
   _text_game_item = make_item_text();
   _text_achievements_item = make_item_text();
   _text_credits_item = make_item_text();

   _text_back_button = make_item_text();
   _text_back_button->setFillColor(color_label_normal);
   _text_accept_button = make_item_text();
   _text_accept_button->setFillColor(color_label_normal);

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

   if (!_text_controls_item)
   {
      return;
   }

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   const auto& accept_layer = isControllerUsed() ? _layers["accept_xbox_0"] : _layers["accept_pc_0"];
   _text_accept_button->setString(sftr("Accept"));
   placeTextRightOf(*_text_accept_button, accept_layer->_sprite->getGlobalBounds());

   auto update_item = [this](sf::Text& text, const sf::FloatRect& reference_rect, const std::string& label, bool selected)
   {
      text.setString(label);
      text.setFillColor(selected ? color_label_selected : color_label_normal);
      placeTextCentered(text, reference_rect);
   };

   update_item(*_text_controls_item, rowRect(_row_label_base_rect, 0), sftr("Controls"), _selection == Selection::Controls);
   update_item(*_text_video_item, rowRect(_row_label_base_rect, 1), sftr("Video"), _selection == Selection::Video);
   update_item(*_text_audio_item, rowRect(_row_label_base_rect, 2), sftr("Audio"), _selection == Selection::Audio);
   update_item(*_text_game_item, rowRect(_row_label_base_rect, 3), sftr("Game"), _selection == Selection::Game);
   // update_item(*_text_achievements_item, rowRect(_row_label_base_rect, 4), sftr("Achievements"), _selection == Selection::Achievements);
   update_item(*_text_credits_item, rowRect(_row_label_base_rect, 4), sftr("Credits"), _selection == Selection::Credits);

   sf::Text* active_text = nullptr;
   switch (_selection)
   {
      case Selection::Controls:
         active_text = _text_controls_item.get();
         break;
      case Selection::Video:
         active_text = _text_video_item.get();
         break;
      case Selection::Audio:
         active_text = _text_audio_item.get();
         break;
      case Selection::Game:
         active_text = _text_game_item.get();
         break;
      case Selection::Achievements:
         active_text = _text_achievements_item.get();
         break;
      case Selection::Credits:
         active_text = _text_credits_item.get();
         break;
      case Selection::Count:
         break;
   }

   if (active_text)
   {
      placeDecorators(active_text->getGlobalBounds());
   }
}

void MenuScreenOptions::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   if (!_text_controls_item)
   {
      return;
   }

   window.draw(*_text_controls_item, states);
   window.draw(*_text_video_item, states);
   window.draw(*_text_audio_item, states);
   window.draw(*_text_game_item, states);
   // window.draw(*_text_achievements_item, states);
   window.draw(*_text_credits_item, states);

   window.draw(*_text_back_button, states);
   window.draw(*_text_accept_button, states);
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
