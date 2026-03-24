#pragma once

#include <SFML/Graphics.hpp>

#include "hermitecurvekey.h"

///
/// \brief Evaluates Catmull-Rom style Hermite curves for position and orientation tracks.
///
class HermiteCurve
{
public:
   ///
   /// \brief Selects which key track to evaluate.
   ///
   enum class Mode
   {
      Position,
      Orientation
   };

///
/// \brief Constructs an empty curve with no keys.
///
   HermiteCurve() = default;

   ///
   /// \brief Destroys the curve.
   ///
   virtual ~HermiteCurve() = default;

   ///
   /// \brief Recomputes tangents for both key tracks.
   ///
   /// this must be called after changing keys and before sampling with computePoint().
   ///
   void compute();

   ///
   /// \brief Samples the selected curve track at normalized time.
   /// \param time Normalized time in the range [0, 1].
   /// \param mode Track to sample (position or orientation).
   /// \return Interpolated value at the requested time.
   ///
   sf::Vector2f computePoint(float time, Mode mode = Mode::Position);

///
/// \brief Replaces the position key track.
/// \param keys Position keys sorted by ascending key time.
///
   void setPositionKeys(const std::vector<HermiteCurveKey>& keys);

   ///
   /// \brief Replaces the orientation key track.
   /// \param keys Orientation keys sorted by ascending key time.
   ///
   void setOrientationKeys(const std::vector<HermiteCurveKey>& keys);

   ///
   /// \brief Returns the currently configured position keys.
   /// \return Position key track.
   ///
   const std::vector<HermiteCurveKey>& getPositionKeys() const;

   ///
   /// \brief Returns the currently configured orientation keys.
   /// \return Orientation key track.
   ///
   const std::vector<HermiteCurveKey>& getOrientationKeys() const;

   ///
   /// \brief Sets the current evaluated position value.
   /// \param position Position value to store.
   ///
   void setPosition(const sf::Vector2f& position);

   ///
   /// \brief Sets the current evaluated orientation value.
   /// \param orientation Orientation value to store.
   ///
   void setOrientation(const sf::Vector2f& orientation);

private:
   sf::Vector2f _position;
   sf::Vector2f _orientation;
   std::vector<HermiteCurveKey> _position_keys;
   std::vector<HermiteCurveKey> _orientation_keys;
   std::vector<sf::Vector2f> _position_tangents;
   std::vector<sf::Vector2f> _orientation_tangents;
};
