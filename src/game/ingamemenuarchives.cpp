#include "ingamemenuarchives.h"

// data/game/archives.psd
// add layer: bg: 0, 0 (641 x 360)
// add layer: footer: 0, 336 (640 x 24)
// add layer: close_xbox_0: 299, 342 (40 x 13)
// add layer: close_xbox_1: 299, 342 (40 x 13)
// add layer: close_pc_0: 297, 340 (42 x 16)
// add layer: close_pc_1: 297, 340 (42 x 16)
// add layer: header_bg: 0, -4 (640 x 35)
// add layer: header: 235, 6 (197 x 18)
// add layer: next_menu_0: 463, 8 (34 x 14)
// add layer: next_menu_1: 463, 8 (34 x 14)
// add layer: previous_menu_0: 142, 8 (34 x 14)
// add layer: previous_menu_1: 142, 8 (34 x 14)

InGameMenuArchives::InGameMenuArchives()
{
   _filename = "data/game/archives.psd";
   load();

   _main_panel = {
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
}

void InGameMenuArchives::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   InGameMenuPage::draw(window, states);
}

void InGameMenuArchives::update(const sf::Time& /*dt*/)
{
   if (_animation == Animation::MoveLeft || _animation == Animation::MoveRight)
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

void InGameMenuArchives::show()
{
}

void InGameMenuArchives::hide()
{
}
