#include "splashscreen.h"
#include <cstdint>
#include "framework/easings/easings.h"

namespace
{
bool shown = false;
}

namespace SplashScreen
{
void show(sf::RenderWindow& window)
{
   if (shown)
   {
      return;
   }

   shown = true;

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

      // fade in
      sf::Clock fade_in_clock;
      constexpr auto fade_in_duration_ms = 1000.0f;
      while (fade_in_clock.getElapsedTime().asMilliseconds() < fade_in_duration_ms)
      {
         const float elapsed = fade_in_clock.getElapsedTime().asMilliseconds();
         const float normalized_ratio = elapsed / fade_in_duration_ms;
         const float eased_ratio = Easings::easeInOutQuad(normalized_ratio);
         const std::uint8_t alpha = static_cast<std::uint8_t>(255 * eased_ratio);
         loading_sprite.setColor(sf::Color(255, 255, 255, alpha));
         window.clear(sf::Color(11, 12, 23));
         window.draw(loading_sprite);
         window.display();
      }

      // Set fully opaque after fade-in is complete
      loading_sprite.setColor(sf::Color(255, 255, 255, 255));
      window.clear(sf::Color(11, 12, 23));
      window.draw(loading_sprite);
      window.display();

      // show the splash screen for a moment after fade in
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1000ms);

      // fade out
      sf::Clock fade_out_clock;
      constexpr auto fade_out_duration_ms = 1000.0f;
      while (fade_out_clock.getElapsedTime().asMilliseconds() < fade_out_duration_ms)
      {
         const float elapsed = fade_out_clock.getElapsedTime().asMilliseconds();
         const float normalized_ratio = 1.0f - (elapsed / fade_out_duration_ms);
         const float eased_ratio = Easings::easeInOutQuad(normalized_ratio);
         const std::uint8_t alpha = static_cast<std::uint8_t>(255 * eased_ratio);
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
