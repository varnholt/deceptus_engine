#pragma once

#include "framework/image/layer.h"
#include "game/animation.h"
#include "game/animationpool.h"
#include "game/bitmapfont.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>


class InfoLayer
{
public:
   using HighResDuration = std::chrono::high_resolution_clock::duration;

   InfoLayer();

   void update(const sf::Time& dt);
   void updateAnimations(const sf::Time& dt);

   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   void drawDebugInfo(sf::RenderTarget& window);
   void drawConsole(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);

   void setLoading(bool loading);

private:
   void loadInventoryItems();
   void playHeartAnimation();
   void drawHeartAnimation(sf::RenderTarget& window);
   void drawInventoryItem(sf::RenderTarget& window);
   void updateInventoryItems();

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

   // small UI animations
   AnimationPool _animation_pool{"data/game/ingame_ui.json"};
   std::shared_ptr<Animation> _animation_heart;
   std::shared_ptr<Animation> _animation_stamina;
   std::shared_ptr<Animation> _animation_skull_blink;
   std::shared_ptr<Animation> _animation_hp_unlock_left;
   std::shared_ptr<Animation> _animation_hp_unlock_right;
   std::array<HighResDuration, 2> _animation_heart_duration_range;
   std::array<HighResDuration, 2> _animation_stamina_duration_range;
   std::array<HighResDuration, 2> _animation_skull_blink_duration_range;
   std::optional<HighResDuration> _next_animation_duration_heart;
   std::optional<HighResDuration> _next_animation_duration_stamina;
   std::optional<HighResDuration> _next_animation_duration_skull_blink;
   HighResDuration _animation_duration_heart{HighResDuration::zero()};
   HighResDuration _animation_duration_stamina{HighResDuration::zero()};
   HighResDuration _animation_duration_skull_blink{HighResDuration::zero()};

   // inventory
   sf::Sprite _inventory_sprite;
   std::map<std::string, sf::Sprite> _sprites;
   std::shared_ptr<sf::Texture> _inventory_texture;
};
