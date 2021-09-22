#include "gamecontrollerinfo.h"


//-----------------------------------------------------------------------------
/*!
   \param axisValues axis values
   \param buttonValues button values
   \param ballValues ball values
   \param hatValues hat values
*/
GameControllerInfo::GameControllerInfo(
   const std::vector<int32_t>& axis_values,
   const std::vector<bool>& button_values,
   const std::vector<GameControllerBallVector>& ball_values,
   const std::vector<int32_t>& hat_values
)
   : _axis_values(axis_values),
     _button_values(button_values),
     _ball_values(ball_values),
     _hat_values(hat_values)
{
}


//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addAxisValue(int32_t val)
{
   _axis_values.push_back(val);
}


//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addButtonState(bool val)
{
   _button_values.push_back(val);
}


//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addBallValue(const GameControllerBallVector& val)
{
   _ball_values.push_back(val);
}

//-----------------------------------------------------------------------------
/*!
   \param val value to append
*/
void GameControllerInfo::addHatValue(int32_t val)
{
   _hat_values.push_back(val);
}


//-----------------------------------------------------------------------------
/*!
   \return axis values
*/
const std::vector<int32_t>& GameControllerInfo::getAxisValues() const
{
   return _axis_values;
}


//-----------------------------------------------------------------------------
/*!
   \return button values
*/
const std::vector<bool>& GameControllerInfo::getButtonValues() const
{
   return _button_values;
}


//-----------------------------------------------------------------------------
/*!
   \return ball values
*/
const std::vector<GameControllerBallVector>& GameControllerInfo::getBallValues() const
{
   return _ball_values;
}


//-----------------------------------------------------------------------------
/*!
   \return hat values
*/
const std::vector<int32_t> &GameControllerInfo::getHatValues() const
{
   return _hat_values;
}
