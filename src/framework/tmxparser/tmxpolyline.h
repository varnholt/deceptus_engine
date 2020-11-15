#pragma once

#include "tmxelement.h"

#include "SFML/Graphics.hpp"

struct TmxPolyLine : TmxElement
{
   TmxPolyLine() = default;

   void deserialize(tinyxml2::XMLElement* e) override;

   std::vector<sf::Vector2f> mPolyLine;
};

