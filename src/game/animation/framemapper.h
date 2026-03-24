#ifndef FRAMEMAPPER_H
#define FRAMEMAPPER_H

#include <chrono>
#include <iostream>
#include <vector>

#include "json/json.hpp"

template <class T>
/// \brief maps an elapsed value to a frame index using accumulated frame durations and an index offset.
class FrameMapper
{
public:
   /// \brief constructs a frame mapper from per-frame durations and an optional frame index offset.
   /// \param values ordered frame durations used to build accumulated frame boundaries.
   /// \param offset starting frame index added to computed indices.
   FrameMapper(std::initializer_list<T> values, int32_t offset = 0) : _frame_durations(values), _offset(offset)
   {
      T sum{};
      for (const auto val : _frame_durations)
      {
         _frame_durations_accumulated.push_back(sum);
         sum += val;
      }
   }

   /// \brief resolves which frame index corresponds to a given elapsed value.
   /// \param frame elapsed value compared against accumulated frame boundaries.
   /// \return mapped frame index including the configured offset.
   int32_t operator[](const T& frame)
   {
      const auto it = std::adjacent_find(
         _frame_durations_accumulated.begin(),
         _frame_durations_accumulated.end(),
         [&frame](const auto& /*prev*/, const auto& current) { return (frame < current); }
      );

      if (it != _frame_durations_accumulated.end())
      {
         return static_cast<int32_t>(_offset + std::distance(_frame_durations_accumulated.begin(), it));
      }

      return static_cast<int32_t>(_offset + _frame_durations_accumulated.size() - 1);
   }

   std::vector<T> _frame_durations;
   std::vector<T> _frame_durations_accumulated;
   int32_t _offset{};
};

#endif  // FRAMEMAPPER_H
