#pragma once

#include <SFML/Graphics.hpp>

#include "hermitecurvekey.h"


class HermiteCurve
{

public:

   enum class Mode
   {
      Position,
      Orientation
   };

   HermiteCurve() = default;
   virtual ~HermiteCurve() = default;
   void compute();
   sf::Vector2f computePoint(float time, Mode mode = Mode::Position);

   void setPositionKeys(const std::vector<HermiteCurveKey>&);
   void setOrientationKeys(const std::vector<HermiteCurveKey>&);
   const std::vector<HermiteCurveKey>& getPositionKeys() const;
   const std::vector<HermiteCurveKey>& getOrientationKeys() const;
   void setPosition(const sf::Vector2f&);
   void setOrientation(const sf::Vector2f&);


private:

   sf::Vector2f _position;
   sf::Vector2f _orientation;
   std::vector<HermiteCurveKey> _position_keys;
   std::vector<HermiteCurveKey> _orientation_keys;
   std::vector<sf::Vector2f> _position_tangents;
   std::vector<sf::Vector2f> _orientation_tangents;

};

