#include "ingamemenuarchives.h"

#include "framework/easings/easings.h"
#include "framework/tools/localization.h"
#include "game/ingamemenu/menuconfig.h"
#include "game/player/playerinfo.h"
#include "game/player/treasures.h"
#include "game/state/savestate.h"

#include <iterator>
#include <numbers>
#include <sstream>

namespace
{

constexpr auto treasure_font_size = 12;
constexpr auto treasure_icon_center_x_px = 215.0f;
constexpr auto treasure_row_start_y_px = 100.0f;
constexpr auto treasure_row_spacing_px = 60.0f;
constexpr auto treasure_text_offset_x_px = 255.0f;
constexpr auto treasure_description_wrap_width_px = 300.0f;
constexpr auto treasure_name_y_offset_px = -12.0f;
constexpr auto treasure_description_y_offset_px = 4.0f;

std::string wrapText(const std::string& original_text, float wrap_width, const sf::Font& font, int32_t character_size)
{
   std::string wrapped_text;
   std::string line;
   sf::Text temp_text(font, sf::Text::Data{});
   temp_text.setCharacterSize(character_size);

   std::vector<std::string> words;
   std::istringstream iss(original_text);
   std::copy(std::istream_iterator<std::string>(iss), std::istream_iterator<std::string>(), std::back_inserter(words));

   for (const auto& word : words)
   {
      const std::string test_line = line + word + " ";
      temp_text.setString(test_line.c_str());
      if (temp_text.getLocalBounds().size.x <= wrap_width)
      {
         line = test_line;
      }
      else
      {
         wrapped_text = wrapped_text + line + "\n";
         line = word + " ";
      }
   }
   wrapped_text = wrapped_text + line;
   return wrapped_text;
}

}  // namespace

InGameMenuArchives::InGameMenuArchives()
{
   _filename = "data/game/archives.psd";

   load();

   updateButtons();

   _panel_left = {
      _layers["menu_achievements"],
      _layers["menu_powers"],
      _layers["menu_statistics"],
      _layers["menu_treasures"],
   };

   _panel_right = {
      _layers["power_dash_0"],
      _layers["power_doublejump_0"],
      _layers["power_walljump_0"],
      _layers["power_x_0"],
      _layers["power_y_0"],
      _layers["power_z_0"],
      _layers["statistics_window"],
   };

   _panel_header = {
      _layers["close_pc_0"],
      _layers["close_pc_1"],
      _layers["close_xbox_0"],
      _layers["close_xbox_1"],
      _layers["footer"],
      _layers["header"],
      _layers["header_bg"],
      _layers["next_menu_0"],
      _layers["next_menu_1"],
      _layers["previous_menu_0"],
      _layers["previous_menu_1"],
   };

   _panel_background = {
      _layers["bg"],
   };

   updateButtons();

   MenuConfig config;
   _duration_hide = config._duration_hide;
   _duration_show = config._duration_show;

   _animation_pool = std::make_unique<AnimationPool>("data/sprites/extra_animations.json");

   _text_treasure_name = std::make_unique<sf::Text>(*_font_treasure, sf::Text::Data{});
   _text_treasure_name->setCharacterSize(treasure_font_size);
   _text_treasure_name->setFillColor(sf::Color{232, 219, 243});

   _text_treasure_description = std::make_unique<sf::Text>(*_font_treasure, sf::Text::Data{});
   _text_treasure_description->setCharacterSize(treasure_font_size);
   _text_treasure_description->setFillColor(sf::Color{232, 219, 243});
}

void InGameMenuArchives::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);

   if (_selected_index == 2)
   {
      drawTreasures(window, states);
   }
}

void InGameMenuArchives::update(const sf::Time& dt)
{
   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft ||
            _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }

   if (_selected_index == 2)
   {
      updateTreasureAnimations(dt);
   }
}

void InGameMenuArchives::updateMove()
{
   const auto move_offset = getMoveOffset();

   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->position = {x, layer._pos.y};
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->position = {x, layer._pos.y};
   }

   for (const auto& layer : _panel_background)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->position = {x, layer._pos.y};
   }

   if (!move_offset.has_value())
   {
      _animation.reset();
   }
}

void InGameMenuArchives::updateShowHide()
{
   const auto now = std::chrono::high_resolution_clock::now();

   const FloatSeconds duration_since_show_s = now - _time_show;
   const FloatSeconds duration_since_hide_s = now - _time_hide;

   sf::Vector2f panel_left_offset_px;
   sf::Vector2f panel_center_offset_px;
   sf::Vector2f panel_right_offset_px;

   auto alpha = 1.0f;

   // animate show event
   if (_animation == Animation::Show && duration_since_show_s.count() < _duration_show.count())
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / _duration_show.count();
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * std::numbers::pi))) * 0.5f;

      panel_left_offset_px.x = -200 * val;
      panel_center_offset_px.y = 350 * val;
      panel_right_offset_px.x = 200 * val;

      alpha = Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      panel_left_offset_px.x = 0;
      panel_center_offset_px.y = 0;
      panel_right_offset_px.x = 0;

      alpha = 1.0f;

      if (_animation == Animation::Show)
      {
         _animation.reset();
      }
   }

   // animate hide event
   if (_animation == Animation::Hide && duration_since_hide_s.count() < _duration_hide.count())
   {
      const auto elapsed_s_normalized = duration_since_hide_s.count() / _duration_hide.count();
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * std::numbers::pi))) * 0.5f);

      panel_left_offset_px.x = -200 * val;
      panel_center_offset_px.y = 350 * val;
      panel_right_offset_px.x = 200 * val;

      alpha = 1.0f - Easings::easeInQuint(elapsed_s_normalized);
   }
   else
   {
      if (_animation == Animation::Hide)
      {
         fullyHidden();
      }
   }

   // move in x
   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + panel_left_offset_px.x;
      layer._layer->_sprite->position = {x, layer._pos.y};
   }

   // move in y
   for (const auto& layer : _panel_right)
   {
      const auto y = layer._pos.y + panel_center_offset_px.y;
      layer._layer->_sprite->position = {layer._pos.x, y};
   }

   // fade in/out
   for (const auto& layer : _panel_header)
   {
      layer._layer->_sprite->color = sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255));
   }

   for (const auto& layer : _panel_background)
   {
      layer._layer->_sprite->color = sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255));
   }

   _content_alpha = alpha;
}

void InGameMenuArchives::updateTreasureAnimations(const sf::Time& dt)
{
   const auto& collected = SaveState::getPlayerInfo()._treasures.getCollected();
   const auto move_offset = getMoveOffset().value_or(0.0f);

   int32_t row_index = 0;
   for (const auto& identifier : collected)
   {
      if (_treasure_animations.find(identifier) == _treasure_animations.end())
      {
         auto animation = _animation_pool->create(identifier, 0.0f, 0.0f, true, false);
         animation->_looped = true;
         _treasure_animations[identifier] = animation;
      }

      const auto row_center_y_px = treasure_row_start_y_px + static_cast<float>(row_index) * treasure_row_spacing_px;
      _treasure_animations[identifier]->position = {treasure_icon_center_x_px + move_offset, row_center_y_px};
      _treasure_animations[identifier]->update(dt);

      row_index++;
   }
}

void InGameMenuArchives::drawTreasures(sf::RenderTarget& window, sf::RenderStates states)
{
   if (!_text_treasure_name || !_text_treasure_description)
   {
      return;
   }

   const auto& collected = SaveState::getPlayerInfo()._treasures.getCollected();
   const auto move_offset = getMoveOffset().value_or(0.0f);

   int32_t row_index = 0;
   for (const auto& identifier : collected)
   {
      const auto animation_it = _treasure_animations.find(identifier);
      if (animation_it != _treasure_animations.end())
      {
         animation_it->second->setAlpha(static_cast<uint8_t>(_content_alpha * 255));
         window.draw(*animation_it->second, states);
      }

      const auto definition = TreasureDefinitions::findDefinition(identifier);
      const auto row_center_y_px = treasure_row_start_y_px + static_cast<float>(row_index) * treasure_row_spacing_px;
      const auto text_x_px = treasure_text_offset_x_px + move_offset;
      const auto text_color = sf::Color{232, 219, 243, static_cast<uint8_t>(_content_alpha * 255)};

      _text_treasure_name->setFillColor(text_color);
      _text_treasure_name->setString((definition ? definition->_name : identifier).c_str());
      _text_treasure_name->position = {text_x_px, row_center_y_px + treasure_name_y_offset_px};
      window.draw(*_text_treasure_name, states);

      const auto description_text = definition ? definition->_description : std::string{};
      const auto wrapped_description = wrapText(description_text, treasure_description_wrap_width_px, *_font_treasure, treasure_font_size);
      _text_treasure_description->setFillColor(text_color);
      _text_treasure_description->setString(wrapped_description.c_str());
      _text_treasure_description->position = {text_x_px, row_center_y_px + treasure_description_y_offset_px};
      window.draw(*_text_treasure_description, states);

      row_index++;
   }
}

void InGameMenuArchives::updateButtons()
{
   const auto xbox = true;
   const auto show_statiastics = _selected_index == 0;
   const auto show_powers = _selected_index == 1;
   const auto show_treasures = _selected_index == 2;
   const auto show_achievements = _selected_index == 3;
   const auto next_menu = false;
   const auto prev_menu = false;
   const auto close_enabled = false;

   _layers["menu_statistics"]->_visible = show_statiastics;
   _layers["menu_powers"]->_visible = show_powers;
   _layers["menu_treasures"]->_visible = show_treasures;
   _layers["menu_achievements"]->_visible = show_achievements;

   _layers["statistics_window"]->_visible = show_statiastics;

   _layers["power_x_0"]->_visible = show_powers;
   _layers["power_y_0"]->_visible = show_powers;
   _layers["power_z_0"]->_visible = show_powers;
   _layers["power_walljump_0"]->_visible = show_powers;
   _layers["power_dash_0"]->_visible = show_powers;
   _layers["power_doublejump_0"]->_visible = show_powers;

   _layers["next_menu_0"]->_visible = !next_menu;
   _layers["next_menu_1"]->_visible = next_menu;
   _layers["previous_menu_0"]->_visible = !prev_menu;
   _layers["previous_menu_1"]->_visible = prev_menu;

   _layers["close_xbox_0"]->_visible = xbox;
   _layers["close_xbox_1"]->_visible = xbox && close_enabled;
   _layers["close_pc_0"]->_visible = !xbox;
   _layers["close_pc_1"]->_visible = !xbox && close_enabled;
}

void InGameMenuArchives::up()
{
   _selected_index--;
   _selected_index = std::max(_selected_index, 0);

   updateButtons();
}

void InGameMenuArchives::down()
{
   _selected_index++;
   _selected_index = std::min(_selected_index, 3);

   updateButtons();
}
