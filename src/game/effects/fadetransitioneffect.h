#pragma once

#include <array>

#include "screentransitioneffect.h"

/// \brief screen transition effect that fades a solid color in or out over time.
struct FadeTransitionEffect : public ScreenTransitionEffect
{
   /// \brief selects whether alpha increases or decreases during the transition.
   enum class Direction
   {
      FadeIn,
      FadeOut
   };

   /// \brief creates a full-screen fade overlay with the given color.
   /// \param color overlay color used for the fade quad.
   FadeTransitionEffect(const sf::Color color = sf::Color::Black);

   /// \brief advances fade alpha, clamps to [0, 1], and finishes when target opacity is reached.
   /// \param dt elapsed frame time since the previous update.
   void update(const sf::Time& dt) override;

   /// \brief draws the fade quad using the current alpha value across the configured view size.
   /// \param window render texture that receives the fade overlay.
   void draw(const std::shared_ptr<sf::RenderTexture>& window) override;

   Direction _direction = Direction::FadeOut;
   sf::Color _fade_color = sf::Color::Black;
   float _value = 0.0f;
   float _speed = 1.0f;
   std::array<sf::Vertex, 4> _vertices;
};
