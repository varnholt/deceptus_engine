#pragma once

#include "box2d/box2d.h"
#include <functional>
#include <vector>
#include "framework/easings/easings.h"
#include "framework/math/sfmlmath.h"
#include "framework/tools/log.h"

template <typename T>
///
/// \brief Builds a normalized key track and evaluates interpolated path values over time.
///
class PathInterpolation
{
public:
   ///
   /// \brief One interpolation key containing a position and normalized time value.
   ///
   struct Key
   {
      T _pos;
      float _time_value = 0.0f;
   };

   PathInterpolation() = default;

   ///
   /// \brief Adds keys and distributes their time values by cumulative path length.
   /// \param positions Path points in traversal order.
   ///
   void addKeys(const std::vector<T>& positions)
   {
      auto length_sum = 0.0f;
      for (auto index = 0; index < static_cast<int32_t>(positions.size()); index++)
      {
         const auto length = (index == 0) ? 0.0f : SfmlMath::length(positions[index] - positions[index - 1]);
         length_sum += length;
      }

      auto length_to_this_point = 0.0f;
      for (auto index = 0; index < static_cast<int32_t>(positions.size()); index++)
      {
         const auto& pos = positions.at(index);
         const auto length = (index == 0) ? 0.0f : SfmlMath::length(positions[index] - positions[index - 1]);
         length_to_this_point += length;
         addKey(pos, length_to_this_point / length_sum);
      }
   }

   ///
   /// \brief Linearly interpolates from `pos1` to `pos2`.
   /// \param pos1 Start value.
   /// \param pos2 End value.
   /// \param weight_pos_2 Weight applied to `pos2`.
   /// \return Interpolated value.
   ///
   T lerp(const T& pos1, const T& pos2, float weight_pos_2)
   {
      return pos1 + weight_pos_2 * (pos2 - pos1);
   }

   ///
   /// \brief Adds eased subdivision keys for each segment and normalizes times by path length.
   /// \param positions Path points in traversal order.
   /// \param subdivision_count Number of interpolation keys inserted per segment.
   /// \param easing_type Easing function used inside each segment.
   ///
   void addKeys(const std::vector<T>& positions, int32_t subdivision_count, Easings::Type easing_type)
   {
      auto func = Easings::getFunction<float>(easing_type);

      auto length_sum = 0.0f;
      for (auto index = 0; index < static_cast<int32_t>(positions.size()); index++)
      {
         const auto length = (index == 0) ? 0.0f : SfmlMath::length(positions[index] - positions[index - 1]);
         length_sum += length;
      }

      auto length_to_this_point = 0.0f;
      for (auto index = 0; index < static_cast<int32_t>(positions.size()) - 1; index++)
      {
         const auto& pos_1 = positions.at(index);
         const auto& pos_2 = positions.at(index + 1);
         const auto length = SfmlMath::length(positions[index + 1] - positions[index]);
         const auto subdiv_step = length / subdivision_count;
         auto subdiv_dist = length_to_this_point;

         for (auto subdivision = 0; subdivision < subdivision_count; subdivision++)
         {
            const auto weight = static_cast<float>(subdivision) / static_cast<float>(subdivision_count);
            const auto pos_lerp = lerp(pos_1, pos_2, func(weight));
            addKey(pos_lerp, subdiv_dist / length_sum);
            subdiv_dist += subdiv_step;
         }

         length_to_this_point += length;
      }

      addKey(positions.at(positions.size() - 1), 1.0f);
   }

   ///
   /// \brief Appends one key to the interpolation track.
   /// \param pos Key position.
   /// \param time_value Normalized key time in [0, 1].
   ///
   void addKey(const T& pos, float time_value)
   {
      Key key;
      key._pos = pos;
      key._time_value = time_value;
      _track.push_back(key);
   }

   ///
   /// \brief Computes velocity from the current position toward the interpolated target at `time_value`.
   /// \param current Current position.
   /// \param time_value Normalized sample time.
   /// \return Velocity vector toward the sampled position.
   ///
   T computeVelocity(const T& current, float time_value)
   {
      time_value = std::clamp(time_value, 0.0f, 1.0f);

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
            const auto a = 1.0f - (time_value - key_a._time_value);
            const auto b = 1.0f - (key_b._time_value - time_value);

            return (a * key_a._pos + b * key_b._pos) - current;
         }
      }

      return T{};
   }

   ///
   /// \brief Computes interpolated position at normalized `time_value`.
   /// \param time_value Normalized sample time; wrapped into [0, 1).
   /// \return Interpolated position.
   ///
   T computePosition(float time_value)
   {
      time_value = fmod(time_value, 1.0f);

      for (auto index_current = 0u; index_current < _track.size(); index_current++)
      {
         auto index_next = index_current + 1;
         if (index_next == _track.size())
         {
            index_next = static_cast<uint32_t>(_track.size() - 1);
         }

         const auto& key_a = _track.at(index_current);
         const auto& key_b = _track.at(index_next);

         if (time_value >= key_a._time_value && time_value <= key_b._time_value)
         {
            const auto time_value_offset = time_value - key_a._time_value;
            const auto distance = key_b._time_value - key_a._time_value;

            constexpr auto epsilon = 0.0001f;
            if (distance < epsilon)
            {
               return key_a._pos;
            }

            const auto time_value_relative = time_value_offset / distance;
            return key_a._pos + time_value_relative * (key_b._pos - key_a._pos);
         }
      }

      return T{};
   }

   ///
   /// \brief Advances internal time accumulator.
   /// \param delta Time step to add.
   ///
   void updateTime(float delta)
   {
      _time += delta;
   }

   ///
   /// \brief Updates current segment state and recomputes velocity when a key is reached.
   /// \param current_pos Current position on the path.
   /// \return `true` when a key transition happened during this update.
   ///
   bool update(const T& current_pos)
   {
      if (_track.empty())
      {
         return false;
      }

      auto reached = false;

      if (checkKeyReached(current_pos) || !_initialized)
      {
         _initialized = true;
         _current_key_index = nextKeyIndex();
         computeVelocity();
         reached = true;
      }

      return reached;
   }

   ///
   /// \brief Returns the current normalized direction vector between active keys.
   /// \return Current velocity direction.
   ///
   const T getVelocity()
   {
      return _velocity;
   }

   ///
   /// \brief Returns the full interpolation key track.
   /// \return Key track.
   ///
   const std::vector<PathInterpolation::Key>& getTrack() const
   {
      return _track;
   }

   ///
   /// \brief Returns the current internal time accumulator.
   /// \return Time value.
   ///
   float getTime() const
   {
      return _time;
   }

   ///
   /// \brief Sets the internal time accumulator.
   /// \param time New time value.
   ///
   void setTime(float time)
   {
      _time = time;
   }

private:
   void computeVelocity()
   {
      const T& a = _track[_current_key_index]._pos;
      const T& b = _track[nextKeyIndex()]._pos;

      _velocity = (a - b);
      _velocity.Normalize();
   }

   size_t nextKeyIndex() const
   {
      auto next_index = _current_key_index + 1;
      if (next_index == _track.size())
      {
         next_index = 0;
      }

      return next_index;
   }

   bool checkKeyReached(const T& current_pos) const
   {
      if (_track.empty())
      {
         return false;
      }

      const auto dist_length_squared = (current_pos - _track[_current_key_index]._pos).LengthSquared();
      return (dist_length_squared < 0.1f);
   }

   std::vector<Key> _track;
   float _time = 0.0f;
   bool _initialized = false;
   T _velocity;
   size_t _current_key_index = 0;
   Easings::Type _easing_type = Easings::Type::None;
};
