#include "segment.h"

Segment::Segment(float height) : _height(height), _target_height(height)
{
}

void Segment::update(float dampening, float tension)
{
   const auto x = _target_height - _height;
   _velocity += tension * x - _velocity * dampening;
   _height += _velocity;
}

void Segment::resetDeltas()
{
   _delta_left = 0.0f;
   _delta_right = 0.0f;
}
