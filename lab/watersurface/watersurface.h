#ifndef WATERSURFACE_H
#define WATERSURFACE_H

#include <vector>

#include "fakesfml.h"
#include "segment.h"
#include "splashparticle.h"

class WaterSurface
{
public:
   WaterSurface();

   void splash(float pos_x, float velocity);
   void update();

private:
   float getSegmentHeight(float x);
   void createSplashParticles(float pos_x, float speed);
   std::vector<Segment> _segments;
   std::vector<SplashParticle> _particles;
};

#endif // WATERSURFACE_H
