#pragma once

#include "tmxelement.h"

// Qt
#include "SFML/Graphics.hpp"


struct TmxPolyLine : TmxElement
{
   TmxPolyLine();

   void deserialize(tinyxml2::XMLElement* e) override;

   std::vector<sf::Vector2f> mPolyLine;
};

