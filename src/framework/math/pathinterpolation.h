#pragma once

#include <vector>
#include <Box2D/Box2D.h>

template <typename T>
class PathInterpolation
{

public:

   enum class Mode
   {
      Linear
   };

   struct Key
   {
      T _pos;
      float _time_value = 0.0f;
   };

   PathInterpolation() = default;

   void addKeys(const std::vector<T>& positions)
   {
      auto time = 0.0f;
      const auto time_increment = 1.0f / static_cast<float>(positions.size());
      for (auto& pos : positions)
      {
         addKey(pos, time);
         time += time_increment;
      }
   }

   void addKey(const T &pos, float time_value)
   {
      Key key;
      key._pos = pos;
      key._time_value = time_value;

      _track.push_back(key);
   }

   T computeVelocity(const T& current, float time_value)
   {
      time_value = std::clamp(time_value, 0.0f, 1.0f);

      T velocity;

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

   T computePosition(float time_value)
   {
      time_value = std::clamp(time_value, 0.0f, 1.0f);

      T position;

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

   float updateTime(float delta)
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

   float updateTimeLooped(float delta)
   {
      if (_time >= 1.0f)
      {
          _time = 0.0f;
      }

      _time += delta;

      return _time;
   }

   bool update(const T& current_pos)
   {
      if (_track.empty())
      {
          return false;
      }

      auto reached = false;

      // just check whether the speed needs to be updated (i.e. if one of the keys has been reached)
      if (checkKeyReached(current_pos) || !_initialized)
      {
         _initialized = true;
         _current_key_index = nextKeyIndex();
         computeVelocity();
         reached = true;
      }

      return reached;
   }

   const T getVelocity()
   {
      return _velocity;
   }

   const std::vector<PathInterpolation::Key>& getTrack() const
   {
      return _track;
   }

   float getTime() const
   {
       return _time;
   }


private:

   void computeVelocity()
   {
      const T& a = _track[_current_key_index]._pos;
      const T& b = _track[nextKeyIndex()]._pos;

      _velocity = (a - b);
      _velocity.Normalize();
   }

   size_t nextKeyIndex()
   {
      auto next_index = _current_key_index + 1;
      if (next_index == _track.size())
      {
        next_index = 0;
      }

      return next_index;
   }

   bool checkKeyReached(const T& current_pos)
   {
     if (_track.empty())
     {
        return false;
     }

     auto reached = false;

     if ((current_pos - _track[_current_key_index]._pos).LengthSquared() < 0.1f)
     {
        reached = true;
     }

     return reached;
   }

   Mode _mode = Mode::Linear;
   std::vector<Key> _track;
   float _time = 0.0f;
   bool _up = true;
   bool _initialized = false;

   T _velocity;
   size_t _current_key_index = 0;
};

