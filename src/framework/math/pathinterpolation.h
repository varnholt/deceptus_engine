#pragma once

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
      b2Vec2 _pos;
      float _time_value = 0.0f;
   };

   PathInterpolation() = default;
   void addKey(const b2Vec2 &pos, float time_value);

   b2Vec2 computeVelocity(const b2Vec2 &current, float time_value);
   b2Vec2 computePosition(float time_value);

   float updateZeroOneZeroOne(float delta);

   bool update(const b2Vec2 &currentPos);
   const b2Vec2 getVelocity();

   const std::vector<Key>& getTrack() const;


private:

   void computeVelocity();

   size_t nextKeyIndex();
   bool checkKeyReached(const b2Vec2& currentPos);

   Mode _mode = Mode::Linear;
   std::vector<Key> _track;
   float _time = 0.0f;
   bool _up = true;
   bool _initialized = false;

   b2Vec2 _velocity;
   size_t _current_key_index = 0;
};

