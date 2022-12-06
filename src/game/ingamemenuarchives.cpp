#include "ingamemenuarchives.h"

#include "framework/easings/easings.h"

InGameMenuArchives::InGameMenuArchives()
{
   _filename = "data/game/archives.psd";

   load();

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
}

void InGameMenuArchives::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);
}

void InGameMenuArchives::update(const sf::Time& /*dt*/)
{
   if (_animation == Animation::Show || _animation == Animation::Hide)
   {
      updateShowHide();
   }
   else if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft || _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }
}

void InGameMenuArchives::updateMove()
{
   const auto move_offset = getMoveOffset();

   for (const auto& layer : _panel_left)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_right)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   for (const auto& layer : _panel_background)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
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

   constexpr auto duration_show_s = 0.5f;
   constexpr auto duration_hide_s = 1.0f;

   sf::Vector2f panel_left_offset_px;
   sf::Vector2f panel_center_offset_px;
   sf::Vector2f panel_right_offset_px;

   auto alpha = 1.0f;

   // animate show event
   if (_animation == Animation::Show && duration_since_show_s.count() < duration_show_s)
   {
      const auto elapsed_s_normalized = duration_since_show_s.count() / duration_show_s;
      const auto val = (1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f;

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
   if (_animation == Animation::Hide && duration_since_hide_s.count() < duration_hide_s)
   {
      const auto elapsed_s_normalized = duration_since_hide_s.count() / duration_hide_s;
      const auto val = 1.0f - ((1.0f + static_cast<float>(std::cos(elapsed_s_normalized * M_PI))) * 0.5f);

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
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   // move in y
   for (const auto& layer : _panel_right)
   {
      const auto y = layer._pos.y + panel_center_offset_px.y;
      layer._layer->_sprite->setPosition(layer._pos.x, y);
   }

   // fade in/out
   for (const auto& layer : _panel_header)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }

   for (const auto& layer : _panel_background)
   {
      layer._layer->_sprite->setColor(sf::Color(255, 255, 255, static_cast<uint8_t>(layer._alpha * alpha * 255)));
   }
}

void InGameMenuArchives::updateButtons()
{
   const auto show_statiastics = _selected_index == 0;
   const auto show_powers = _selected_index == 1;
   const auto show_treasures = _selected_index == 2;
   const auto show_achievements = _selected_index == 3;
   const auto next_menu = false;
   const auto prev_menu = false;
   const auto xbox = false;
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
