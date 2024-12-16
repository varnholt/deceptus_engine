#include "savestate.h"

#include "framework/tools/log.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using json = nlohmann::json;

std::array<SaveState, 3> SaveState::__save_states;
uint32_t SaveState::__slot = 0;

std::array<SaveState, 3>& SaveState::getSaveStates()
{
   return __save_states;
}

bool SaveState::allEmpty()
{
   return std::all_of(__save_states.begin(), __save_states.end(), [](const auto& s) { return s.isEmpty(); });
}

bool SaveState::isEmpty() const
{
   return _player_info._name.empty();
}

void SaveState::invalidate()
{
   _player_info = {};
   _level_index = {};
   _checkpoint = {};
}

int32_t SaveState::computeProgress() const
{
   float progress = 0.0f;
   return static_cast<int32_t>(progress * 100);
}

SaveState& SaveState::getSaveState(uint32_t slot)
{
   return __save_states[slot];
}

PlayerInfo& SaveState::getPlayerInfo()
{
   return __save_states[__slot]._player_info;
}

SaveState& SaveState::getCurrent()
{
   return __save_states[__slot];
}

void SaveState::setCurrent(uint32_t slot)
{
   __slot = slot;
}

void SaveState::deserialize(const std::string& data)
{
   if (data.empty())
   {
      return;
   }

   json save_states = json::parse(data);

   try
   {
      __save_states = save_states.get<std::array<SaveState, 3>>();
   }
   catch (const std::exception& e)
   {
      Log::Error() << e.what();
   }
}

void SaveState::deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

   auto c = ifs.get();
   std::string data;

   while (ifs.good())
   {
      data.push_back(static_cast<char>(c));
      c = ifs.get();
   }

   ifs.close();

   deserialize(data);
}

void SaveState::serializeToFile(const std::string& filename)
{
   Log::Info() << "saving " << filename;

   std::string data = serialize();
   std::ofstream file(filename);
   file << data;
}

std::string SaveState::serialize()
{
   std::stringstream out;
   json arr(__save_states);
   out << std::setw(4) << arr << "\n\n";
   return out.str();
}

void to_json(nlohmann::json& j, const SaveState& data)
{
   j = json{
      {"levelindex", data._level_index},
      {"checkpoint", data._checkpoint},
      {"playerinfo", data._player_info},
      {"levelstate", data._level_state}
   };
}

void from_json(const nlohmann::json& j, SaveState& data)
{
   if (j.find("levelindex") != j.end())
   {
      data._level_index = j.at("levelindex").get<int32_t>();
   }

   if (j.find("checkpoint") != j.end())
   {
      data._checkpoint = j.at("checkpoint").get<int32_t>();
   }

   if (j.find("playerinfo") != j.end())
   {
      data._player_info = j.at("playerinfo").get<PlayerInfo>();
   }

   if (j.find("levelstate") != j.end())
   {
      data._level_state = j.at("levelstate");
   }
}

void SaveState::updatePlayerStatsToFile(const std::string& filename) const
{
   // open the file and read its current contents
   std::ifstream input_file(filename);
   if (!input_file.is_open())
   {
      Log::Error() << "could not open file for reading: " + filename;
      return;
   }

   nlohmann::json save_states_json;
   input_file >> save_states_json;
   input_file.close();

   // update only the stats field in the current SaveState's playerinfo
   save_states_json[__slot]["playerinfo"]["stats"] = _player_info._stats;

   // write updated JSON back to file
   std::ofstream output_file(filename);
   if (!output_file.is_open())
   {
      Log::Error() << "could not open file for writing: " + filename;
      return;
   }

   output_file << save_states_json.dump(4);
   output_file.close();
}
