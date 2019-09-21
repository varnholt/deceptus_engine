#pragma once

#include <array>
#include <string>


class PlayerInfo
{
   std::string mName;
};


class SaveState
{

public:

   SaveState() = default;

   static std::array<SaveState, 3>& getSaveStates();
   bool isEmpty() const;

private:

   PlayerInfo mPlayerInfo;
   bool mEmpty = true;
   int32_t mLevelIndex = 0;
   int32_t mCheckpoint = 0;

   static std::array<SaveState, 3> sSaveStates;
};

