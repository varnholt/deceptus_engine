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

      sf::Vector2f mPosition;
      sf::Vector2f mOrientation;
      std::vector<HermiteCurveKey> mPositionKeys;
      std::vector<HermiteCurveKey> mOrientationKeys;
      std::vector<sf::Vector2f> mPositionTangents;
      std::vector<sf::Vector2f> mOrientationTangents;
};

