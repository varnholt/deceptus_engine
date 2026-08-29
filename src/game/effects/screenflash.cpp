#include "screenflash.h"

#include "framework/easings/easings.h"
#include "game/config/gameconfiguration.h"

#ifdef DECEPTUS_VRSFML
#include <span>
#endif

ScreenFlash& ScreenFlash::getInstance()
{
   static ScreenFlash instance;
   return instance;
}

void ScreenFlash::flash(const sf::Color& color, float peak_intensity, float duration_s)
{
   _color = color;
   _peak_intensity = peak_intensity;
   _duration_s = duration_s;
   _elapsed_s = 0.0f;
   _intensity = peak_intensity;
}

void ScreenFlash::update(const sf::Time& dt)
{
   if (_duration_s <= 0.0f)
   {
      return;
   }

   _elapsed_s += dt.asSeconds();

   if (_elapsed_s >= _duration_s)
   {
      _duration_s = 0.0f;
      _intensity = 0.0f;
      return;
   }

   // an impact flash is at full strength on the frame it is triggered and then drops away quickly,
   // so the fade-out eases out rather than running down linearly
   _intensity = _peak_intensity * (1.0f - Easings::easeOutCubic<float>(_elapsed_s / _duration_s));
}

void ScreenFlash::draw(const std::shared_ptr<sf::RenderTexture>& target)
{
   if (_intensity <= 0.0f)
   {
      return;
   }

   const auto width = static_cast<float>(GameConfiguration::getInstance()._view_width);
   const auto height = static_cast<float>(GameConfiguration::getInstance()._view_height);

   auto color = _color;
   color.a = static_cast<uint8_t>(_intensity * 255.0f);

   const std::array<sf::Vertex, 4> vertices{
      sf::Vertex{{0.0f, 0.0f}, color},
      sf::Vertex{{0.0f, height}, color},
      sf::Vertex{{width, 0.0f}, color},
      sf::Vertex{{width, height}, color}
   };

#ifdef DECEPTUS_VRSFML
   const sf::View view = sf::View::fromRect(sf::FloatRect{{0.0f, 0.0f}, {width, height}});
   target->draw(
      std::span<const sf::Vertex>{vertices.data(), vertices.size()}, sf::PrimitiveType::TriangleStrip, sf::RenderStates{.view = view}
   );
#else
   const sf::View view(sf::FloatRect({0.0f, 0.0f}, {width, height}));
   target->setView(view);
   target->draw(vertices.data(), vertices.size(), sf::PrimitiveType::TriangleStrip);
#endif
}
