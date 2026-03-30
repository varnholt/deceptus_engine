#pragma once

#include <cstdint>
#include <vector>

// joystick
#include "gamecontrollerballvector.h"

///
/// \brief Stores one sampled snapshot of controller axes, buttons, balls, and hats.
///
class GameControllerInfo
{
public:
   GameControllerInfo() = default;

   ///
   /// \brief Constructs a snapshot from fully populated input vectors.
   /// \param axis_values Axis values in SDL read order.
   /// \param button_values Button states in SDL button order.
   /// \param ball_values Ball delta values.
   /// \param hat_values Hat states.
   ///
   GameControllerInfo(
      const std::vector<int32_t>& axis_values,
      const std::vector<bool>& button_values,
      const std::vector<GameControllerBallVector>& ball_values,
      const std::vector<int32_t>& hat_values
   );

   ///
   /// \brief Appends one axis sample.
   /// \param value Axis value to append.
   ///
   void addAxisValue(int32_t value);

   ///
   /// \brief Appends one button state sample.
   /// \param value Button state to append.
   ///
   void addButtonState(bool value);

   ///
   /// \brief Appends one trackball sample.
   /// \param value Ball value to append.
   ///
   void addBallValue(const GameControllerBallVector& value);

   ///
   /// \brief Appends one hat sample.
   /// \param value Hat value to append.
   ///
   void addHatValue(int32_t value);

   ///
   /// \brief Returns sampled axis values.
   /// \return Axis value vector.
   ///
   const std::vector<int32_t>& getAxisValues() const;

   ///
   /// \brief Returns sampled button states.
   /// \return Button state vector.
   ///
   const std::vector<bool>& getButtonValues() const;

   ///
   /// \brief Returns sampled trackball values.
   /// \return Ball value vector.
   ///
   const std::vector<GameControllerBallVector>& getBallValues() const;

   ///
   /// \brief Returns sampled hat values.
   /// \return Hat value vector.
   ///
   const std::vector<int32_t>& getHatValues() const;

protected:
   std::vector<int32_t> _axis_values;
   std::vector<bool> _button_values;
   std::vector<GameControllerBallVector> _ball_values;
   std::vector<int32_t> _hat_values;
};
