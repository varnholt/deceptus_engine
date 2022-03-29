// header
#include "hermitecurve.h"

// stl
#include <cstdint>
#include <math.h>


namespace
{
static const auto clamp = true;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setPositionKeys(const std::vector<HermiteCurveKey>& keys)
{
   _position_keys = keys;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setOrientationKeys(const std::vector<HermiteCurveKey>& keys)
{
   _orientation_keys = keys;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setPosition(const sf::Vector2f& position)
{
   _position = position;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setOrientation(const sf::Vector2f& orientation)
{
   _orientation = orientation;
}


//-----------------------------------------------------------------------------
const std::vector<HermiteCurveKey>& HermiteCurve::getPositionKeys() const
{
   return _position_keys;
}


//-----------------------------------------------------------------------------
const std::vector<HermiteCurveKey>& HermiteCurve::getOrientationKeys() const
{
   return _orientation_keys;
}


//-----------------------------------------------------------------------------
void HermiteCurve::compute()
{
   // calculate the tangents (catmull-rom splines)
   // Ti = 0.5 * (Pi + 1 - Pi - 1)

   auto comp = [](std::vector<HermiteCurveKey>& source, std::vector<sf::Vector2f>& destination)
   {
      if (source.empty())
      {
          return;
      }

      sf::Vector2f p1;
      sf::Vector2f p2;
      for (auto i = 0u; i < source.size(); i++)
      {
         if (i == 0)
         {
            p1 = source[        0                      ]._position;
            p2 = source[clamp ? 0 : (source.size() - 1)]._position;
         }
         else if (i == source.size() - 1)
         {
            p1 = source[clamp ? (i - 1) : 0]._position;
            p2 = source[         i - 1     ]._position;
         }
         else
         {
            p1 = source[i + 1]._position;
            p2 = source[i - 1]._position;
         }

         sf::Vector2f tangent;

         tangent.x = 0.5f * (p1.x - p2.x);
         tangent.y = 0.5f * (p1.y - p2.y);

         // tangent.z = 0.5f * (p1.z - p2.z);

         destination.push_back(tangent);
      }
   };

   comp(_position_keys, _position_tangents);
   comp(_orientation_keys, _orientation_tangents);
}


//-----------------------------------------------------------------------------
sf::Vector2f HermiteCurve::computePoint(float time, Mode mode)
{
   if (clamp)
   {
      if (time > 1.0f)
      {
         time = 1.0f;
      }
      else if (time < 0.0f)
      {
         time = 0.0f;
      }
   }
   else
   {
      // scale time to 0..1
      if (time >= 1.0f)
      {
         time -= floor(time);
      }
      else if (time < 0.0f)
      {
         // -0.7 => 0.3
         time = -time;
         time -= floor(time);
         time = 1.0f - time;
      }
   }

   // init data to work on
   std::vector<HermiteCurveKey> keys;
   std::vector<sf::Vector2f> tangents;

   if (mode == Mode::Position)
   {
      keys = _position_keys;
      tangents = _position_tangents;
   }
   else
   {
      keys = _orientation_keys;
      tangents = _orientation_tangents;
   }

   sf::Vector2f p;
   auto h1 = 0.0f;
   auto h2 = 0.0f;
   auto h3 = 0.0f;
   auto h4 = 0.0f;

   // find index
   auto index = 0;
   for (; index < static_cast<int32_t>(keys.size()); index++)
   {
      if (keys[index]._time > time)
      {
         index--;
         break;
      }
   }

   if (index < 0)
      index = 0;

   // init points
   sf::Vector2f p1;
   sf::Vector2f p2;
   auto p1_time = 0.0f;
   auto p2_time = 0.0f;

   if (index >= static_cast<int32_t>(keys.size()))
   {
      sf::Vector2f p = keys[keys.size() - 1]._position;
      return p;
   }

   p1 = keys[index]._position;
   p1_time = keys[index]._time;

   p2 = keys[index + 1]._position;
   p2_time = keys[index + 1]._time;

   // scale s to a value between 0 and 1
   const auto s = (time - p1_time) / (p2_time - p1_time);
   const auto s2 = s * s;
   const auto s3 = s2 * s;

   // init tangents
   auto t1 = tangents[index];
   auto t2 = tangents[index + 1];

   // calculate base functions 1-4
   h1 =  2.0f * s3 - 3.0f * s2 + 1.0f;
   h2 = -2.0f * s3 + 3.0f * s2;
   h3 =         s3 - 2.0f * s2 + s;
   h4 =         s3 -        s2;

   p.x = p1.x * h1 + p2.x * h2 + t1.x * h3 + t2.x * h4;
   p.y = p1.y * h1 + p2.y * h2 + t1.y * h3 + t2.y * h4;

   return p;
}



