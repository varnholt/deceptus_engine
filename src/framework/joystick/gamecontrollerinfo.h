#pragma once

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
         const std::vector<int>& axisValues,
         const std::vector<bool>& buttonValues,
         const std::vector<GameControllerBallVector>& ballValues,
         const std::vector<int>& hatValues
      );

      //! add axis value
      void addAxisValue(int);

      //! add button state
      void addButtonState(bool);

      //! add ball value
      void addBallValue(const GameControllerBallVector&);

      //! add hat value
      void addHatValue(int);

      //! getter for axis values
      const std::vector<int>& getAxisValues() const;

      //! getter for button values
      const std::vector<bool>& getButtonValues() const;

      //! getter for ball values
      const std::vector<GameControllerBallVector>& getBallValues() const;

      //! getter for hat values
      const std::vector<int>& getHatValues() const;


   protected:

      //! axis values
      std::vector<int> mAxisValues;

      //! button values
      std::vector<bool> mButtonValues;

      //! ball values
      std::vector<GameControllerBallVector> mBallValues;

      //! hat values
      std::vector<int> mHatValues;
};

