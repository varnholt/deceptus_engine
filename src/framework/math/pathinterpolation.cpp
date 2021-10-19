#include "pathinterpolation.h"


// const float checkRadius = 0.5f;


void PathInterpolation::addKey(const b2Vec2& pos, float timeValue)
{
   Key key;
   key._pos = pos;
   key._time_value = timeValue;

   _track.push_back(key);
}


b2Vec2 PathInterpolation::computeVelocity(const b2Vec2& current, float time_value)
{
   // clamp time
   if (time_value > 1.0f)
   {
      time_value = 1.0f;
   }
   else if (time_value < 0.0f)
   {
      time_value = 0.0f;
   }

   b2Vec2 velocity;

   switch (_mode)
   {
      case Mode::Linear:
      {
         for (auto index_current = 0u; index_current < _track.size(); index_current++)
         {
            size_t index_next = index_current + 1;
            if (index_next == _track.size())
            {
               index_next = _track.size() - 1;
            }

            const auto& key_a = _track.at(index_current);
            const auto& key_b = _track.at(index_next);

            if (time_value >= key_a._time_value && time_value < key_b._time_value)
            {
               auto a = 1.0f - (time_value - key_a._time_value);
               auto b = 1.0f - (key_b._time_value - time_value);

               velocity = (a * key_a._pos + b * key_b._pos) - current;
               // printf("a: %f, b: %f, current: %f, %f, velocity: %f, %f\n", a, b, current.x, current.y, velocity.x, velocity.y);
               break;
            }
         }
         break;
      }
   }

   return velocity;
}


b2Vec2 PathInterpolation::computePosition(float time_value)
{
   // clamp time
   if (time_value > 1.0f)
   {
      time_value = 1.0f;
   }
   else if (time_value < 0.0f)
   {
      time_value = 0.0f;
   }

   b2Vec2 position;

   switch (_mode)
   {
      case Mode::Linear:
      {
         for (auto index_current = 0u; index_current < _track.size(); index_current++)
         {
            size_t index_next = index_current + 1;
            if (index_next == _track.size())
            {
               index_next = _track.size() - 1;
            }

            const auto& key_a = _track.at(index_current);
            const auto& key_b = _track.at(index_next);

            if (time_value >= key_a._time_value && time_value < key_b._time_value)
            {
               auto a = 1.0f - (time_value - key_a._time_value);
               auto b = 1.0f - (key_b._time_value - time_value);

               position = (a * key_a._pos + b * key_b._pos);
               break;
            }
         }

         break;
      }
   }

   return position;
}


float PathInterpolation::updateZeroOneZeroOne(float delta)
{
   if (_time >= 1.0f)
   {
      _time = 1.0f;
      _up = false;
   }
   else if (_time <= 0.0f)
   {
      _time = 0.0f;
      _up = true;
   }

   if (_up)
   {
      _time += delta;
   }
   else
   {
      _time -= delta;
   }

   return _time;
}



bool PathInterpolation::checkKeyReached(const b2Vec2& current_pos)
{
  auto reached = false;

  if (_track.empty())
  {
     return false;
  }

  if ((current_pos - _track[_current_key_index]._pos).LengthSquared() < 0.1f)
  {
     reached = true;
  }

  return reached;
}


const std::vector<PathInterpolation::Key>& PathInterpolation::getTrack() const
{
   return _track;
}


bool PathInterpolation::update(const b2Vec2& current_pos)
{
   if (_track.empty())
   {
       return false;
   }

   auto reached = false;

   // just check whether the speed needs to be updated (i.e. if one of the keys has been reached)
   if (checkKeyReached(current_pos) || !_velocity.IsValid()) // NOT CALLED
   {
      _current_key_index = nextKeyIndex();
      computeVelocity();
      reached = true;
   }

   return reached;
}


void PathInterpolation::computeVelocity()
{
   b2Vec2 a = _track[_current_key_index]._pos;
   b2Vec2 b = _track[nextKeyIndex()]._pos;

   _velocity = (a - b);
   _velocity.Normalize();
}


const b2Vec2 PathInterpolation::getVelocity()
{
   return _velocity;
}


size_t PathInterpolation::nextKeyIndex()
{
   auto next_index = _current_key_index + 1;
   if (next_index == _track.size())
   {
     next_index = 0;
   }

   return next_index;
}


