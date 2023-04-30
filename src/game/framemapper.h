
#ifndef FRAMEMAPPER_H
#define FRAMEMAPPER_H

#include <chrono>
#include <vector>

#include "json/json.hpp"

class FrameMapper
{
public:
   FrameMapper();

   using HighResDuration = std::chrono::high_resolution_clock::duration;

   std::vector<HighResDuration> _frame_durations;
};

void to_json(nlohmann::json& j, const FrameMapper& d);
void from_json(const nlohmann::json& j, FrameMapper& d);

#endif // FRAMEMAPPER_H
