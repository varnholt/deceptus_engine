#include "gamecontrollerinfo.h"


//-----------------------------------------------------------------------------
/*!
   \param axisValues axis values
   \param buttonValues button values
   \param ballValues ball values
   \param hatValues hat values
*/
GameControllerInfo::GameControllerInfo(
   const std::vector<int>& axisValues,
   const std::vector<bool>& buttonValues,
   const std::vector<GameControllerBallVector>& ballValues,
   const std::vector<int> &hatValues
)
   : mAxisValues(axisValues),
     mButtonValues(buttonValues),
     mBallValues(ballValues),
     mHatValues(hatValues)
{
}


//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addAxisValue(int val)
{
   mAxisValues.push_back(val);
}


//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addButtonState(bool val)
{
   mButtonValues.push_back(val);
}


//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addBallValue(const GameControllerBallVector& val)
{
   mBallValues.push_back(val);
}

//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addHatValue(int val)
{
   mHatValues.push_back(val);
}


//-----------------------------------------------------------------------------
/*!
   \return axis values
*/
const std::vector<int>& GameControllerInfo::getAxisValues() const
{
   return mAxisValues;
}


//-----------------------------------------------------------------------------
/*!
   \return button values
*/
const std::vector<bool>& GameControllerInfo::getButtonValues() const
{
   return mButtonValues;
}


//-----------------------------------------------------------------------------
/*!
   \return ball values
*/
const std::vector<GameControllerBallVector>& GameControllerInfo::getBallValues() const
{
   return mBallValues;
}


//-----------------------------------------------------------------------------
/*!
   \return hat values
*/
const std::vector<int> &GameControllerInfo::getHatValues() const
{
   return mHatValues;
}
