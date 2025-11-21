#include "splashscreen.h"
#include <cstdint>

namespace SplashScreen
{
void show(sf::RenderWindow& window)
{
   sf::Texture loading_texture;
   window.clear(sf::Color(30, 30, 30));
   if (loading_texture.loadFromFile("data/game/loading.png"))
   {
      sf::Sprite loading_sprite(loading_texture);

      const sf::Vector2u window_size = window.getSize();
      const sf::Vector2u texture_size = loading_texture.getSize();
      const auto scale_y = static_cast<float>(window_size.y) / texture_size.y;
      const auto scaled_width = texture_size.x * scale_y;
      loading_sprite.setScale({scale_y, scale_y});
      const auto position_x = (window_size.x - scaled_width) / 2.0f;
      loading_sprite.setPosition({position_x, 0.0f});

      // show the splash screen for a moment before fading
      window.draw(loading_sprite);
      window.display();

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1500ms);  // Show for 1.5 seconds before starting fade

      // Fade out effect - gradually reduce alpha from 255 to 0
      sf::Clock fade_clock;
      const float fade_duration = 500.0f;  // 0.5 seconds for fade out

      while (fade_clock.getElapsedTime().asMilliseconds() < fade_duration)
      {
          const float elapsed = fade_clock.getElapsedTime().asMilliseconds();
          const float alpha_ratio = 1.0f - (elapsed / fade_duration); // From 1.0 to 0.0
          const std::uint8_t alpha = static_cast<std::uint8_t>(255 * alpha_ratio);

          // update color with new alpha
          loading_sprite.setColor(sf::Color(255, 255, 255, alpha));

          window.clear(sf::Color(11, 12, 23));
          window.draw(loading_sprite);
          window.display();
      }

      // Final draw with 0 alpha to ensure complete fade out
      loading_sprite.setColor(sf::Color(255, 255, 255, 0));
      window.clear(sf::Color(11, 12, 23));
      window.draw(loading_sprite);
      window.display();
   }
}
}  // namespace SplashScreen
