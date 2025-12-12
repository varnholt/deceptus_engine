#pragma once

#include "framework/image/layer.h"
#include "game/animation/animation.h"
#include "game/animation/animationpool.h"
#include "game/image/layerdata.h"
#include "game/layers/bitmapfont.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <atomic>
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

   void drawHeartAnimation(sf::RenderTarget& window, sf::RenderStates states);
   void drawInventoryItem(sf::RenderTarget& window, sf::RenderStates states);
   void drawHealth(sf::RenderTarget& window, sf::RenderStates states);
   void drawCameraPanorama(sf::RenderTarget& window, sf::RenderStates states);
   void drawLoading(sf::RenderTarget& window, sf::RenderStates states);
   void drawEventReplay(sf::RenderStates states, sf::RenderTarget& window);

   void updateInventoryItems();
   void updateHealthLayerOffsets();
   void updateEventReplayIcons();
   void updateLoading();

   BitmapFont _font;

   std::atomic<bool> _loading;
   std::optional<sf::Time> _show_time;
   std::optional<sf::Time> _hide_time;
   float _loading_alpha{0.0f};

   std::map<std::string, std::shared_ptr<LayerData>> _layers;
   std::vector<std::shared_ptr<LayerData>> _player_health_layers;
   static constexpr float x_offset_hidden = -220.0f;
   float _player_health_x_offset{x_offset_hidden};

   std::vector<std::shared_ptr<Layer>> _heart_layers;
   std::vector<std::shared_ptr<Layer>> _stamina_layers;
   std::shared_ptr<Layer> _character_window_layer;

   Animation _heart_animation;

   // small UI animations
   AnimationPool _animation_pool{"data/game/ingame_ui.json"};
   std::shared_ptr<Animation> _animation_heart;
   std::shared_ptr<Animation> _animation_stamina;
   std::shared_ptr<Animation> _animation_skull_blink;
   std::shared_ptr<Animation> _animation_hp_unlock_left;
   std::shared_ptr<Animation> _animation_hp_unlock_right;
   std::array<HighResDuration, 2> _animation_heart_duration_range{};
   std::array<HighResDuration, 2> _animation_stamina_duration_range{};
   std::array<HighResDuration, 2> _animation_skull_blink_duration_range{};
   std::optional<HighResDuration> _next_animation_duration_heart;
   std::optional<HighResDuration> _next_animation_duration_stamina;
   std::optional<HighResDuration> _next_animation_duration_skull_blink;
   HighResDuration _animation_duration_heart{HighResDuration::zero()};
   HighResDuration _animation_duration_stamina{HighResDuration::zero()};
   HighResDuration _animation_duration_skull_blink{HighResDuration::zero()};
   std::shared_ptr<Animation> _animation_loading;

   // inventory
   std::array<std::shared_ptr<Layer>, 2> _slot_item_layers{};
   std::array<std::unique_ptr<sf::Sprite>, 2> _inventory_sprites{};
   std::map<std::string, std::unique_ptr<sf::Sprite>> _sprites;
   std::shared_ptr<sf::Texture> _inventory_texture;

   // event replay
   std::shared_ptr<Layer> _event_replay_recording;
   std::shared_ptr<Layer> _event_replay_playing;
};
