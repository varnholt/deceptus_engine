#include "menuscreenpause.h"

#include "framework/tools/localization.h"
#include "framework/tools/sfmlcompat.h"
#include "framework/tools/sfmlstring.h"
#include "game/audio/audio.h"
#include "game/audio/musicfilenames.h"
#include "game/audio/musicplayer.h"
#include "game/sfx/gameaudio.h"
#include "game/state/gamestate.h"
#include "game/ui/menulabel.h"
#include "game/ui/messagebox.h"
#include "menu.h"
#include "menuaudio.h"

#include <algorithm>
#include <cmath>

namespace
{

//! x the rows are centered on
constexpr auto row_center_x_px = 320;

//! width of the diamond ornament left of a selected row inside its layer image
constexpr auto decorator_left_width_px = 74;

//! width of the diamond ornament right of a selected row inside its layer image
constexpr auto decorator_right_width_px = 71;

constexpr auto row_character_size = 12u;

//! rows of the title layer holding the word; everything below is the flourish under it
constexpr auto title_band_height_px = 34;

const sf::Color color_row_normal{67, 69, 91};
const sf::Color color_row_selected{127, 171, 253};

/// \brief returns the image height a row needs so a full-height glyph is not clipped.
///
/// a row layer is only as tall as a latin capital. japanese glyphs are taller than that, so a row
/// composed at the original height would lose the bottom of every one of them.
///
/// \param original_height_px height of the layer as the artwork has it.
/// \param character_size character size the row is rendered at.
/// \return height to build the row image at.
int32_t rowHeight(int32_t original_height_px, uint32_t character_size)
{
   constexpr auto padding_px = 4;
   return std::max(original_height_px, static_cast<int32_t>(character_size) + padding_px);
}

/// \brief redraws one pause menu row so its word comes from the translation table.
///
/// the unselected layer becomes the word on its own, the selected one the same word between the two
/// diamond ornaments the artwork carries. both are recentered afterwards, since a translation is
/// rarely as wide as the english it replaces.
///
/// \param plain_layer layer shown while the row is not selected.
/// \param selected_layer layer shown while the row is selected.
/// \param source_text english source text of the row.
void updateRowLabel(Layer& plain_layer, Layer& selected_layer, const std::string& source_text)
{
   const auto word_width_px = MenuLabel::measureWidth(source_text, row_character_size);

   const auto plain_size = plain_layer._texture->getSize();
   const auto plain_height_px = rowHeight(static_cast<int32_t>(plain_size.y), row_character_size);
   const auto plain_position = sfcompat::getPosition(*plain_layer._sprite);
   const auto plain_y_offset_px = (plain_height_px - static_cast<int32_t>(plain_size.y)) / 2;

   MenuLabel::compose(
      plain_layer,
      {word_width_px, plain_height_px},
      {},
      {MenuLabel::Label{
         ._text = source_text,
         ._box = sf::IntRect{{0, 0}, {word_width_px, plain_height_px}},
         ._align = MenuLabel::Align::Centered,
         ._character_size = row_character_size,
         ._color = color_row_normal
      }}
   );

   sfcompat::setPosition(
      *plain_layer._sprite,
      {static_cast<float>(row_center_x_px - word_width_px / 2), plain_position.y - static_cast<float>(plain_y_offset_px)}
   );

   const auto selected_size = selected_layer._texture->getSize();
   const auto selected_height_px = rowHeight(static_cast<int32_t>(selected_size.y), row_character_size);
   const auto selected_position = sfcompat::getPosition(*selected_layer._sprite);
   const auto selected_y_offset_px = (selected_height_px - static_cast<int32_t>(selected_size.y)) / 2;
   const auto selected_width_px = decorator_left_width_px + word_width_px + decorator_right_width_px;
   const auto decorator_source_height_px = static_cast<int32_t>(selected_size.y);

   MenuLabel::compose(
      selected_layer,
      {selected_width_px, selected_height_px},
      {MenuLabel::KeptRegion{
          ._source = sf::IntRect{{0, 0}, {decorator_left_width_px, decorator_source_height_px}},
          ._target = sf::Vector2i{0, selected_y_offset_px}
       },
       MenuLabel::KeptRegion{
          ._source =
             sf::IntRect{
                {static_cast<int32_t>(selected_size.x) - decorator_right_width_px, 0},
                {decorator_right_width_px, decorator_source_height_px}
             },
          ._target = sf::Vector2i{decorator_left_width_px + word_width_px, selected_y_offset_px}
       }},
      {MenuLabel::Label{
         ._text = source_text,
         ._box = sf::IntRect{{decorator_left_width_px, 0}, {word_width_px, selected_height_px}},
         ._align = MenuLabel::Align::Centered,
         ._character_size = row_character_size,
         ._color = color_row_selected
      }}
   );

   sfcompat::setPosition(
      *selected_layer._sprite,
      {static_cast<float>(row_center_x_px - word_width_px / 2 - decorator_left_width_px),
       selected_position.y - static_cast<float>(selected_y_offset_px)}
   );
}

}  // namespace

MenuScreenPause::MenuScreenPause()
{
   setFilename("data/menus/pause.psd");
}

void MenuScreenPause::update(const sf::Time& /*dt*/)
{
}

void MenuScreenPause::keyboardKeyPressed(sf::Keyboard::Key key)
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
      resume();
   }
}

void MenuScreenPause::loadingFinished()
{
   updateRowLabel(*_layers["resume_0"], *_layers["resume_1"], "Resume");
   updateRowLabel(*_layers["options_0"], *_layers["options_1"], "Options");
   updateRowLabel(*_layers["quit_game_0"], *_layers["quit_game_1"], "Quit Game");

   setTitle("header", "Pause", title_band_height_px);

#ifdef DECEPTUS_VRSFML
   _text_back_button = std::make_unique<sf::Text>(_font, sf::Text::Data{});
#else
   _text_back_button = std::make_unique<sf::Text>(_font);
#endif
   _text_back_button->setCharacterSize(12);
   _text_back_button->setFillColor(color_label_normal);
#ifdef DECEPTUS_VRSFML
   _text_accept_button = std::make_unique<sf::Text>(_font, sf::Text::Data{});
#else
   _text_accept_button = std::make_unique<sf::Text>(_font);
#endif
   _text_accept_button->setCharacterSize(12);
   _text_accept_button->setFillColor(color_label_normal);

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

   MusicPlayer::getInstance().queueTrack(
      {.filename = MusicFilenames::getLevelMusic().string(),
       .transition = MusicPlayerTypes::TransitionType::Crossfade,
       .duration = std::chrono::milliseconds(1000),
       .post_action = MusicPlayerTypes::PostPlaybackAction::Loop}
   );
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
            tr("Do you want to end the game?"),
            [](MessageBox::Button button)
            {
               if (button == MessageBox::Button::Yes)
               {
                  GameState::getInstance().enqueueStop();
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

   if (!_text_back_button)
   {
      return;
   }

   const auto& back_layer = isControllerUsed() ? _layers["back_xbox_0"] : _layers["back_pc_0"];
   _text_back_button->setString(sftr("Back"));
   placeTextRightOf(*_text_back_button, back_layer->_sprite->getGlobalBounds());

   const auto& accept_layer = isControllerUsed() ? _layers["accept_xbox_0"] : _layers["accept_pc_0"];
   _text_accept_button->setString(sftr("Accept"));
   placeTextRightOf(*_text_accept_button, accept_layer->_sprite->getGlobalBounds());
}

void MenuScreenPause::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   MenuScreen::draw(window, states);

   if (!_text_back_button)
   {
      return;
   }

   window.draw(*_text_back_button, states);
   window.draw(*_text_accept_button, states);
}
