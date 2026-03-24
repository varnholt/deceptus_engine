#ifndef WATERBUBBLES_H
#define WATERBUBBLES_H

#include "SFML/Graphics.hpp"

#include <memory>
#include <vector>

/// \brief spawns and animates rising bubble sprites around the player while underwater.
class WaterBubbles
{
public:
   /// \brief loads the shared player texture used for bubble sprite frames.
   WaterBubbles();

   /// \brief runtime state for a single bubble sprite, including motion and delayed spawn.
   struct Bubble
   {
      /// \brief creates a bubble at a position with a starting velocity.
      /// \param pos initial bubble position in pixels.
      /// \param vel initial bubble velocity in pixels per second.
      /// \param texture shared texture that contains bubble frames.
      Bubble(const sf::Vector2f& pos, const sf::Vector2f& vel, const std::shared_ptr<sf::Texture>& texture);
      std::unique_ptr<sf::Sprite> _sprite;
      sf::Vector2f _position;
      sf::Vector2f _velocity;
      bool _pop = false;
      float _delay_s = 0.0f;
      float _alive_s = 0.0f;
   };

   /// \brief per-frame player context used to decide when and where bubbles should spawn.
   struct WaterBubbleInput
   {
      bool _player_in_water = false;
      bool _player_pointing_right = false;
      sf::FloatRect _player_rect;
   };

   /// \brief draws bubbles whose startup delay already elapsed.
   /// \param target render target.
   /// \param normal normal-map render target, currently unused by this effect.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   /// \brief advances bubble spawning, movement, and removal based on player water state.
   /// \param dt elapsed frame time since the previous update.
   /// \param input current player water state and bounding rectangle.
   void update(const sf::Time& dt, const WaterBubbleInput& input);

private:
   /// \brief creates one bubble with random frame selection and a short random start delay.
   /// \param pos_px position in pixels.
   /// \param vel_px initial velocity in pixels per second.
   void spawnBubble(const sf::Vector2f pos_px, const sf::Vector2f vel_px);
   /// \brief emits periodic rising bubbles near the player's head region while submerged.
   /// \param input current player water state and bounding rectangle.
   void spawnBubblesFromHead(const WaterBubbleInput& input);
   /// \brief emits burst bubbles for several frames after entering water.
   /// \param input current player water state and bounding rectangle.
   void spawnSplashBubbles(const WaterBubbleInput& input);

   std::vector<std::shared_ptr<Bubble>> _bubbles;
   std::shared_ptr<sf::Texture> _texture;
   WaterBubbleInput _previous_input;

   float _duration_since_last_bubbles_s = 0.0f;
   float _delay_between_spawn_variation_s = 1.0f;
   float _delay_between_spawn_min_s = 4.0f;

   int32_t _dive_frames_left = 0;
};

#endif  // WATERBUBBLES_H
