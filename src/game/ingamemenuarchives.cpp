#include "ingamemenuarchives.h"

// data/game/archives.psd
// add layer: header_bg: 0, -4 (640 x 35)
// add layer: header: 235, 6 (197 x 18)
// add layer: footer: 0, 336 (640 x 24)

InGameMenuArchives::InGameMenuArchives()
{
   _filename = "data/game/archives.psd";
   load();

   _main_panel = {
      _layers["bg"],
      _layers["power_x_0"],
      _layers["power_y_0"],
      _layers["power_z_0"],
      _layers["power_walljump_0"],
      _layers["power_dash_0"],
      _layers["power_doublejump_0"],
      _layers["statistics_window"],
      _layers["menu_achievements"],
      _layers["menu_treasures"],
      _layers["menu_powers"],
      _layers["menu_statistics"],
   };

   updateButtons();
}

void InGameMenuArchives::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);
}

void InGameMenuArchives::update(const sf::Time& /*dt*/)
{
   if (_animation == Animation::MoveInFromLeft || _animation == Animation::MoveInFromRight || _animation == Animation::MoveOutToLeft || _animation == Animation::MoveOutToRight)
   {
      updateMove();
   }
}

void InGameMenuArchives::updateMove()
{
   const auto move_offset = getMoveOffset();

   for (const auto& layer : _main_panel)
   {
      const auto x = layer._pos.x + move_offset.value_or(0.0f);
      layer._layer->_sprite->setPosition(x, layer._pos.y);
   }

   if (!move_offset.has_value())
   {
      _animation.reset();
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

void InGameMenuArchives::show()
{
}

void InGameMenuArchives::hide()
{
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
