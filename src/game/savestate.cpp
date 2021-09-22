#include "savestate.h"

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
   return std::all_of(__save_states.begin(), __save_states.end(), [](const auto& s) {return s.isEmpty();});
}


bool SaveState::isEmpty() const
{
   return _player_info.mName.empty();
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

   json saveStates = json::parse(data);

   try
   {
      __save_states = saveStates.get<std::array<SaveState, 3>>();
   }
   catch (const std::exception& e)
   {
     std::cout << e.what() << std::endl;
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
   std::cout << "[-] saving " << filename << std::endl;

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
      {"playerinfo", data._player_info}
   };
}


void from_json(const nlohmann::json& j, SaveState& data)
{
   data._level_index = j.at("levelindex").get<uint32_t>();
   data._checkpoint = j.at("checkpoint").get<uint32_t>();
   data._player_info = j.at("playerinfo").get<PlayerInfo>();
}
