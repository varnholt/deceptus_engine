#pragma once

#include "tmxelement.h"

#include "SFML/Graphics.hpp"

struct TmxPolygon : TmxElement
{
   TmxPolygon() = default;
   void deserialize(tinyxml2::XMLElement* element, const std::shared_ptr<TmxParseData>&) override;
   std::vector<sf::Vector2f> _polyline;
};

