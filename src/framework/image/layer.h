#pragma once

#include <SFML/Graphics.hpp>
#include <memory>

///
/// \brief Encapsulates rendering behavior for layer.
///

struct Layer : public sf::Drawable
{
   ///
   /// \brief Initializes object state for this type.
   ///
   Layer() = default;

///

/// \brief Renders this object to the target render surface.

/// \param target Render target.

/// \param states States.

///

   void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

///

/// \brief Shows the associated view.

///

   void show();
   ///
   /// \brief Hides the associated view.
   ///
   void hide();

   bool _visible = true;

   std::string _name;
   std::shared_ptr<sf::Texture> _texture;
   std::shared_ptr<sf::Sprite> _sprite;
};
