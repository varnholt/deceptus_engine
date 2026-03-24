#pragma once

#include "tmxelement.h"

#include "SFML/Graphics.hpp"

///
/// \brief Represents polyline geometry parsed from TMX object points.
///
struct TmxPolyLine : TmxElement
{
   ///
   /// \brief Constructs an empty polyline.
   ///
   TmxPolyLine() = default;

   ///
   /// \brief Parses polyline point pairs into `_path`.
   /// \param e XML element for `<polyline>`.
   /// \param parse_data Shared TMX parse context.
   ///
   void deserialize(tinyxml2::XMLElement* e, const std::shared_ptr<TmxParseData>& parse_data) override;

   std::vector<sf::Vector2f> _path;
};
