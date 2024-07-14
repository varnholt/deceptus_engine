#pragma once

#include <cstdint>
#include <vector>

// joystick
#include "gamecontrollerballvector.h"

class GameControllerInfo
{
public:
   //! constructor a
   GameControllerInfo() = default;

   //! constructor b
   GameControllerInfo(
      const std::vector<int32_t>& axis_values,
      const std::vector<bool>& button_values,
      const std::vector<GameControllerBallVector>& ball_values,
      const std::vector<int32_t>& hat_values
   );

   //! add axis value
   void addAxisValue(int32_t);

   //! add button state
   void addButtonState(bool);

   //! add ball value
   void addBallValue(const GameControllerBallVector&);

   //! add hat value
   void addHatValue(int32_t);

   //! getter for axis values
   const std::vector<int32_t>& getAxisValues() const;

   //! getter for button values
   const std::vector<bool>& getButtonValues() const;

   //! getter for ball values
   const std::vector<GameControllerBallVector>& getBallValues() const;

   //! getter for hat values
   const std::vector<int32_t>& getHatValues() const;

protected:
   //! axis values
   std::vector<int32_t> _axis_values;

   //! button values
   std::vector<bool> _button_values;

   //! ball values
   std::vector<GameControllerBallVector> _ball_values;

   //! hat values
   std::vector<int32_t> _hat_values;
};
