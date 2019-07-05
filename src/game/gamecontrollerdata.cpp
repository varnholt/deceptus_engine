#include "gamecontrollerdata.h"

GameControllerData GameControllerData::sInstance;


//-----------------------------------------------------------------------------
GameControllerData& GameControllerData::getInstance()
{
   return sInstance;
}


//-----------------------------------------------------------------------------
const GameControllerInfo& GameControllerData::getJoystickInfo() const
{
   return mJoystickInfo;
}


//-----------------------------------------------------------------------------
void GameControllerData::setJoystickInfo(const GameControllerInfo &joystickInfo)
{
   mJoystickInfo = joystickInfo;
}


//-----------------------------------------------------------------------------
bool GameControllerData::isControllerUsed() const
{
  return !mJoystickInfo.getAxisValues().empty();
}
