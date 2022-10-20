#include "waterbubbles.h"

#include "atmosphere.h"
#include "level.h"
#include "texturepool.h"

#include <iostream>

namespace
{

float frand(float min = 0.0f, float max = 1.0f)
{
   const auto val = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
   return min + val * (max - min);
}

}  // namespace

WaterBubbles::WaterBubbles()
{
   _texture = TexturePool::getInstance().get("data/sprites/player.png");
}

void WaterBubbles::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   for (const auto& bubble : _bubbles)
   {
      target.draw(bubble->_sprite);
   }
}

void WaterBubbles::update(const sf::Time& dt, const WaterBubbleInput& input)
{
   if (input._player_in_water)
   {
      duration_since_last_bubbles_s += dt.asSeconds();
   }

   if (duration_since_last_bubbles_s > delay_between_spawn_min_s + delay_between_spawn_variation_s)
   {
      duration_since_last_bubbles_s = 0.0f;
      delay_between_spawn_variation_s = frand();
      const auto spawn_bubble_count = std::max(1, std::rand() % bubble_count_spawn_max);

      std::cout << "spawn " << spawn_bubble_count << " bubbles" << std::endl;

      for (auto i = 0; i < spawn_bubble_count; i++)
      {
         static const auto spriteRects = std::array<sf::IntRect, 4>{
            sf::IntRect{576, 936, 24, 24},
            sf::IntRect{600, 936, 24, 24},
            sf::IntRect{624, 936, 24, 24},
            sf::IntRect{648, 936, 24, 24},
         };

         sf::Vector2f pos = {input._player_rect.left + frand(0.0f, input._player_rect.width), input._player_rect.top};
         sf::Vector2f vel = {0.0f, frand(0.5f, 1.0f)};
         auto bubble = std::make_shared<Bubble>(pos, vel, _texture);
         bubble->_sprite.setTextureRect(spriteRects[std::rand() % spriteRects.size()]);
         _bubbles.push_back(bubble);
      }
   }

   for (const auto& bubble : _bubbles)
   {
      bubble->_position += dt.asSeconds() * bubble->_velocity;

      const auto atmosphere = Level::getCurrentLevel()->getAtmosphere().getTileForPosition(bubble->_position);
      if (atmosphere != AtmosphereTileWaterFull)
      {
         bubble->_pop = true;
         std::cout << "pop bubble" << std::endl;
      }
   }

   // remove popped bubbles
   _bubbles.erase(std::remove_if(_bubbles.begin(), _bubbles.end(), [](const auto& bubble) { return bubble->_pop; }), _bubbles.end());
}

WaterBubbles::Bubble::Bubble(const sf::Vector2f& pos, const sf::Vector2f& vel, const std::shared_ptr<sf::Texture>& texture)
    : _position(pos), _velocity(vel)
{
   _sprite.setTexture(*texture);
}
