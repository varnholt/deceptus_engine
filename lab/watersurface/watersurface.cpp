#include "watersurface.h"

#include <math.h>
#include <algorithm>
#include <cmath>
#include <iostream>

#include "fakesfml.h"

namespace
{

float tension = 0.025f;
float dampening = 0.025f;
float spread = 0.25f;

static constexpr auto FACTOR_DEG_TO_RAD = 0.0174532925199432957f;

sf::Vector2f fromPolar(float angle, float magnitude)
{
   return sf::Vector2f(std::cos(angle), std::sin(angle)) * magnitude;
}

float frand(float min, float max)
{
   return std::rand() * (max - min) + min;
}

sf::Vector2f getRandomVector(float max_length)
{
   return fromPolar(frand(static_cast<float>(-M_PI), static_cast<float>(M_PI)), frand(0.0f, max_length));
}

}  // namespace

WaterSurface::WaterSurface()
{
   for (auto i = 0; i < 100; i++)
   {
      // the height can be refactored out
      _segments.push_back({0.0f});
   }
}

float WaterSurface::getSegmentHeight(float x)
{
   if (x < 0 || x > 800)
   {
      return 48.0f;
   }

   const auto scale = 1.0f;

   return _segments[(int32_t)(x * scale)]._height;
}

void WaterSurface::splash(int32_t index, float velocity)
{
   const auto start_index = std::max(0, index);
   const auto stop_index = std::min<size_t>(_segments.size() - 1, index + 1);

   for (auto i = start_index; i < stop_index; i++)
   {
      _segments[index]._velocity = velocity;
   }

   // createSplashParticles(pos_x, speed);
}

void WaterSurface::createSplashParticles(float pos_x, float velocity)
{
   float y = getSegmentHeight(pos_x);

   if (velocity <= 120.0f)
   {
      return;
   }

   for (auto i = 0; i < velocity / 8; i++)
   {
      const auto pos = sf::Vector2f{pos_x, y} + getRandomVector(40);
      const auto vel = fromPolar(FACTOR_DEG_TO_RAD * (frand(-150, -30)), frand(0, 0.5f * (float)std::sqrt(velocity)));
      _particles.push_back({pos, vel, 0});
   }
}

void WaterSurface::update(float dt)
{
   constexpr auto animation_speed = 10.0f;

   for (auto& segment : _segments)
   {
      segment.update(dampening, tension);
      segment.resetDeltas();
   }

   // todo, incorporate dt

   static constexpr auto integration_steps = 8;

   // integrate a few times
   for (auto j = 0; j < integration_steps; j++)
   {
      for (auto segment_index = 0; segment_index < _segments.size(); segment_index++)
      {
         if (segment_index > 0)
         {
            const auto delta_left =
               spread * (_segments[segment_index]._height - _segments[segment_index - 1]._height) * dt * animation_speed;

            _segments[segment_index]._delta_left = delta_left;
            _segments[segment_index - 1]._velocity += delta_left;
         }

         if (segment_index < _segments.size() - 1)
         {
            const auto delta_right =
               spread * (_segments[segment_index]._height - _segments[segment_index + 1]._height) * dt * animation_speed;

            _segments[segment_index]._delta_right = delta_right;
            _segments[segment_index + 1]._velocity += delta_right;
         }
      }

      // update heights based on deltas
      for (auto segment_index = 0; segment_index < _segments.size(); segment_index++)
      {
         if (segment_index > 0)
         {
            _segments[segment_index - 1]._height += _segments[segment_index]._delta_left;
         }

         if (segment_index < _segments.size() - 1)
         {
            _segments[segment_index + 1]._height += _segments[segment_index]._delta_right;
         }
      }
   }

   // for (auto& particle : _particles)
   // {
   //    particle.update();
   // }

   //   for (const auto& segment : _segments)
   //   {
   //      std::cout << " " << segment._height;
   //   }
   //   std::cout << std::endl;
}

const std::vector<Segment>& WaterSurface::getSegments()
{
   return _segments;
}
