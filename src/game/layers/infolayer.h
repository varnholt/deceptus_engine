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

/// \brief draws and updates the in-game hud, console text, and ui animations.
class InfoLayer
{
public:
   using HighResDuration = std::chrono::high_resolution_clock::duration;

   /// \brief loads ui layers, hud animations, and inventory icon sprites.
   InfoLayer();

   /// \brief updates loading fade, health panel motion, inventory icons, and timed animations.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt);

   /// \brief advances randomized heartbeat, stamina, and skull-blink animations.
   /// \param dt elapsed frame time since the previous update.
   void updateAnimations(const sf::Time& dt);

   /// \brief draws the complete hud pass in view space.
   /// \param window SFML render target used for hud output.
   /// \param RenderStates render state overrides passed to sub-draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);

   /// \brief draws debug text such as player tile position, pixel position, and room name.
   /// \param window SFML render target used for debug text rendering.
   void drawDebugInfo(sf::RenderTarget& window);

   /// \brief draws the developer console, command history, and help topics.
   /// \param window SFML render target used for console output.
   /// \param states render state overrides for console background layers.
   void drawConsole(sf::RenderTarget& window, sf::RenderStates states = sf::RenderStates::Default);

   /// \brief toggles loading mode and triggers hud/show hide transitions.
   /// \param loading true to show loading icon and hide health panel, false to reverse it.
   void setLoading(bool loading);

private:
   /// \brief loads inventory icon atlas sprites and initializes two item slot sprites.
   void loadInventoryItems();

   /// \brief draws the legacy heart animation strip.
   /// \param window SFML render target used for animation output.
   /// \param states render state overrides for the animation draw.
   void drawHeartAnimation(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief draws equipped inventory slot icons and their slot overlays.
   /// \param window SFML render target used for inventory output.
   /// \param states render state overrides for slot layer draws.
   void drawInventoryItem(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief draws health quarters, stamina bars, inventory slots, and ui micro-animations.
   /// \param window SFML render target used for hud output.
   /// \param states render state overrides for layer and animation draws.
   void drawHealth(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief draws camera-pan indicator arrows while keyboard look is active.
   /// \param window SFML render target used for indicator output.
   /// \param states render state overrides for indicator layer draws.
   void drawCameraPanorama(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief draws the loading spinner animation with current fade alpha.
   /// \param window SFML render target used for loading icon output.
   /// \param states render state overrides for animation drawing.
   void drawLoading(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief draws replay recording and playback status icons when enabled.
   /// \param states render state overrides for icon layer draws.
   /// \param window SFML render target used for event replay icon output.
   void drawEventReplay(sf::RenderStates states, sf::RenderTarget& window);

   /// \brief updates slot texture rects from the current saved inventory entries.
   void updateInventoryItems();

   /// \brief animates health panel x-offset for hide/show transitions and applies it to hud layers.
   void updateHealthLayerOffsets();

   /// \brief syncs replay icon visibility with active display mode flags.
   void updateEventReplayIcons();

   BitmapFont _font;
   sf::Font _console_font;  //!< ttf font used exclusively for console text rendering

   std::atomic<bool> _loading;
   std::optional<sf::Time> _show_time_health;
   std::optional<sf::Time> _hide_time_health;

   enum class LoadingFadeState
   {
      None,    // no animation
      FadeIn,  // fading in
      Keep,    // just spinning
      FadeOut  // fading out (unused)
   };

   /// \brief state machine for fading the loading spinner in and out.
   struct LoadingAnimation
   {
      /// \brief starts fade-in and restarts the loading animation.
      void show();

      /// \brief requests fade-out of the loading animation.
      void hide();

      /// \brief advances fade timing, alpha, and animation playback state.
      /// \param delta_time elapsed frame time since the previous update.
      void update(const sf::Time& delta_time);

      /// \brief draws the loading animation when visible.
      /// \param window SFML render target used for loading icon output.
      /// \param states render state overrides for animation drawing.
      void draw(sf::RenderTarget& window, sf::RenderStates states);

      float _alpha{0.0f};
      std::shared_ptr<Animation> _animation;
      std::optional<sf::Time> _show_time;
      std::optional<sf::Time> _hide_time;
      LoadingFadeState _fade_state{LoadingFadeState::None};
      std::atomic<bool> _hide_pending{false};  //!< set from any thread; consumed and applied on the main thread in update().
   } _loading_anim;

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
