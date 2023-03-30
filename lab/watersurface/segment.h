#ifndef SEGMENT_H
#define SEGMENT_H

struct Segment
{
   Segment(float height);

   void update(float dampening, float tension);
   void resetDeltas();

   float _height{0.0f};
   float _target_height{0.0f};
   float _velocity{0.0f};

   float _delta_left{0.0f};
   float _delta_right{0.0f};
};

#endif  // SEGMENT_H
