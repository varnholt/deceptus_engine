#pragma once

#include <SFML/Graphics.hpp>

/// \brief stores a local hitbox rectangle and a translation offset.
struct Hitbox
{
   /// \brief creates a hitbox from a base rectangle and offset.
   /// \param rect rectangle in local pixel coordinates.
   /// \param offset translation applied when querying world-space bounds.
   Hitbox(const sf::FloatRect& rect, const sf::Vector2f& offset);
   /// \brief returns the rectangle translated by the stored offset.
   /// \return rectangle in pixel coordinates after adding _offset_px.
   sf::FloatRect getRectTranslated() const;

   sf::FloatRect _rect_px;
   sf::Vector2f _offset_px;
};
