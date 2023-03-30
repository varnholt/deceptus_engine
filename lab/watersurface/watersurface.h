#ifndef WATERSURFACE_H
#define WATERSURFACE_H

#include <vector>

#include "segment.h"
#include "splashparticle.h"

class WaterSurface
{
public:
   WaterSurface();

   void splash(int32_t index, float velocity);
   void update(float dt);

   const std::vector<Segment>& getSegments();

private:
   float getSegmentHeight(float x);
   void createSplashParticles(float pos_x, float speed);
   std::vector<Segment> _segments;
   std::vector<SplashParticle> _particles;
};

#endif // WATERSURFACE_H
