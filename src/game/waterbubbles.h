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
   };

   struct WaterBubbleInput
   {
      bool _player_in_water = false;
      sf::FloatRect _player_rect;
   };

   void draw(sf::RenderTarget& target, sf::RenderTarget& normal);
   void update(const sf::Time& dt, const WaterBubbleInput& input);

private:
   std::vector<std::shared_ptr<Bubble>> _bubbles;
   std::shared_ptr<sf::Texture> _texture;

   int32_t bubble_count_spawn_max = 5;
   float duration_since_last_bubbles_s = 0.0f;
   float delay_between_spawn_variation_s = 1.0f;
   float delay_between_spawn_min_s = 5.0f;
};

#endif // WATERBUBBLES_H
