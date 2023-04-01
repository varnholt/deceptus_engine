#include "watersurface.h"

namespace
{

float tension = 0.025f;
float dampening = 0.025f;
float spread = 0.25f;

}  // namespace

void WaterSurface::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   auto index = 0;
   const auto segment_width = _bounding_box.width / _segments.size();
   std::vector<sf::Vertex> sf_lines;
   for (const auto& segment : _segments)
   {
      const auto x = static_cast<float>(index * segment_width);
      const auto y = segment._height;
      sf_lines.push_back(sf::Vertex{sf::Vector2f{x, y}, sf::Color::White});
      index++;
   }

   color.draw(sf_lines.data(), sf_lines.size(), sf::LineStrip);
}

void WaterSurface::update(const sf::Time& dt)
{
   constexpr auto animation_speed = 10.0f;

   for (auto& segment : _segments)
   {
      segment.update(dampening, tension);
      segment.resetDeltas();
   }

   static constexpr auto integration_steps = 8;

   // integrate a few times
   for (auto j = 0; j < integration_steps; j++)
   {
      for (auto segment_index = 0; segment_index < _segments.size(); segment_index++)
      {
         if (segment_index > 0)
         {
            const auto delta_left =
               spread * (_segments[segment_index]._height - _segments[segment_index - 1]._height) * dt.asSeconds() * animation_speed;

            _segments[segment_index]._delta_left = delta_left;
            _segments[segment_index - 1]._velocity += delta_left;
         }

         if (segment_index < _segments.size() - 1)
         {
            const auto delta_right =
               spread * (_segments[segment_index]._height - _segments[segment_index + 1]._height) * dt.asSeconds() * animation_speed;

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
}

std::optional<sf::FloatRect> WaterSurface::getBoundingBoxPx()
{
   return _bounding_box;
}

void WaterSurface::setup(const GameDeserializeData& data)
{
}

void WaterSurface::Segment::update(float dampening, float tension)
{
   const auto x = _target_height - _height;
   _velocity += tension * x - _velocity * dampening;
   _height += _velocity;
}

void WaterSurface::Segment::resetDeltas()
{
   _delta_left = 0.0f;
   _delta_right = 0.0f;
}
