#pragma once

#include <set>
#include <string>

#include "json/json.hpp"

/// \brief named gameplay facts belonging to one save slot, independent of collected items.
class ProgressionState
{
public:
   [[nodiscard]] bool completed(const std::string& id) const
   {
      return _completed.contains(id);
   }

   bool complete(const std::string& id)
   {
      return !id.empty() && _completed.insert(id).second;
   }

   bool reset(const std::string& id)
   {
      return _completed.erase(id) != 0;
   }

   [[nodiscard]] const std::set<std::string>& getCompleted() const
   {
      return _completed;
   }

private:
   std::set<std::string> _completed;
};

inline void to_json(nlohmann::json& json, const ProgressionState& progress)
{
   json = progress.getCompleted();
}

inline void from_json(const nlohmann::json& json, ProgressionState& progress)
{
   progress = {};
   if (!json.is_array())
   {
      return;
   }
   for (const auto& id : json)
   {
      if (id.is_string())
      {
         progress.complete(id.get<std::string>());
      }
   }
}
