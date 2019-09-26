#include "savestate.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>


using json = nlohmann::json;


std::array<SaveState, 3> SaveState::sSaveStates;
uint32_t SaveState::sSlot = 0;


std::array<SaveState, 3>& SaveState::getSaveStates()
{
   return sSaveStates;
}


bool SaveState::isEmpty() const
{
   return mEmpty;
}


PlayerInfo& SaveState::getPlayerInfo()
{
   return sSaveStates[sSlot].mPlayerInfo;
}


void SaveState::deserialize(const std::string& data)
{
   json saveStates = json::parse(data);

   try
   {
      sSaveStates = saveStates.get<std::array<SaveState, 3>>();
   }
   catch (const std::exception& e)
   {
     std::cout << e.what() << std::endl;
   }
}


void SaveState::deserializeFromFile(const std::string &filename)
{
  std::ifstream ifs (filename, std::ifstream::in);

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



void SaveState::serializeToFile(const std::string &filename)
{
  std::string data = serialize();
  std::ofstream file(filename);
  file << data;
}


std::string SaveState::serialize()
{
   std::stringstream out;
   json arr(sSaveStates);
   out << std::setw(4) << arr << "\n\n";
   return out.str();
}


void to_json(nlohmann::json& j, const SaveState& data)
{
   j = json{
      {"levelindex", data.mLevelIndex},
      {"checkpoint", data.mCheckpoint},
      {"playerinfo", data.mPlayerInfo}
   };
}


void from_json(const nlohmann::json& j, SaveState& data)
{
   data.mLevelIndex = j.at("levelindex").get<int32_t>();
   data.mCheckpoint = j.at("checkpoint").get<int32_t>();
}
