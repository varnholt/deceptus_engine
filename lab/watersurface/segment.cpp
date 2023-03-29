#include "segment.h"

Segment::Segment(float height) : _height(height), _target_height(height)
{
}

void Segment::Update(float dampening, float tension)
{
   const auto x = _target_height - _height;
   _velocity += tension * x - _velocity * dampening;
   _height += _velocity;
}
