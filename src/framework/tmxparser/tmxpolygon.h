#pragma once

#include "tmxelement.h"

#include "SFML/Graphics.hpp"

///
/// \brief Represents polygon geometry parsed from TMX object points.
///
struct TmxPolygon : TmxElement
{
   ///
   /// \brief Constructs an empty polygon.
   ///
   TmxPolygon() = default;
   ///
   /// \brief Parses polygon point pairs into `_polyline`.
   /// \param element XML element for `<polygon>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>& parse_data) override;
   std::vector<sf::Vector2f> _polyline;
};
