#include "watersurface.h"

#include <math.h>
#include <algorithm>
#include <cmath>

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
   return fromPolar(frand(-M_PI, M_PI), frand(0, max_length));
}

}  // namespace

WaterSurface::WaterSurface()
{
   for (auto i = 0; i < 200; i++)
   {
      _segments.push_back({48.0f});
   }
}

float WaterSurface::getSegmentHeight(float x)
{
   if (x < 0 || x > 800)
   {
      return 48.0f;
   }

   const auto scale = 1.0f;

   return _segments[(int)(x * scale)]._height;
}

void WaterSurface::splash(float pos_x, float velocity)
{
   const auto scale = 1.0f;
   const auto index = std::clamp<int32_t>(pos_x * scale, 0, _segments.size() - 1);

   for (auto i = std::max(0, index - 0); i < std::min<size_t>(_segments.size() - 1, index + 1); i++)
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

void WaterSurface::update()
{
   for (auto i = 0; i < _segments.size(); i++)
   {
      _segments[i].Update(dampening, tension);
   }

   for (auto j = 0; j < 8; j++)
   {
      for (auto i = 0; i < _segments.size(); i++)
      {
         if (i > 0)
         {
            const auto delta_left = spread * (_segments[i]._height - _segments[i - 1]._height);
            _segments[i].delta_left = delta_left;
            _segments[i - 1]._velocity += delta_left;
         }

         if (i < _segments.size() - 1)
         {
            const auto delta_right = spread * (_segments[i]._height - _segments[i + 1]._height);
            _segments[i].delta_right = delta_right;
            _segments[i + 1]._velocity += delta_right;
         }
      }

      for (auto i = 0; i < _segments.size(); i++)
      {
         if (i > 0)
            _segments[i - 1]._height += _segments[i].delta_left;
         if (i < _segments.size() - 1)
            _segments[i + 1]._height += _segments[i].delta_right;
      }
   }

   for (auto& particle : _particles)
   {
      particle.update();
   }
}
