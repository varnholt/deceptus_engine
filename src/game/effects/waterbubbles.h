#ifndef WATERBUBBLES_H
#define WATERBUBBLES_H

#include "SFML/Graphics.hpp"

#include <memory>
#include <vector>

class WaterBubbles
{
public:
   WaterBubbles();

   struct Bubble
   {
      Bubble(const sf::Vector2f& pos, const sf::Vector2f& vel, const std::shared_ptr<sf::Texture>& texture);
      sf::Sprite _sprite;
      sf::Vector2f _position;
      sf::Vector2f _velocity;
      bool _pop = false;
      float _delay_s = 0.0f;
      float _alive_s = 0.0f;
   };

   struct WaterBubbleInput
   {
      bool _player_in_water = false;
      bool _player_pointing_right = false;
      sf::FloatRect _player_rect;
   };

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   void update(const sf::Time& dt, const WaterBubbleInput& input);

private:
   void spawnBubble(const sf::Vector2f pos_px, const sf::Vector2f vel_px);
   void spawnBubblesFromHead(const WaterBubbleInput& input);
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
