#include "splashscreen.h"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>
#include "framework/easings/easings.h"
#include "framework/tools/log.h"

namespace
{
bool shown = false;
}

namespace SplashScreen
{

void eatEvents(sf::RenderWindow& window)
{
   const std::vector<std::function<bool(const sf::Event&)>> ignoredEventChecks = {
      // keyboard events
      [](const sf::Event& evt) { return evt.is<sf::Event::KeyPressed>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::KeyReleased>(); },
      // mouse events
      [](const sf::Event& evt) { return evt.is<sf::Event::MouseButtonPressed>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::MouseButtonReleased>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::MouseMoved>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::MouseWheelScrolled>(); },
      // touch events
      [](const sf::Event& evt) { return evt.is<sf::Event::TouchBegan>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::TouchMoved>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::TouchEnded>(); },
      // joystick events
      [](const sf::Event& evt) { return evt.is<sf::Event::JoystickButtonPressed>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::JoystickButtonReleased>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::JoystickMoved>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::JoystickConnected>(); },
      [](const sf::Event& evt) { return evt.is<sf::Event::JoystickDisconnected>(); }
   };

   auto isIgnoredEvent = [&ignoredEventChecks](const sf::Event& evt) -> bool
   {
      return std::ranges::find_if(
                ignoredEventChecks.begin(),
                ignoredEventChecks.end(),
                [&evt](const std::function<bool(const sf::Event&)>& check) { return check(evt); }
             ) != ignoredEventChecks.end();
   };

   while (auto event = window.pollEvent())
   {
      if (!isIgnoredEvent(*event))
      {
         auto getEventTypeName = [](const sf::Event& evt) -> std::optional<std::string>
         {
            if (evt.is<sf::Event::Closed>())
            {
               return "Closed";
            }
            if (evt.is<sf::Event::Resized>())
            {
               return "Resized";
            }
            if (evt.is<sf::Event::TextEntered>())
            {
               return "TextEntered";
            }
            if (evt.is<sf::Event::KeyPressed>())
            {
               return "KeyPressed";
            }
            if (evt.is<sf::Event::KeyReleased>())
            {
               return "KeyReleased";
            }
            if (evt.is<sf::Event::MouseWheelScrolled>())
            {
               return "MouseWheelScrolled";
            }
            if (evt.is<sf::Event::MouseButtonPressed>())
            {
               return "MouseButtonPressed";
            }
            if (evt.is<sf::Event::MouseButtonReleased>())
            {
               return "MouseButtonReleased";
            }
            if (evt.is<sf::Event::MouseMoved>())
            {
               return "MouseMoved";
            }
            if (evt.is<sf::Event::JoystickButtonPressed>())
            {
               return "JoystickButtonPressed";
            }
            if (evt.is<sf::Event::JoystickButtonReleased>())
            {
               return "JoystickButtonReleased";
            }
            if (evt.is<sf::Event::JoystickMoved>())
            {
               return "JoystickMoved";
            }
            if (evt.is<sf::Event::JoystickConnected>())
            {
               return "JoystickConnected";
            }
            if (evt.is<sf::Event::JoystickDisconnected>())
            {
               return "JoystickDisconnected";
            }
            if (evt.is<sf::Event::TouchBegan>())
            {
               return "TouchBegan";
            }
            if (evt.is<sf::Event::TouchMoved>())
            {
               return "TouchMoved";
            }
            if (evt.is<sf::Event::TouchEnded>())
            {
               return "TouchEnded";
            }
            if (evt.is<sf::Event::SensorChanged>())
            {
               return "SensorChanged";
            }
            return std::nullopt;
         };

         if (auto event_name = getEventTypeName(*event); event_name.has_value())
         {
            Log::Info() << "event ignored: " << event_name.value();
         }
      }
   }
}

void show(sf::RenderWindow& window)
{
   if (shown)
   {
      return;
   }

   shown = true;

   sf::Texture loading_texture;
   window.clear(sf::Color(30, 30, 30));
   if (loading_texture.loadFromFile("data/game/splash.png"))
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

   eatEvents(window);
}
}  // namespace SplashScreen
