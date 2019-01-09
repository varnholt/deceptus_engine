#pragma once

#include "tmxelement.h"

// Qt
#include "SFML/Graphics.hpp"

struct TmxPolygon : TmxElement
{

  TmxPolygon() = default;
  void deserialize(tinyxml2::XMLElement* element);
  std::vector<sf::Vector2f> mPolyLine;
};

