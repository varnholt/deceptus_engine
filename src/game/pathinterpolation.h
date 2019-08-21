#ifndef PATHINTERPOLATION_H
#define PATHINTERPOLATION_H

#include <vector>
#include <Box2D/Box2D.h>

class PathInterpolation
{

public:

   enum class Mode
   {
      Linear
   };

   struct Key
   {
      b2Vec2 mPos;
      float mTimeValue = 0.0f;
   };


private:

   Mode mMode = Mode::Linear;
   std::vector<Key> mTrack;
   float mTime = 0.0f;
   bool mUp = true;

   b2Vec2 mVelocity;
   size_t mCurrentKeyIndex = 0;


public:

   PathInterpolation() = default;
   void addKey(const b2Vec2 &pos, float timeValue);

   b2Vec2 compute(const b2Vec2 &current, float timeValue);
   float updateZeroOneZeroOne(float delta);

   bool update(const b2Vec2 &currentPos);
   const b2Vec2 getVelocity();


private:

   void computeVelocity();

   size_t nextKeyIndex();
   bool checkKeyReached(const b2Vec2& currentPos);
};

#endif // PATHINTERPOLATION_H
