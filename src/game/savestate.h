#pragma once

#include <string>


class PlayerInfo
{
   std::string mName;
};


class SaveState
{
   public:
      SaveState() = default;

   PlayerInfo mPlayerInfo;
   int32_t mLevelIndex = 0;
   int32_t mCheckpoint = 0;
};

