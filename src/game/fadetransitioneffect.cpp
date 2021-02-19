#include "fadetransitioneffect.h"

#include "gameconfiguration.h"


FadeTransitionEffect::FadeTransitionEffect()
{
   const auto w = static_cast<float>(GameConfiguration::getInstance().mViewWidth);
   const auto h = static_cast<float>(GameConfiguration::getInstance().mViewHeight);

   _vertices = {
      sf::Vector2f{0, 0}, sf::Vector2f{0, h}, sf::Vector2f{w, h}, sf::Vector2f{w, 0}
   };

//   triangle[0].color = sf::Color::Red;
//   triangle[1].color = sf::Color::Blue;
//   triangle[2].color = sf::Color::Green;
}


void FadeTransitionEffect::update(const sf::Time& dt)
{
   float sign = 1.0f;

   switch (_direction)
   {
      case Direction::FadeIn:
         sign = -1.0f;
         break;
      case Direction::FadeOut:
         sign = 1.0f;
         break;
   }

   _value = _value + (_speed * sign * dt.asSeconds());
}


void FadeTransitionEffect::draw(const std::shared_ptr<sf::RenderTexture>& window)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   window->setView(view);

   window->draw(_vertices.data(), _vertices.size(), sf::Quads);
}
