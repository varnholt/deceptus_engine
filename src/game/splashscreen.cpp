#include "splashscreen.h"

namespace SplashScreen
{
void show(sf::RenderWindow& window)
{
   sf::Texture loading_texture;
   window.clear(sf::Color(30, 30, 30));
   if (loading_texture.loadFromFile("data/game/loading.png"))
   {
      sf::Sprite loading_sprite(loading_texture);

      sf::Vector2u window_size = window.getSize();
      sf::Vector2u texture_size = loading_texture.getSize();
      const auto scale_y = static_cast<float>(window_size.y) / texture_size.y;
      const auto scaled_width = texture_size.x * scale_y;
      loading_sprite.setScale({scale_y, scale_y});
      const auto position_x = (window_size.x - scaled_width) / 2.0f;
      loading_sprite.setPosition({position_x, 0.0f});

      window.draw(loading_sprite);
      window.display();

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(2s);
   }
}
}  // namespace SplashScreen
