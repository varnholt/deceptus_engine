// header
#include "hermitecurve.h"

// stl
#include <cstdint>
#include <math.h>


//-----------------------------------------------------------------------------
void HermiteCurve::setPositionKeys(const std::vector<HermiteCurveKey>& keys)
{
   mPositionKeys = keys;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setOrientationKeys(const std::vector<HermiteCurveKey>& keys)
{
   mOrientationKeys = keys;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setPosition(const sf::Vector2f& position)
{
   mPosition = position;
}


//-----------------------------------------------------------------------------
void HermiteCurve::setOrientation(const sf::Vector2f& orientation)
{
   mOrientation = orientation;
}


//-----------------------------------------------------------------------------
const std::vector<HermiteCurveKey>& HermiteCurve::getPositionKeys() const
{
   return mPositionKeys;
}


//-----------------------------------------------------------------------------
const std::vector<HermiteCurveKey>& HermiteCurve::getOrientationKeys() const
{
   return mOrientationKeys;
}


//-----------------------------------------------------------------------------
void HermiteCurve::initialize()
{
   sf::Vector2f p1;
   sf::Vector2f p2;

   // calculate the tangents (catmull-rom splines)
   // Ti = 0.5 * (Pi + 1 - Pi - 1)

   std::vector<HermiteCurveKey>& source = mPositionKeys;
   std::vector<sf::Vector2f>& destination = mPositionTangents;

   for (auto pass = 0u; pass < 2; pass++)
   {
      if (pass == 1)
      {
          if (!mOrientationKeys.empty())
          {
             source = mOrientationKeys;
             destination = mOrientationTangents;
          }
      }

      if (source.empty())
      {
          continue;
      }

      for (auto i = 0u; i < mPositionKeys.size(); i++)
      {
         if (i == source.size() - 1)
         {
            p1 = source[0].mPosition;
            p2 = source[i - 1].mPosition;
         }
         else if (i == 0)
         {
            p1 = source[1].mPosition;
            p2 = source[source.size() - 1].mPosition;
         }
         else
         {
            p1 = source[i + 1].mPosition;
            p2 = source[i - 1].mPosition;
         }

         sf::Vector2f tangent;

         tangent.x = 0.5f * (p1.x - p2.x);
         tangent.y = 0.5f * (p1.y - p2.y);

         // tangent.z = 0.5f * (p1.z - p2.z);

         destination.push_back(tangent);
      }
   }
}


//-----------------------------------------------------------------------------
sf::Vector2f HermiteCurve::computePoint(float time, Mode mode)
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

   // init data to work on
   std::vector<HermiteCurveKey> keys;
   std::vector<sf::Vector2f> tangents;

   if (mode == Mode::Position)
   {
      keys = mPositionKeys;
      tangents = mPositionTangents;
   }
   else
   {
      keys = mOrientationKeys;
      tangents = mOrientationTangents;
   }

   // init sf::Vector2fs
   sf::Vector2f p;
   auto h1 = 0.0f;
   auto h2 = 0.0f;
   auto h3 = 0.0f;
   auto h4 = 0.0f;

   // find index
   auto index = 0;
   for (; index < static_cast<int32_t>(keys.size()); index++)
   {
      if (keys[index].mTime > time)
      {
         index--;
         break;
      }
   }

   if (index < 0)
      index = 0;

//   if (index == -1)
//   {
//      fprintf(stdout,"HermiteCurve::getCameraTrackPoint: bad data!\n");
//   }

   // init points
   sf::Vector2f p1;
   sf::Vector2f p2;
   auto p1Time = 0.0f;
   auto p2Time = 0.0f;

   if (index >= static_cast<int32_t>(keys.size()))
   {
      sf::Vector2f p = keys[keys.size() - 1].mPosition;
      return p;
   }

   p1 = keys[index].mPosition;
   p1Time = keys[index].mTime;

   p2 = keys[index + 1].mPosition;
   p2Time = keys[index + 1].mTime;

   // scale s to a value between 0 and 1
   const auto s = (time - p1Time) / (p2Time - p1Time);
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

   // p = p1 * h1 + p2 * h2 + t3 * h3 + t4 * h4
   p.x = p1.x * h1 + p2.x * h2 + t1.x * h3 + t2.x * h4;
   p.y = p1.y * h1 + p2.y * h2 + t1.y * h3 + t2.y * h4;

   return p;
}



