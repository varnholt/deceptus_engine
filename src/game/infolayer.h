#pragma once

#include "animation.h"
#include "bitmapfont.h"

#include "framework/image/layer.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>


class InfoLayer
{
public:

   InfoLayer();

   void update(const sf::Time& dt);
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void drawDebugInfo(sf::RenderTarget& window);
   void drawConsole(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);

   void setLoading(bool loading);

private:

   void playHeartAnimation();
   void drawHeartAnimation(sf::RenderTarget& window);

   BitmapFont _font;

   bool _loading = false;
   sf::Time _show_time;

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;

   std::vector<std::shared_ptr<Layer>> _heart_layers;
   std::vector<std::shared_ptr<Layer>> _stamina_layers;
   std::shared_ptr<Layer> _character_window_layer;
   std::shared_ptr<Layer> _item_sword_ammo_layer;
   std::shared_ptr<Layer> _weapon_sword_icon_layer;

   std::shared_ptr<Layer> _slot_1_item_layer;
   std::shared_ptr<Layer> _slot_1_weapon_layer;
   std::shared_ptr<Layer> _slot_2_item_layer;
   std::shared_ptr<Layer> _slot_2_weapon_layer;

   Animation _heart_animation;
};

