#include "waterbubbles.h"

#include "atmosphere.h"
#include "level.h"
#include "texturepool.h"

#include <iostream>

namespace
{

constexpr auto center_offset_px = 10;
const auto head_offset_percent = 0.66f;
constexpr auto velocity_scale = 100.0f;
constexpr auto bubble_count_spawn_max = 10;

float frand(float min = 0.0f, float max = 1.0f)
{
   const auto val = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
   return min + val * (max - min);
}

}  // namespace

WaterBubbles::WaterBubbles() : _texture(TexturePool::getInstance().get("data/sprites/player.png"))
{
}

void WaterBubbles::draw(sf::RenderTarget& target, sf::RenderTarget& /*normal*/)
{
   for (const auto& bubble : _bubbles)
   {
      // sleeping bubbles aren't drawn just yet
      if (bubble->_alive_s < bubble->_delay_s)
      {
         continue;
      }

      target.draw(bubble->_sprite);
   }
}

//
//                        head
//                      | area |
//        +-------------+------+
//        |             |xxxxxx|
//        |             |xxxxxx|
//        +-------------+------+
//

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
      const auto spawn_bubble_count = std::max(bubble_count_spawn_max / 3, std::rand() % bubble_count_spawn_max);

      // std::cout << "spawn " << spawn_bubble_count << " bubbles" << std::endl;

      for (auto i = 0; i < spawn_bubble_count; i++)
      {
         static const auto sprite_rects = std::array<sf::IntRect, 3>{
            // sf::IntRect{576, 936, 24, 24},
            sf::IntRect{600, 936, 24, 24},
            sf::IntRect{624, 936, 24, 24},
            sf::IntRect{648, 936, 24, 24},
         };

         const auto range_right_px = head_offset_percent * input._player_rect.width +
                                     frand(0.0f, (1.0f - head_offset_percent) * input._player_rect.width) + center_offset_px;
         const auto range_left_px = frand(0.0f, (1.0f - head_offset_percent) * input._player_rect.width) - center_offset_px;
         const auto bubble_pos_x_px = input._player_rect.left + (input._player_pointing_right ? range_right_px : range_left_px);

         const sf::Vector2f pos_px = {bubble_pos_x_px, input._player_rect.top};
         const sf::Vector2f vel_px = {0.0f, -velocity_scale * frand(0.5f, 1.0f)};

         // std::cout << "spawn " << pos_px.x << ", " << pos_px.y << std::endl;

         auto bubble = std::make_shared<Bubble>(pos_px, vel_px, _texture);
         bubble->_sprite.setTextureRect(sprite_rects[std::rand() % sprite_rects.size()]);
         bubble->_sprite.setOrigin(12.0f, 12.0f);
         bubble->_delay_s = frand(0.0f, 0.3f);
         _bubbles.push_back(bubble);
      }
   }

   for (const auto& bubble : _bubbles)
   {
      bubble->_alive_s += dt.asSeconds();
      if (bubble->_alive_s < bubble->_delay_s)
      {
         continue;
      }

      bubble->_position += dt.asSeconds() * bubble->_velocity;
      bubble->_sprite.setPosition(bubble->_position);

      const auto atmosphere = Level::getCurrentLevel()->getAtmosphere().getTileForPosition(bubble->_position);
      if (atmosphere != AtmosphereTileWaterFull)
      {
         bubble->_pop = true;
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
