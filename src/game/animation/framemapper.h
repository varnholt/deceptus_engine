#ifndef FRAMEMAPPER_H
#define FRAMEMAPPER_H

#include <chrono>
#include <iostream>
#include <vector>

#include "json/json.hpp"

template <class T>
class FrameMapper
{
public:
   FrameMapper(std::initializer_list<T> values, int32_t offset = 0) : _frame_durations(values), _offset(offset)
   {
      T sum{};
      for (const auto val : _frame_durations)
      {
         _frame_durations_accumulated.push_back(sum);
         sum += val;
      }
   }

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
